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
        while (1) {
            printf(0, "still alive\n");
            sleep(100);
        }
        exit();
    }

    sleep(500);
    printf(0, "sending kill\n");
    kill(pid, SIGKILL);
    wait();


    exit();
}


