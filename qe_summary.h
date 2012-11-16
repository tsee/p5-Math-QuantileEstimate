#ifndef QE_SUMMARY_H_
#define QE_SUMMARY_H_

#include "qe_defs.h"

/* Each value has a range of ranks in which we know it is contained.
 * When referring to its lower and upper ranks, that is the range of ranks
 * we're referring to (inclusive). */
typedef struct qe_tuple {
    qe_float value; /* The actual value this tuple represents */
    qe_uint lower_rank_diff; /* g_i: lower rank of this value minus lower rank of prev. value */
    qe_uint rank_range;  /* Delta_i: upper rank of this value minus lower rank of this value */
} qe_tuple_t;

typedef struct qe_summary {
    qe_uint size;       /* Total summary size */
    qe_uint index;      /* Next index to write to in summary_tuples */
    qe_tuple_t **tuples; /* Summary tuple array */
    qe_uint flags; /* FIXME waste of space... */
} qe_summary_t;

qe_summary_t *summary_create(qe_uint size);
int summary_insert(qe_summary_t *summary, qe_float value);
void summary_free(qe_summary_t *summary);

#define QE_TUPLE_CP(src, dest) \
    (void)memcpy(dest, src, sizeof(struct qe_tuple_t))

#endif
