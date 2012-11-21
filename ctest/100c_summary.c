#include <stdio.h>
#include <stdlib.h>
#include <quant_est.h>
#include <qe_summary.h>
#include <assert.h>
#include <math.h>

#include "mytap.h"

void basic_summary_tests();
void summary_combine_tests();
int check_epsilon_n_summary(qe_summary_t *s, qe_float epsilon_n, qe_float true_median);

int
main ()
{
    basic_summary_tests();

    summary_combine_tests();
    ok_m(1, "alive");
    done_testing();
    return 0;
}

void
basic_summary_tests()
{
    const qe_uint b = 10;
    const qe_uint n = 5;
    const qe_float v[] = {12.3, -1., 1., 4., 2.};
    qe_tuple_t *tuples;
    qe_tuple_t *t;
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

#define CHECK_RANK_RESULT(msg)                                                      \
        /* Assert index = rank-1 */                                                 \
        is_int_m(j, i-1, "Base level summary: rank-1 is same as index (" msg ")");  \
        /* Assert rank relationships */                                             \
        ok_m(s->tuples[j].lower_rank <= s->tuples[j].upper_rank,                    \
             "Rank range for item is sane");                                        \
        if (j != 0) {                                                               \
            ok_m(s->tuples[j].value >= s->tuples[j-1].value,                        \
                 "Value monotonically increasing");                                 \
            ok_m(s->tuples[j].lower_rank > s->tuples[j-1].upper_rank,               \
                 "Rank strictly monotonically increasing");                         \
        }

        CHECK_RANK_RESULT("summary_rank_binsearch")

        /* Test O(1) variant of summary_quantile_query */
        t = summary_quantile_query(s, i);
        ok_m(t != NULL, "summary_quantile_query yields valid result");
        is_int_m(t->lower_rank, t->upper_rank, "Upper/lower ranks are identical");
        j = t->lower_rank-1;
        CHECK_RANK_RESULT("summary_quantile_query")

#undef CHECK_RANK_RESULT
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
        /* Set the compressed flag since rank ranges imply
         * that it's the output of compress */
        QE_SUMMARY_SET_FLAG(s, QE_SUMMARY_F_COMPRESSED);

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

    /* Revert the manual rank changes */
    for (i = 0; i < n; ++i) {
        tuples[i].lower_rank = i+1;
        tuples[i].upper_rank = i+1;
    }
    /*
    summary_insert(s, 1000.);
    summary_sort(s);
    */
    check_epsilon_n_summary(s, 1., 2.);
    summary_compress(s, 2); /* FIXME buggy compress? */
    check_epsilon_n_summary(s, 1. + 1./(2.*2.), 2.);

    summary_free(s);
}


#define SET_NEXT_TUPLE(t, v, l, u)   \
    STMT_START {                     \
        (t)->value      = (v);       \
        (t)->lower_rank = (l);       \
        (t)->upper_rank = (u);       \
        (t)++;                       \
    } STMT_END

#define CHECK_TUPLE(t, i, v, l, u)                                      \
    STMT_START {                                                        \
        printf("# Checking output tuple %lu\n", (unsigned long) (i));   \
        is_double_m(1e-6, (t)[i].value, (v), "Check value");            \
        is_int_m((t)[i].lower_rank, (l), "Check lower rank");           \
        is_int_m((t)[i].upper_rank, (u), "Check upper rank");           \
    } STMT_END
void
summary_combine_tests()
{
    const qe_uint n = 4;
    qe_summary_t *s1, *s2, *sout;
    qe_tuple_t *t1, *t2, *tout;

    s1 = summary_create(n);
    assert(s1);
    s2 = summary_create(n);
    assert(s2);

    t1 = s1->tuples;
    t2 = s2->tuples;

    SET_NEXT_TUPLE(t1, 2., 1, 1);
    SET_NEXT_TUPLE(t1, 4., 3, 4);
    SET_NEXT_TUPLE(t1, 8., 5, 6);
    SET_NEXT_TUPLE(t1, 17., 8, 8);

    SET_NEXT_TUPLE(t2, 1., 1, 1);
    SET_NEXT_TUPLE(t2, 7., 3, 3);
    SET_NEXT_TUPLE(t2, 12., 5, 6);
    SET_NEXT_TUPLE(t2, 15., 8, 8);

    /* all filled up */
    s1->pos = s2->pos = n;

    /* Prevent sorting (which assigns ranks due to how we filled manually */
    QE_SUMMARY_SET_FLAG(s1, QE_SUMMARY_F_SORTED);
    QE_SUMMARY_SET_FLAG(s2, QE_SUMMARY_F_SORTED);

    sout = summary_combine(s1, s2);
    tout = sout->tuples;
    CHECK_TUPLE(tout, 0, 1., 1, 1);
    CHECK_TUPLE(tout, 1, 2., 2, 3);
    CHECK_TUPLE(tout, 2, 4., 4, 6);
    CHECK_TUPLE(tout, 3, 7., 6, 8);
    CHECK_TUPLE(tout, 4, 8., 8, 11);
    CHECK_TUPLE(tout, 5, 12., 10, 13);
    CHECK_TUPLE(tout, 6, 15., 13, 15);
    CHECK_TUPLE(tout, 7, 17., 16, 16);

    check_epsilon_n_summary(s1, 3, 4);
    check_epsilon_n_summary(s2, 3, 7);
    check_epsilon_n_summary(sout, 3*2, 7);

    summary_free(s1);
    summary_free(s2);
    summary_free(sout);
}

int
check_epsilon_n_summary(qe_summary_t *s, qe_float epsilon_n, qe_float true_median)
{
    qe_uint i, n;
    qe_tuple_t *t;
    char msg[1024];
    int res = 1;

    n = s->pos;

    if (n <= 1)
        return 1;

    for (i = 1; i < n; ++i) {
        const float rdiff = s->tuples[i].upper_rank - s->tuples[i-1].lower_rank;
        sprintf(msg, "Step %lu: Small enough for summary with epsilon_n=%f",
                (unsigned long)i, (float)epsilon_n);
        if (ok_m(rdiff <= epsilon_n+1e-9, msg) == 0) {
            res = 0;
            printf("# Expected '%i' to be smaller than or equal to '%f'\n", (int)rdiff, (float)epsilon_n);
        }
    }

    t = summary_quantile_query(s, floor(s->pos*0.5));
    if (ok_m(fabs(t->value - true_median) <= epsilon_n,
             "Check that residual from true median is within epsilon_n"))
    {
        printf("# Expected '%f' to be smaller than or equal to '%f'\n",
               (float)fabs(t->value - true_median), (float)epsilon_n);
    }

    return res;
}
