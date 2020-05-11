//
// Created by a_braverman on 08/05/2020.
//

#include "types.h"
#include "stat.h"
#include "user.h"

void
hand10(int a) {
    printf(0, "hand10 was called\n");
    return;
}

void
hand2(int a) {
    printf(0, "hand2 was called SHOLD HAVE BEEN IGNORED\n");
    return;
}


int
main(int argc, char *argv[]) {
    int pid = fork();
    if (pid == 0) {
        struct sigaction a = {hand10, 0};
        struct sigaction b = {(void*)SIG_IGN, 0};
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





