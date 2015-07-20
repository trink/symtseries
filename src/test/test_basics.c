#include "test.h"
#include "symtseries.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *test_get_symbol() {
    for (size_t pow = 1; pow < MAX_CORDINALITY; ++pow) {
        size_t zero_encoded = get_symbol(0.0, pow);
        mu_assert(zero_encoded == (1 << (pow-1)), 
                "zero encoded into %zu for cardinality %zu", zero_encoded, pow);
    }
    return NULL;
}

static char *test_to_iSAX() {
    return NULL;
}

static char* all_tests() {
    mu_run_test(test_get_symbol);
    mu_run_test(test_to_iSAX);
    return NULL;
}

int main() {
    char* result = all_tests();
    if (result) {
        printf("%s\n", result);
    } else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", mu_tests_run);

    return result != 0;
}
