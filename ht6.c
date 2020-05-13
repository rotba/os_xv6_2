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
just_do_it(int a) {
    sleep(1000);
    exit();
    return;
}


int
main(int argc, char *argv[]) {
    if(fork()==0){
        just_do_it();
    }
    if(fork()==0){
        just_do_it();
    }
    if(fork()==0){
        just_do_it();
    }
}





