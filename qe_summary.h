#ifndef QE_SUMMARY_H_
#define QE_SUMMARY_H_

#include "quant_est.h"
#include "qe_defs.h"

/* Each value has a range of ranks in which we know it is contained.
 * When referring to its lower and upper ranks, that is the range of ranks
 * we're referring to (inclusive). */
typedef struct qe_tuple {
    qe_float value; /* The actual value this tuple represents */
    qe_uint lower_rank;  /* Lower-most rank represented by this value/tuple */
    qe_uint upper_rank;  /* Upper-most rank represented by this value/tuple */
} qe_tuple_t;

typedef struct qe_summary {
    qe_uint size;       /* Total summary size */
    qe_uint pos;        /* Next index to write to in summary_tuples (IOW number of current tuples) */
    qe_tuple_t *tuples; /* Summary tuple array */
    qe_uint flags;      /* FIXME waste of space... */
} qe_summary_t;

qe_summary_t *summary_create(qe_uint size);
int summary_insert(qe_summary_t *summary, qe_float value);
void summary_free(qe_summary_t *summary);
void summary_sort(qe_summary_t *summary);
void summary_compress(qe_summary_t *summary, qe_uint b);

/* tentative */
QEINLINE qe_uint summary_rank_binsearch(qe_summary_t *summary, qe_uint rank);

#endif
