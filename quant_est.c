#include "quant_est.h"
#include <stdlib.h>
#include <stdio.h>

#ifndef STMT_START
#   define STMT_START	do
#endif
#ifndef STMT_END
#   define STMT_END	while (0)
#endif

void compress(quant_est_t *qe);
void merge(quant_est_t *qe);
void empty(quant_est_t *qe);


/* Each value has a range of ranks in which we know it is contained.
 * When referring to its lower and upper ranks, that is the range of ranks
 * we're referring to (inclusive). */
typedef struct qe_tuple {
    qe_float value; /* The actual value this tuple represents */
    qe_uint lower_rank; /* lower_rank of this value */
    qe_uint upper_rank; /* upper_rank of this value */
} qe_tuple_t;

typedef struct qe_summary {
    qe_uint size;       /* Total summary size */
    qe_uint index;      /* Next index to write to in summary_tuples */
    qe_tuple_t *tuples; /* Summary tuple array */
} qe_summary_t;

static qe_summary_t *make_summary(qe_uint size);


static qe_summary_t *
make_summary(qe_uint size)
{
    qe_summary_t *s;
    s = (qe_summary_t *)malloc(sizeof(qe_summary_t));
    if (s == NULL)
        return NULL;
    s->size = size;
    s->index = 0;
    s->tuples = (qe_tuple_t *)malloc(sizeof(qe_tuple_t) * size);
    if (s->tuples == NULL) {
        free(s);
        return NULL;
    }
    return s;
}

