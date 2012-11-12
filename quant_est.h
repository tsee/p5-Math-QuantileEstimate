#ifndef QUANT_EST_H_
#define QUANT_EST_H_

typedef unsigned qe_uint;
typedef double qe_float;

typedef struct quant_est {
  qe_uint n;        /* total number of observations seen */
  qe_float epsilon; /* required precision as epsilon times n */
  qe_uint s;        /* total number of observations *in memory* */
} quant_est_t;

void qe_quantile(quant_est_t *qe, qe_float sigma, qe_float *value, qe_float *lower, qe_float *upper);
void qe_insert(quant_est_t *qe, qe_float value);

#endif
