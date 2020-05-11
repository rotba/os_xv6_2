//
// Created by a_braverman on 08/05/2020.
//

#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[]) {
    int pid = fork();
    if (pid == 0) {
        printf(0, "Stopping\n");
        sleep(100);
        printf(0, "continued\n");
        exit();
    }

    sleep(100);
    printf(0, "sending stop\n");
    kill(pid, SIGSTOP);
    sleep(500);
    printf(0, "sending cont\n");
    kill(pid, SIGCONT);
    wait();


    exit();
}


