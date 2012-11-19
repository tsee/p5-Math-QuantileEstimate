#include "qe_summary.h"
#include <stdlib.h>

#define QE_SUMMARY_F_SORTED 1

#define QE_SUMMARY_GET_FLAG(summary, flag) ((summary)->flags & (flag))
#define QE_SUMMARY_RESET_FLAG(summary, flag) ((summary)->flags &= ~(flag))
#define QE_SUMMARY_SET_FLAG(summary, flag) ((summary)->flags |= (flag))

static QEINLINE void summary_empty(qe_summary_t *summary);

int
qe_tuple_cmp_fun(const void *left_tuple, const void *right_tuple)
{
    const double v1 = ((qe_tuple_t *)left_tuple)->value;
    const double v2 = ((qe_tuple_t *)right_tuple)->value;
    return v1 < v2 ? -1 : v1==v2 ? 0 : 1;
}

int
summary_insert(qe_summary_t *summary, qe_float v)
{
    qe_tuple_t *tuple;
    if (summary->pos == summary->size)
        return QE_ERROR_FULL;

    tuple = (qe_tuple_t *)malloc(sizeof(qe_tuple_t));
    if (tuple == NULL)
        return QE_ERROR_OOM;

    tuple = &summary->tuples[summary->pos++];
    tuple->value = v;

    tuple->upper_rank = tuple->lower_rank = 0; /* filled when sorted */

    /* NOT sorted any more if it was before */
    QE_SUMMARY_RESET_FLAG(summary, QE_SUMMARY_F_SORTED);

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
    s->pos = 0;
    s->tuples = (qe_tuple_t *)malloc(sizeof(qe_tuple_t) * size);
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
    free(summary->tuples);
    free(summary);
}

void
summary_sort(qe_summary_t *summary)
{
    qe_uint i;
    qe_uint n = summary->pos;
    qe_tuple_t *tuples = summary->tuples;

    if (QE_SUMMARY_GET_FLAG(summary, QE_SUMMARY_F_SORTED))
        return;

    /* FIXME replace with merge-sort or similar true O(nlogn) */
    qsort((void *)summary->tuples, summary->pos, sizeof(qe_tuple_t), qe_tuple_cmp_fun);

    for (i = 0; i < n; ++i) {
        tuples[i].lower_rank = tuples[i].upper_rank = i+1;
    }

    QE_SUMMARY_SET_FLAG(summary, QE_SUMMARY_F_SORTED);
}

void
summary_compress(qe_summary_t *summary, qe_uint b)
{
    const qe_uint bsize = summary->pos;

    if (!QE_SUMMARY_GET_FLAG(summary, QE_SUMMARY_F_SORTED))
        summary_sort(summary);

    
}

static QEINLINE qe_tuple_t *
summary_quantile_query(qe_summary_t *summary, qe_uint r)
{
    if (!QE_SUMMARY_GET_FLAG(summary, QE_SUMMARY_F_SORTED))
        summary_sort(summary);

    
}

#define QE_TUPLE_CP(src, dest) \
    (void)memcpy(dest, src, sizeof(struct qe_tuple_t))


/*
 * The following binsearch function taken from libgit2 sources and modified:
 *
 * An array-of-pointers implementation of Python's Timsort
 * Based on code by Christopher Swenson under the MIT license
 */
qe_uint
summary_rank_binsearch(qe_summary_t *summary, qe_uint rank)
{
    int l, c, r;
    qe_tuple_t *lx, *cx;
    qe_uint size = summary->pos;
    qe_tuple_t *tuples = summary->tuples;

    l = 0;
    r = size - 1;
    c = r >> 1;
    lx = &tuples[l];

    /* check for beginning conditions */
    if (rank <= lx->upper_rank)
        return 0;
    else if (rank >= tuples[r].lower_rank)
        return r;

    cx = &tuples[c];
    while (1) {
        const int val = rank < cx->lower_rank ? -1 : rank > cx->upper_rank ? 1 : 0;
        if (val < 0) {
            if (c - l <= 1) return c;
            r = c;
        } else if (val > 0) {
            if (r - c <= 1) return c + 1;
            l = c;
            lx = cx;
        } else {
            return c;
        }
        c = l + ((r - l) >> 1);
        cx = &tuples[c];
    }
}

