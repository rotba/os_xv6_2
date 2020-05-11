//
// Created by a_braverman on 08/05/2020.
//

#include "types.h"
#include "stat.h"
#include "user.h"

void
hand(int a) {
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
main(int argc, char *argv[]) {
    printf(0, "started from the top\n");
    struct sigaction a = {hand, 0};
    sigaction(10, &a, null);
    printf(0, "%p\n", hand);
    int mypid =getpid();
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





