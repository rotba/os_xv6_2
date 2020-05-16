//
// Created by a_braverman on 16/05/2020.
//


#include "types.h"
#include "stat.h"
#include "user.h"

void
hand1(int a) {
    printf(1, "hooray!!!!\n");
    return;
}

int
handlertest() {


    int pid = fork();
    if (pid == 0) {
        struct sigaction a = {hand1, 0};
        sigaction(10, &a, null);

        while (1) {
            printf(1, "still alive\n");
            sleep(100);
        }
        exit();
    }


    sleep(200);
    printf(1, "sending user handler\n");
    kill(pid, 10);
    sleep(300);
    printf(1, "sending kill\n");
    kill(pid, SIGKILL);
    wait();


    exit();
}


void
hand2(int a) {
    printf(0, "foo\n");
    return;
}

void
foo(int mypid) {
    printf(0, "foo\n");
    kill(mypid, 10);
    return;
}


int
ht2() {
    printf(0, "started from the top\n");
    struct sigaction a = {hand2, 0};
    sigaction(10, &a, null);
    printf(0, "%p\n", hand2);
    int mypid = getpid();
    while (1) {
        printf(0, "im alive\n");
        sleep(100);
        printf(0, "sending signal 10\n");
        kill(mypid, 10);
        printf(0, "waiting a lot of time\n");
        sleep(500);
        printf(0, "sending signal SIGKILL\n");
        kill(mypid, SIGKILL);
        sleep(500);
        printf(0, "i should be dead :-(\n");
    }
    exit();
}

void
hand30(int a) {
    printf(0, "hand10 was called\n");
    return;
}

void
hand3(int a) {
    printf(0, "hand2 was called SHOLD HAVE BEEN IGNORED\n");
    return;
}


int
ht3() {
    sigprocmask(1 << 2);
    int pid = fork();
    if (pid == 0) {
        struct sigaction a = {hand30, 0};
        struct sigaction b = {hand3, 0};
        sigaction(10, &a, null);
        sigaction(2, &b, null);

        while (1) {}
        exit();
    }


    sleep(200);
    printf(0, "sending 2 should ignore\n");
    kill(pid, 2);
    sleep(300);
    printf(0, "sending 10 should not ignore\n");
    kill(pid, 10);
    sleep(300);
    printf(0, "sending kill\n");
    kill(pid, SIGKILL);
    wait();


    exit();
}


void
hand40(int a) {
    printf(0, "hand10 was called\n");
    return;
}

void
hand4(int a) {
    printf(0, "hand2 was called SHOLD HAVE BEEN IGNORED\n");
    return;
}


int
ht4() {
    int pid = fork();
    if (pid == 0) {
        struct sigaction a = {hand40, 0};
        struct sigaction b = {(void *) SIG_IGN, 0};
        sigaction(10, &a, null);
        sigaction(2, &b, null);

        while (1) {}
        exit();
    }


    sleep(200);
    printf(0, "sending 2 should ignore\n");
    kill(pid, 2);
    sleep(300);
    printf(0, "sending 10 should not ignore\n");
    kill(pid, 10);
    sleep(300);
    printf(0, "sending kill\n");
    kill(pid, SIGKILL);
    wait();


    exit();
}

void
hand51(int a) {
    printf(0, "hand51 was called\n");
    return;
}

//volatile int handled_3 = 0;
void
hand5(int a) {
//    handled_3=1;
    printf(0, "hand5 was called\n");
    return;
}

void
hand50(int a) {
    printf(0, "handle 50\n");
    while (1) {}
    return;
}


int
ht5() {
    int pid = fork();
    if (pid == 0) {
        struct sigaction a = {hand50, 1 << 2};
        sigaction(10, &a, null);
        struct sigaction b = {hand5, 0};
        sigaction(2, &b, null);
        struct sigaction c = {hand51, 0};
        sigaction(3, &c, null);
        while (1) {}
        exit();
    }


    printf(0, "sending 10 should not ignore\n");
    kill(pid, 10);
    sleep(200);
    printf(0, "sending 2 should ignore\n");
    kill(pid, 2);
    sleep(300);
    printf(0, "sending 3, should call hand3\n");
    kill(pid, 3);
    sleep(300);
//    printf(0, "sending 2 should call hand2\n");
//    kill(pid, 2);
//    sleep(300);
    printf(0, "sending kill\n");
    kill(pid, SIGKILL);
    wait();


    exit();
}



void
hand6(int a) {
    printf(0, "hand6 was called\n");
    return;
}

//volatile int handled_3 = 0;
void
just_do_it() {
    sleep(1000);
    exit();
}


int
ht6() {
    int harbe =2;
    int tmp_harbe = harbe;
    while (tmp_harbe-->0){
        if(fork()==0){just_do_it();}
        wait();
    }
//    while (harbe-->0){
//        wait();
//        printf(0,"woke up %d\n", harbe);
//    }
    exit();
}


void
hand7(int a) {
    printf(0, "hand2 was called\n");
    return;
}

//volatile int handled_3 = 0;
void
just_do_it7() {
    sleep(1000);
    exit();
}


int
ht7() {
    int harbe =100;
    int tmp_harbe = harbe;
    while (tmp_harbe-->0){
        if(fork()==0){
            exit();
        }else{
            wait();
            printf(0,"woke up %d\n", tmp_harbe);
        }
    }
//    while (harbe-->0){
//        wait();
//        printf(0,"woke up %d\n", harbe);
//    }
    exit();
}





int
main(int argc, char *argv[]) {

    if (!fork())
        handlertest();
    wait();
    if (!fork())
        ht2();
    wait();
    if (!fork())
        ht3();
    wait();
    if (!fork())
        ht4();
    wait();
    if (!fork())
        ht5();
    wait();
    if (!fork())
        ht6();
    wait();


    exit();
}





