#include "qe_summary.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#define QE_TUPLE_CP(src, dest) \
    (void)memcpy(dest, src, sizeof(qe_tuple_t))

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

    s->tuples = (qe_tuple_t *)malloc(sizeof(qe_tuple_t) * size);
    if (s->tuples == NULL) {
        free(s);
        return NULL;
    }

    s->size = size;
    s->pos = 0;
    /* Starts out not sorted and not compressed */
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

int
summary_compress(qe_summary_t *summary, qe_uint b)
{
    const qe_uint bsize = summary->pos;
    double factor;
    qe_tuple_t *tpl;
    qe_tuple_t *out_tuples;
    qe_uint out_pos, out_size, prev_rank, i, n;

    assert(b != 0);

    factor = bsize / b;

    if (!QE_SUMMARY_GET_FLAG(summary, QE_SUMMARY_F_SORTED))
        summary_sort(summary);

    out_pos = 0;
    out_size = b+1;
    out_tuples = (qe_tuple_t *)malloc(sizeof(qe_tuple_t) * out_size);
    if (out_tuples == NULL)
        return QE_ERROR_OOM;

    /* FIXME this is a very naive way of doing things. It may end up as O(b*log(b)),
     *       but it most certainly shouldn't have to. */
    /* FIXME This must be swimming in off by ones */
    prev_rank = 0;
    for (i = 0; i < out_size; ++i) {
        const qe_uint rank = i == 0 ? 1 : floor(factor*i);
        if (rank != prev_rank) {
            prev_rank = rank;
            tpl = summary_quantile_query(summary, rank);
            assert(tpl != NULL);
            QE_TUPLE_CP(tpl, &out_tuples[out_pos]);
            /* fprintf(stderr, "!%lu %lu\n", (unsigned long) out_pos, (unsigned long) rank); */
            ++out_pos;
        }
        /*else {
            fprintf(stderr, "prevrank was %lu\n", (unsigned long)prev_rank);
        }*/
    }

    /* Now swap tuple buffer */
    free(summary->tuples);
    summary->tuples = out_tuples;
    summary->pos = out_pos;
    summary->size = out_size;

    QE_SUMMARY_SET_FLAG(summary, QE_SUMMARY_F_COMPRESSED);
    return QE_OK;
}

/*
 * The following binsearch function taken from libgit2 sources and modified:
 *
 * An array-of-pointers implementation of Python's Timsort
 * Based on code by Christopher Swenson under the MIT license
 */
QEINLINE qe_uint
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


qe_tuple_t *
summary_quantile_query(qe_summary_t *summary, qe_uint rank)
{
    if (!QE_SUMMARY_GET_FLAG(summary, QE_SUMMARY_F_SORTED))
        summary_sort(summary);

    /* If this quantile was never compressed before, all ranks are
     * exact and we can do O(1) direct access. */
    if (!QE_SUMMARY_GET_FLAG(summary, QE_SUMMARY_F_COMPRESSED)) {
        if (rank < 1 || rank > summary->pos)
            return NULL;
        return &summary->tuples[rank-1];
    }
    else {
        /* O(log(size)) binary search */
        const qe_uint i = summary_rank_binsearch(summary, rank);
        qe_tuple_t *t = &summary->tuples[i];
        if (    (i == 0 && rank < t->lower_rank)
             || (i == summary->pos && rank > t->upper_rank) )
            return NULL;
        return t;
    }
}


