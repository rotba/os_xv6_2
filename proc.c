#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

int debug = 0;

struct {
//    struct spinlock lock;
    struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

volatile int nextpid = 1;

extern void forkret(void);

extern void trapret(void);

extern void call_sigret(void);

extern void call_sigret_end(void);

extern void call_handler(void *, int);

static void wakeup1(void *chan);

void
pinit(void) {
//    initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
    return mycpu() - cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu *
mycpu(void) {
    int apicid, i;

    if (readeflags() & FL_IF)
        panic("mycpu called with interrupts enabled\n");

    apicid = lapicid();
    // APIC IDs are not guaranteed to be contiguous. Maybe we should have
    // a reverse map, or reserve a register to store &cpus[i].
    for (i = 0; i < ncpu; ++i) {
        if (cpus[i].apicid == apicid)
            return &cpus[i];
    }
    panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc *
myproc(void) {
    struct cpu *c;
    struct proc *p;
    pushcli();
    c = mycpu();
    p = c->proc;
    popcli();
    return p;
}


int
allocpid(void) {
    int pid = nextpid;
    int old = nextpid;
    do {
        old = nextpid;
        pid = nextpid + 1;
    } while (!cas((&nextpid), old, nextpid + 1));
    return pid;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc *
allocproc(void) {
    struct proc *p;
    char *sp;

    pushcli();
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        if (cas((volatile void *) &p->state, UNUSED, EMBRYO)) {
            goto found;
        }

    popcli();
    return 0;

    found:
    popcli();
    for (int i = 0; i < 32; ++i) {

        p->signal_handlers[i] = SIG_DFL;
    }
    p->pending_signals = 0;
    p->signal_mask = 0;

    p->pid = allocpid();

    // Allocate kernel stack.
    if ((p->kstack = kalloc()) == 0) {
        p->state = UNUSED;
        return 0;
    }
    sp = p->kstack + KSTACKSIZE;



//     Leave room for backup trap frame.
//    sp -= sizeof *p->backup;
//    p->backup = (struct trapframe *) sp;
//    // Leave room for trap frame.
    sp -= sizeof *p->tf;
    p->tf = (struct trapframe *) sp;

    // Set up new context to start executing at forkret,
    // which returns to trapret.
    sp -= 4;
    *(uint *) sp = (uint) trapret;

    sp -= sizeof *p->context;
    p->context = (struct context *) sp;
    memset(p->context, 0, sizeof *p->context);
    p->context->eip = (uint) forkret;

    return p;
}


//PAGEBREAK: 32
// Set up first user process.
void
userinit(void) {
    struct proc *p;
    extern char _binary_initcode_start[], _binary_initcode_size[];

    p = allocproc();

    initproc = p;
    if ((p->pgdir = setupkvm()) == 0)
        panic("userinit: out of memory?");
    inituvm(p->pgdir, _binary_initcode_start, (int) _binary_initcode_size);
    p->sz = PGSIZE;
    memset(p->tf, 0, sizeof(*p->tf));
//    memset(p->backup, 0, sizeof(*p->tf));
    p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
    p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
    p->tf->es = p->tf->ds;
    p->tf->ss = p->tf->ds;
    p->tf->eflags = FL_IF;
    p->tf->esp = PGSIZE;
    p->tf->eip = 0;  // beginning of initcode.S

    safestrcpy(p->name, "initcode", sizeof(p->name));
    p->cwd = namei("/");


    // this assignment to p->state lets other cores
    // run this process. the acquire forces the above
    // writes to be visible, and the lock is also needed
    // because the assignment might not be atomic.

    pushcli();
    if (!cas(&p->state, EMBRYO, RUNNABLE)) {
        panic("assert p->state == EMBRYO violated\n");
    }
    popcli();

}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n) {
    uint sz;
    struct proc *curproc = myproc();

    sz = curproc->sz;
    if (n > 0) {
        if ((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
            return -1;
    } else if (n < 0) {
        if ((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
            return -1;
    }
    curproc->sz = sz;
    switchuvm(curproc);
    return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void) {
    int i, pid;
    struct proc *np;
    struct proc *curproc = myproc();

    // Allocate process.
    if ((np = allocproc()) == 0) {
        return -1;
    }

    // Copy process state from proc.
    if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0) {
        kfree(np->kstack);
        np->kstack = 0;
        np->state = UNUSED;
        return -1;
    }
    np->sz = curproc->sz;
    np->parent = curproc;
    *np->tf = *curproc->tf;

    // Clear %eax so that fork returns 0 in the child.
    np->tf->eax = 0;

    for (i = 0; i < NOFILE; i++)
        if (curproc->ofile[i])
            np->ofile[i] = filedup(curproc->ofile[i]);
    np->cwd = idup(curproc->cwd);

    safestrcpy(np->name, curproc->name, sizeof(curproc->name));

    pid = np->pid;

    np->signal_mask = curproc->signal_mask;

    for (int i = 0; i < 32; i++) {
        np->signal_handlers[i] = curproc->signal_handlers[i];
    }


    pushcli();
    if (!cas(&np->state, EMBRYO, RUNNABLE)) {
        panic("assert p->state == EMBRYO violated");
    }
    popcli();


    return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void) {
    struct proc *curproc = myproc();
    struct proc *p;
    int fd;

    if (curproc == initproc)
        panic("init exiting");

    // Close all open files.
    for (fd = 0; fd < NOFILE; fd++) {
        if (curproc->ofile[fd]) {
            fileclose(curproc->ofile[fd]);
            curproc->ofile[fd] = 0;
        }
    }

    begin_op();
    iput(curproc->cwd);
    end_op();
    curproc->cwd = 0;


    pushcli();
    cas(&curproc->state, RUNNING, -ZOMBIE);

    // Parent might be sleeping in wait()
    wakeup1(curproc->parent);

    // Pass abandoned children to init.
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->parent == curproc) {
            p->parent = initproc;
            if (p->state == ZOMBIE || p->state == -ZOMBIE)
                wakeup1(initproc);
        }
    }

    // Jump into the scheduler, never to return.

    sched();
    panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void) {
    struct proc *p;
    int havekids, pid;
    struct proc *curproc = myproc();

    pushcli();
    for (;;) {
        // Scan through table looking for exited children.
        havekids = 0;
        for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
            if (p->parent != curproc)
                continue;
            havekids = 1;
            while (cas(&p->state, -ZOMBIE, -ZOMBIE) || cas(&p->state, ZOMBIE, ZOMBIE)) {
                if (cas(&p->state, ZOMBIE, -UNUSED)) {
                    // Found one.
                    pid = p->pid;
                    kfree(p->kstack);
                    p->kstack = 0;
                    freevm(p->pgdir);
                    p->pid = 0;
                    p->parent = 0;
                    p->name[0] = 0;
                    p->killed = 0;
                    cas(&p->state, -UNUSED, UNUSED);
                    popcli();
                    return pid;
                }
            }
        }

        // No point waiting if we don't have any children.
        if (!havekids || curproc->killed) {
            popcli();
            return -1;
        }

        // Wait for children to exit.  (See wakeup1 call in proc_exit.)
        sleep(curproc, null);  //DOC: wait-sleep
    }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void) {
    struct proc *p;
    struct cpu *c = mycpu();
    c->proc = 0;

    for (;;) {
        // signalsHanEnable interrupts on this processor.
        sti();

        // Loop over process table looking for process to run.
        pushcli();
//        acquire(&ptable.lock);
        for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
            if (p->state != RUNNABLE) {
                continue;
            }
            if (!cas(&p->state, RUNNABLE, -RUNNING))
                continue;
            // Switch to chosen process.  It is the process's job
            // to release ptable.lock and then reacquire it
            // before jumping back to us.
            if (debug) {
                cprintf("%s\n", p->name);
            }
            c->proc = p;
            switchuvm(p);
            if (!cas(&p->state, -RUNNING, RUNNING)) {
                panic("scheduler:: assert p->state == -RUNNING violated\n");
            }
            if (debug) {
                cprintf("%s\n", p->name);
            }
            swtch(&(c->scheduler), p->context);
            switchkvm();
            cas(&p->state, -RUNNABLE, RUNNABLE);
            (cas(&p->state, -SLEEPING, SLEEPING));
            cas(&p->state, -ZOMBIE, ZOMBIE);
            // Process is done running for now.
            // It should have changed its p->state before coming back.
            c->proc = 0;
        }
        popcli();
//        release(&ptable.lock);
    }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void) {
    int intena;
    struct proc *p = myproc();

//    if (!holding(&ptable.lock))
//        panic("sched ptable.lock");
    if (mycpu()->ncli != 1)
        panic("sched locks");
    if (p->state == RUNNING)
        panic("sched running");
    if (p->state == -RUNNING)
        panic("sched -running");
    if (readeflags() & FL_IF)
        panic("sched interruptible");
    intena = mycpu()->intena;
    if (debug) {
        cprintf("scheding\n");
    }
    swtch(&p->context, mycpu()->scheduler);
    mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void) {
//    acquire(&ptable.lock);  //DOC: yieldlock
    pushcli();
    myproc()->state = -RUNNABLE;//-run
    sched();
    popcli();
//    release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void) {
    static int first = 1;


    popcli();
    if (debug) {
        cprintf("forkretiing\n");
    }
    // Still holding ptable.lock from scheduler.
//    release(&ptable.lock);

    if (first) {
        // Some initialization functions must be run in the context
        // of a regular process (e.g., they call sleep), and thus cannot
        // be run from main().
        first = 0;
        iinit(ROOTDEV);

        if (debug) {
            cprintf("forkretiing2\n");
        }
        initlog(ROOTDEV);
    }

    // Return to "caller", actually trapret (see allocproc).

}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk) {
    if (debug) {
        cprintf("sleeping on %d\n", (int) chan);
    }
    struct proc *p = myproc();

    if (p == 0)
        panic("sleep");

//    if (lk == 0)
//        panic("sleep without lk");

    // Must acquire ptable.lock in order to
    // change p->state and then call sched.
    // Once we hold ptable.lock, we can be
    // guaranteed that we won't miss any wakeup
    // (wakeup runs with ptable.lock locked),
    // so it's okay to release lk.
//    if (lk != &ptable.lock) {  //DOC: sleeplock0
//        acquire(&ptable.lock);  //DOC: sleeplock1
//        release(lk);
//    }


//    if(!used_to_be_pushed){
//        pushcli();
//    }
    if (lk != null) {
        pushcli();
        release(lk);
    }
    // Go to sleep.
    p->chan = chan;
    p->state = -SLEEPING;
    if (debug) {
        cprintf("about to sched sleeping on %d\n", (int) p->chan);
    }
    sched();
    if (debug) {
        cprintf("finished sleeping %d\n", (int) p->chan);
    }

    // Tidy up.
    p->chan = 0;

    if (lk != null) {
        popcli();
        acquire(lk);
    }
    // Reacquire original lock.
//    if (lk != &ptable.lock) {  //DOC: sleeplock2
//        release(&ptable.lock);
//        acquire(lk);
//    }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan) {
    struct proc *p;
//    cprintf("chan:%d\n", (int)chan);
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->chan == chan) {
            if (debug) {
                cprintf("proc %s is in state %d\n", p->name, p->state);
            }
            while (cas(&p->state, -SLEEPING, -SLEEPING)) {}

            cas(&p->state, SLEEPING, RUNNABLE);

        }
    }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan) {
    pushcli();
    wakeup1(chan);
    popcli();
}

int
is_sigact(void *a) {
    if ((int) a == 0 || (int) a == 1 || (int) a == 9 || (int) a == 17 || (int) a == 19) {
        return 0;
    } else {
        return 1;
    }
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int//
is_kill(int signum, struct proc *p) {
    int i = signum;
    int ans = 0;
    if (i == SIGKILL) {
        cprintf("bug1\n");
        ans = 1;
    } else if ((p->signal_handlers[i] == (void *) SIGKILL ||
                p->signal_handlers[i] == (void *) SIG_DFL) && (i != SIGCONT && i != SIGSTOP)) {
        cprintf("bug2\n");
        ans = 1;
    } else if (is_sigact(p->signal_handlers[i]) &&
               (((struct sigaction *) (p->signal_handlers[i]))->sa_handler == (void *) SIGKILL ||
                (((struct sigaction *) (p->signal_handlers[i]))->sa_handler == (void *) SIG_DFL))) {
        cprintf("bug3\n");
        if (((struct sigaction *) (p->signal_handlers[i]))->sa_handler == (void *) SIGKILL)cprintf("bug3.1\n");
        if ((((struct sigaction *) (p->signal_handlers[i]))->sa_handler == (void *) SIG_DFL))cprintf("bug3.2\n");
        cprintf(
                "p:%p, pid:%d, i:%d, p->signal_handlers[i]: %p\n", p,p->pid, i,p->signal_handlers[i]
        );
        cprintf("%p\n", ((struct sigaction *) (p->signal_handlers[i]))->sa_handler);
        ans = 1;
    }
    return ans;
//    pushcli();
//    myproc()->killed = 1;
//    // Wake process from sleep if necessary.
////    while (cas(&myproc()->state, -SLEEPING, -SLEEPING)){}
//    cas(&myproc()->state, SLEEPING, RUNNABLE);
////    if (myproc()->state == SLEEPING)
////        myproc()->state = RUNNABLE;
//    popcli();
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid, int signum) {
    if (signum > 32)
        return -1;
    int bit = 1 << signum;
    struct proc *p;
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->pid == pid) {
//            pushcli();
            if (is_kill(signum, p)) {
                cprintf("signum:%d, pid:%d\n", signum, p->pid);
                pushcli();
                p->killed = 1;
                cas(&p->state, SLEEPING, RUNNABLE);
                popcli();
            } else {
                p->pending_signals |= bit;
            }
//            popcli();
            return 0;
        }
    }
    return -1;
}


void stop_handler() {
    int bit = 1 << SIGCONT;
    int done = 0;
    while (done == 0) {
//        acquire(&ptable.lock);
        done = ((myproc()->pending_signals & bit) == 0);
//        release(&ptable.lock);
        if (!done) {
            yield();
        }
    }
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void) {
    static char *states[] = {
            [UNUSED]    "unused",
            [EMBRYO]    "embryo",
            [SLEEPING]  "sleep ",
            [RUNNABLE]  "runble",
            [RUNNING]   "run   ",
            [ZOMBIE]    "zombie"
    };
    int i;
    struct proc *p;
    char *state;
    uint pc[10];

    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->state == UNUSED)
            continue;
        if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
            state = states[p->state];
        else
            state = "???";
        cprintf("%d %s %s", p->pid, state, p->name);
        if (p->state == SLEEPING) {
            getcallerpcs((uint *) p->context->ebp + 2, pc);
            for (i = 0; i < 10 && pc[i] != 0; i++)
                cprintf(" %p", pc[i]);
        }
        cprintf("\n");
    }
}

uint
sigprocmask(uint sigmask) {
    uint old_mask = myproc()->signal_mask;
    myproc()->signal_mask = sigmask;
    return old_mask;
}

int
sigaction(int signum, const struct sigaction *act, struct sigaction *oldact) {

    if (signum == SIGKILL || signum == SIGSTOP) {
        return -1;
    }

//    acquire(&ptable.lock);
    if (oldact != null) {
        *oldact = *(struct sigaction *) (myproc()->signal_handlers[signum]);
    }

    myproc()->signal_handlers[signum] = (void *) act;
    cprintf("signum: %d\n", signum);
    cprintf(
            "myproc():%p, pid:%d, signum:%d, myproc()->signal_handlers[signum]:%p\n", myproc(),myproc()->pid, signum,myproc()->signal_handlers[signum]
    );
    cprintf("done it, pid: %d, act->sa_handler:%p\n", myproc()->pid,
            ((struct sigaction *) (myproc()->signal_handlers[signum]))->sa_handler);

//    release(&ptable.lock);

    return 0;
}


void
signalsHandler(struct trapframe *parameter) {
    if ((parameter->cs & 3) != DPL_USER || myproc() == 0) return;
//    cprintf("tf: %x\n",myproc()->tf);
//    cprintf("backup: is %x\n",((int)(myproc()->backup)));
//    cprintf("parameter: is %x\n",parameter);
    int is_stopped = 0;
    do {

//        int is_killed = 0;
        int is_cont = 0;

//        acquire(&ptable.lock);


        for (int i = 0; i < 32; i++) {

            if (myproc()->pending_signals & (1 << i) &&
                (!(myproc()->signal_mask & (1 << i)) || i == SIGSTOP || i == SIGCONT)) {
//                (!(myproc()->signal_mask & (1 << i)) || i == SIGKILL || i == SIGSTOP || i == SIGCONT)) {
//                if (i == SIGKILL) {
//                    is_killed = 1;
//                    is_stopped = 0;
//                    break;
//                } else
//                    if ((myproc()->signal_handlers[i] == (void *) SIGKILL ||
//                            myproc()->signal_handlers[i] == (void *) SIG_DFL) && (i != SIGCONT && i != SIGSTOP)) {
//                    is_killed = 1;
//                    is_stopped = 0;
//                    break;
//                } else if (is_sigact(myproc()->signal_handlers[i]) &&
//                           (((struct sigaction *) (myproc()->signal_handlers[i]))->sa_handler == (void *) SIGKILL ||
//                            (((struct sigaction *) (myproc()->signal_handlers[i]))->sa_handler == (void *) SIG_DFL))) {
//                    is_killed = 1;
//                    is_stopped = 0;
//                    break;
//                }


                if (i == SIGSTOP) {
                    is_stopped = 1;
                    myproc()->pending_signals &= ~(1 << i);
                } else if (myproc()->signal_handlers[i] == (void *) SIGSTOP) {
                    is_stopped = 1;
                    myproc()->pending_signals &= ~(1 << i);
                } else if (is_sigact(myproc()->signal_handlers[i]) &&
                           (((struct sigaction *) (myproc()->signal_handlers[i]))->sa_handler == (void *) SIGSTOP)) {
                    is_stopped = 1;
                    myproc()->pending_signals &= ~(1 << i);
                }

                if (i == SIGCONT) {
                    is_cont = 1;
                    myproc()->pending_signals &= ~(1 << i);
                } else if (myproc()->signal_handlers[i] == (void *) SIGCONT) {
                    is_cont = 1;
                    myproc()->pending_signals &= ~(1 << i);
                } else if (is_sigact(myproc()->signal_handlers[i]) &&
                           (((struct sigaction *) (myproc()->signal_handlers[i]))->sa_handler == (void *) SIGCONT)) {
                    is_cont = 1;
                    myproc()->pending_signals &= ~(1 << i);
                }

            }
        }

//        release(&ptable.lock);


        if (myproc()->killed) {
//            kill_handler();
            is_stopped = 0;
            return;
        }

        if (is_stopped) {
            yield();
        }

        if (is_cont) {
            is_stopped = 0;
        }
    } while (is_stopped);

    //backup trapfram
//    acquire(&ptable.lock);
//    acquire(&ptable.lock);
    for (int i = 0; i < 32; i++) {

        int is_pending = (myproc()->pending_signals & 1 << i) != 0;

        int is_ignored = (myproc()->signal_mask & 1 << i) != 0 ||
                         (myproc()->signal_handlers[i]) == (void *) (SIG_IGN) ||
                         (void *) ((struct sigaction *) (myproc()->signal_handlers[i]))->sa_handler == (void *) SIG_IGN;

//        if (debug) {
//            cprintf("myproc()->signal_mask:%d\n", myproc()->signal_mask);
//            cprintf("i:%d\n", i);
//            if ((myproc()->pending_signals & 1 << i) != 0) {
//                cprintf("(void *) ((struct sigaction *) (myproc()->signal_handlers[i]))->sa_handler == (void *) SIG_IGN : %x:\n",
//                        (void *) ((struct sigaction *) (myproc()->signal_handlers[i]))->sa_handler ==
//                        (void *) SIG_IGN);
//            }
//        }
        if (is_pending && !is_ignored) {
            myproc()->pending_signals &= ~(1 << i);
            uint func = (uint) ((struct sigaction *) (myproc()->signal_handlers[i]))->sa_handler;
            myproc()->backup = *(myproc()->tf);


//            memmove(
//                    myproc()->backup,
//                    myproc()->tf,
//                    sizeof(struct trapframe)
//            );

            myproc()->signal_mask_backup = myproc()->signal_mask;
            if (is_sigact(myproc()->signal_handlers[i])) {
                myproc()->signal_mask = ((struct sigaction *) myproc()->signal_handlers[i])->sigmask;
            }
            uint code_length = (uint) call_sigret_end - (uint) call_sigret;
            myproc()->tf->esp -= code_length + (code_length % 8);
            uint call_sigret_add = (uint) myproc()->tf->esp;

            memmove(
                    (void *) call_sigret_add,
                    call_sigret,
                    code_length
            );
//            cprintf("%x, %x, %d, %p\n", *((int*)call_sigret), *((int*)call_sigret_add), code_length, call_sigret);
            myproc()->tf->esp -= 4;
            *((int *) (myproc()->tf->esp)) = i;
//            cprintf("esp: %p\n" , myproc()->tf->esp);
            myproc()->tf->esp -= 4;
            *((uint *) (myproc()->tf->esp)) = call_sigret_add;
            myproc()->tf->eip = (uint) func;
//            release(&ptable.lock);
            return;
        } else if (is_pending && is_ignored) {
            myproc()->pending_signals &= ~(1 << i);
        }
    }
    //restore trapframe
//    release(&ptable.lock);
    return;
}

void
sigret() {
//    cprintf("before restorage eip is %p\n",myproc()->tf->eip);
//    cprintf("before restorage backup eip is %p\n",myproc()->backup->eip);
//    acquire(&ptable.lock);
    *(myproc()->tf) = myproc()->backup;
//    memmove(
//            myproc()->tf,
//            myproc()->backup,
//            sizeof(struct trapframe)
//    );
    myproc()->signal_mask = myproc()->signal_mask_backup;
//    release(&ptable.lock);
    return;
}

