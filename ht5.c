//
// Created by a_braverman on 08/05/2020.
//

#include "types.h"
#include "stat.h"
#include "user.h"

void
hand2(int a) {
    printf(0, "hand2 was called\n");
    return;
}

//volatile int handled_3 = 0;
void
hand3(int a) {
//    handled_3=1;
    printf(0, "hand3 was called\n");
    return;
}
void
hand10(int a) {
    printf(0, "handle 10\n");
    while (1){}
    return;
}


int
main(int argc, char *argv[]) {
    int pid = fork();
    if (pid == 0) {
        struct sigaction a = {hand10, 1<<2};
        sigaction(10, &a, null);
        struct sigaction b = {hand2, 0};
        sigaction(2, &b, null);
        struct sigaction c = {hand3, 0};
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





