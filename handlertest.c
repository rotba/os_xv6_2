//
// Created by a_braverman on 08/05/2020.
//

#include "types.h"
#include "stat.h"
#include "user.h"

void
hand(int a) {
    printf(1, "hooray!!!!\n");
    return;
}


int
main(int argc, char *argv[]) {


    int pid = fork();
    if (pid == 0) {
        struct sigaction a = {hand, 0};
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





