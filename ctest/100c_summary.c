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
    qe_tuple_t *tuples;
    qe_uint i, j;

    /* Build up a very basic summary with index = rank-1 */
    qe_summary_t *s = summary_create(b);
    is_int_m(s->size, b, "New summary has right size");
    is_int_m(s->pos, 0, "New summary at pos 0");

    for (i = 0; i < n; ++i) {
        summary_insert(s, v[i]);
        is_int_m(s->pos, i+1, "Summary at expected pos");
    }

    /* Sort by value */
    printf("# Inserted %u elements into summary.\n", n);
    summary_sort(s);
    printf("# Sorted summary.\n");

    for (i = 1; i <= n; ++i) {
        j = summary_rank_binsearch(s, i);
        /* Assert index = rank-1 */
        is_int_m(j, i-1, "Base level summary: rank-1 is same as index");

        /* Assert rank relationships */
        ok_m(s->tuples[j].lower_rank <= s->tuples[j].upper_rank, "Rank range for item is sane");
        if (j != 0) {
            ok_m(s->tuples[j].value >= s->tuples[j-1].value, "Value monotonically increasing");
            ok_m(s->tuples[j].lower_rank > s->tuples[j-1].upper_rank, "Rank strictly monotonically increasing");
        }
    }

    /* Manually construct summary with actual rank ranges for testing */
    {
        const qe_uint lower_ranks[] = {1,2,3,5,8};
        const qe_uint upper_ranks[] = {1,2,4,7,8};
        tuples = s->tuples;
        for (i = 0; i < n; ++i) {
            tuples[i].lower_rank = lower_ranks[i];
            tuples[i].upper_rank = upper_ranks[i];
        }

        /* Yes, this is dumb. Patches to generate from the above rank tables welcome. */
        j = summary_rank_binsearch(s, 1);
        is_int_m(tuples[j].lower_rank, lower_ranks[0], "Base level summary: lower rank according to rank table (1)");
        is_int_m(tuples[j].upper_rank, upper_ranks[0], "Base level summary: upper rank according to rank table (1)");

        j = summary_rank_binsearch(s, 2);
        is_int_m(tuples[j].lower_rank, lower_ranks[1], "Base level summary: lower rank according to rank table (2)");
        is_int_m(tuples[j].upper_rank, upper_ranks[1], "Base level summary: upper rank according to rank table (2)");

        j = summary_rank_binsearch(s, 3);
        is_int_m(tuples[j].lower_rank, lower_ranks[2], "Base level summary: lower rank according to rank table (3)");
        is_int_m(tuples[j].upper_rank, upper_ranks[2], "Base level summary: upper rank according to rank table (3)");

        j = summary_rank_binsearch(s, 4);
        is_int_m(tuples[j].lower_rank, lower_ranks[2], "Base level summary: lower rank according to rank table (4)");
        is_int_m(tuples[j].upper_rank, upper_ranks[2], "Base level summary: upper rank according to rank table (4)");

        j = summary_rank_binsearch(s, 5);
        is_int_m(tuples[j].lower_rank, lower_ranks[3], "Base level summary: lower rank according to rank table (5)");
        is_int_m(tuples[j].upper_rank, upper_ranks[3], "Base level summary: upper rank according to rank table (5)");

        j = summary_rank_binsearch(s, 6);
        is_int_m(tuples[j].lower_rank, lower_ranks[3], "Base level summary: lower rank according to rank table (6)");
        is_int_m(tuples[j].upper_rank, upper_ranks[3], "Base level summary: upper rank according to rank table (6)");

        j = summary_rank_binsearch(s, 7);
        is_int_m(tuples[j].lower_rank, lower_ranks[3], "Base level summary: lower rank according to rank table (7)");
        is_int_m(tuples[j].upper_rank, upper_ranks[3], "Base level summary: upper rank according to rank table (7)");

        j = summary_rank_binsearch(s, 8);
        is_int_m(tuples[j].lower_rank, lower_ranks[4], "Base level summary: lower rank according to rank table (8)");
        is_int_m(tuples[j].upper_rank, upper_ranks[4], "Base level summary: upper rank according to rank table (8)");
    }

    ok_m(1, "alive");
    done_testing();
    return 0;
}
