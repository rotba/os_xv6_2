//
// Created by a_braverman on 08/05/2020.
//

#include "types.h"
#include "stat.h"
#include "user.h"

void
hand(int a) {
    printf(0, "hooray!!!!\n");
    return;
}


int
main(int argc, char *argv[]) {

    struct sigaction a = {hand, 0};
    sigaction(10, &a, null);
    int pid = fork();
    if (pid == 0) {


        while (1) {
            printf(0, "still alive\n");
            sleep(100);
        }
        exit();
    }


    sleep(200);
    printf(0, "sending user handler\n");
    kill(pid, 10);
    sleep(300);
    printf(0, "sending kill\n");
    kill(pid, SIGKILL);
    wait();


    exit();
}





