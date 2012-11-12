#include "quant_est.h"
#include <stdlib.h>
#include <stdio.h>

typedef qs_uint unsigned int;
typedef qs_float double;

/* Each value has a range of ranks in which we know it is contained.
 * When referring to its lower and upper ranks, that is the range of ranks
 * we're referring to (inclusive). */
typedef struct qs_tuple {
  qs_float value; /* The actual value this tuple represents */
  qs_uint lower_rank_diff; /* lower_rank of this value minus lower_rank of prev. value */
  qs_uint rel_upper_rank; /* upper_rank of this value minus lower_rank of this value */
} qs_tuple_t;

