#ifndef QUANT_EST_H_
#define QUANT_EST_H_

typedef struct stream_struct stream_t;

stream_t * gkstr_new(double epsilon, int n);
void gkstr_free(stream_t *stream);

int gkstr_update(stream_t *stream, double e);

void gkstream_finish(stream_t *s);
double gkstream_query(stream_t *s, double q);

#endif
