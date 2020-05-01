//
// Created by a_braverman on 01/05/2020.
//

#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[]) {
    uint mask = sigprocmask(5);
    printf(1, "%d\n", mask);
    mask = sigprocmask(7);
    printf(1, "%d\n", mask);
    exit();
}


