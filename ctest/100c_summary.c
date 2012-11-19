#include <stdio.h>
#include <stdlib.h>
#include <quant_est.h>
#include <qe_summary.h>

#include "mytap.h"

int
main ()
{
    const qe_uint b = 10;
    const qe_uint n = 5;
    const qe_float v[] = {12.3, -1., 1., 4., 2.};
    qe_uint i, j;

    qe_summary_t *s = summary_create(b);
    is_int_m(s->size, b, "New summary has right size");
    is_int_m(s->pos, 0, "New summary at pos 0");

    for (i = 0; i < n; ++i) {
        summary_insert(s, v[i]);
        is_int_m(s->pos, i+1, "Summary at expected pos");
    }
    printf("# Inserted %u elements into summary.\n", n);
    summary_sort(s);
    printf("# Sorted summary.\n");

    for (i = 1; i <= n; ++i) {
        j = summary_rank_binsearch(s, i);
        is_int_m(j, i-1, "Base level summary: rank-1 is same as index");
    }

    ok_m(1, "alive");
    done_testing();
    return 0;
}
