#include <stdio.h>
#include "sfmm.h"
#include "debug.h"
#include "my_header.h"

int main(int argc, char const *argv[]) {

    sf_mem_init(); // INIT
    double *ptr1;
    double *ptr2;
    double *ptr3;
    debug("%s", "##################TEST FOR SF_MALLOC()#################");
    ptr1 = sf_malloc(sizeof(double));
    ptr2 = sf_malloc(sizeof(double));
    ptr3 = sf_malloc(sizeof(double));
    debug("HEADER PTR : %p", ptr1);
    sf_varprint(ptr1);
    debug("HEADER PTR : %p", ptr2);
    sf_varprint(ptr2);
    debug("HEADER PTR : %p", ptr3);
    sf_varprint(ptr3);

    debug("%s", "##################TEST FOR SF_REALLOC()#################");
    ptr2 = sf_realloc(ptr2, sizeof(double) * 10);
    sf_varprint(ptr2);
    // *ptr = 320320320e-320;
    // printf("%f\n", *ptr);
    debug("%s", "##################TEST FOR SF_FREE()#################");
    sf_free(ptr1);
    sf_free(ptr2);
    sf_free(ptr3);


    sf_mem_fini(); // FINISH

    return EXIT_SUCCESS;
}
