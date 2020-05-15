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
just_do_it() {
    sleep(1000);
    exit();
}


int
main(int argc, char *argv[]) {
    int harbe =100;
    int tmp_harbe = harbe;
    while (tmp_harbe-->0){
        int pid;
        if((pid = fork())==0){just_do_it();}
        sleep(0);
        sleep(0);
        kill(pid , SIGKILL);
        wait();
        printf(0,"woke up %d\n", tmp_harbe);
    }
//    while (harbe-->0){
//        wait();
//        printf(0,"woke up %d\n", harbe);
//    }
    exit();
}





