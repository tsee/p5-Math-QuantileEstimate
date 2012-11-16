#include "qe_summary.h"

#define QE_SUMMARY_F_SORTED 1

static QEINLINE void summary_empty(qe_summary_t *summary);


static QEINLINE void
summary_empty(qe_summary_t *summary)
{
    qe_uint i, n;
    qe_tuple_t **tuples;
    n = summary->index;
    tuples = summary->tuples;
    for (i = 0; i < n; ++i) {
        free(tuples[i]);
    }
    summary->index = 0;
}


int cmp_fun(const void *left_tuple, const void *right_tuple)
{
    const double v1 = ((qe_tuple_t *)left_tuple)->value;
    const double v2 = ((qe_tuple_t *)right_tuple)->value;
    return v1==v2 ? 0 : v1 < v2 ? -1 : 1;
}

int
summary_insert(qe_summary_t *summary, qe_float v)
{
    int pos;
    qe_tuple_t *tuple;
    if (summary->index == summary->size)
        return QE_ERROR_FULL;

    tuple = (qe_tuple_t *)malloc(sizeof(qe_tuple_t));
    if (tuple == NULL)
        return QE_ERROR_OOM;

    tuple = summary->tuples[summary->index++];
    tuple->value = v;

    tuple->lower_rank_diff = 0; /* filled when sorted */
    tuple->rank_range = 0; /* filled when sorted */

    /* NOT sorted any more if it was before */
    summary->flags &= ~QE_SUMMARY_F_SORTED;

    return 0;
}

qe_summary_t *
summary_create(qe_uint size)
{
    qe_summary_t *s;
    s = (qe_summary_t *)malloc(sizeof(qe_summary_t));
    if (s == NULL)
        return NULL;
    s->size = size;
    s->index = 0;
    s->tuples = (qe_tuple_t **)malloc(sizeof(qe_tuple_t *) * size);
    if (s->tuples == NULL) {
        free(s);
        return NULL;
    }
    s->flags = 0;
    return s;
}

void
summary_free(qe_summary_t *summary)
{
    summary_empty(summary);
    free(summary->tuples);
    free(summary);
}

