//
// Created by a_braverman on 20/05/2020.
//


#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[]) {
    int total = 64;
    int k = total;

    while (k--) {
        if (!fork()) {
            printf(1, "%d strted\n", k);

            int i = 1000;
//    int dummy = 0;
            for (double j = 0.01; j < i; j += 0.01) {
                double x = x + 3.14 * 89.64;
            }
            printf(1, "%d done\n", k);
            exit();
        }

    }

    while (k++ < total) {
        wait();
//        sleep(100);
    }
    printf(1, "all done\n");


    exit();
}







