#include "quant_est.h"
#include <stdlib.h>
#include <stdio.h>

#include <math.h>

#include "qe_defs.h"
#include "ptrarray.h"

typedef struct {
  double v;
  int g;
  int delta;
} tuple_t;

typedef ptrarray_t gksummary_t;

#define QE_GET_TUPLES(gk) ((tuple_t **)ptrarray_data_pointer(gk))

typedef ptrarray_t summaries_t;

typedef struct {
  summaries_t *summaries; /* AoA of tuples */
  double epsilon;
  int n;
  int b; /* block size */
} stream_t;


QE_STATIC_INLINE tuple_t *
gks_new_tuple()
{
  tuple_t *t = calloc(1, sizeof(tuple_t));

  if (t == NULL)
    return NULL;

  return t;
}

QE_STATIC_INLINE tuple_t *
gks_clone_tuple(tuple_t *proto)
{
  tuple_t *res = malloc(sizeof(tuple_t));
  if (res == NULL)
    return NULL;

  memcpy(res, proto, sizeof(tuple_t));
  return res;
}

QE_STATIC_INLINE unsigned int
gks_len(gksummary_t *gk)
{
  return ptrarray_nelems(gk);
}

QE_STATIC_INLINE int
gks_less(gksummary_t *gk, size_t i, size_t j)
{
  tuple_t **d = QE_GET_TUPLES(gk);
  return d[i]->v < d[j]->v;
}

QE_STATIC_INLINE void
gks_swap(gksummary_t *gk, size_t i, size_t j)
{
  tuple_t *tmp;
  tuple_t **d = QE_GET_TUPLES(gk);
  tmp = d[i];
  d[i] = d[j];
  d[j] = tmp;
}

QE_STATIC_INLINE int
gks_size(gksummary_t *gk)
{
  size_t i;
  int n = 0;
  const size_t l = gks_len(gk);
  tuple_t **d = QE_GET_TUPLES(gk);

  if (l == 0)
    return 0;

  for (i = 0; i < l; ++i) {
    n += d[i]->g;
  }

  return n + d[l-1]->delta;
}

/* reduces the number of elements but doesn't lose precision.
 * Algorithm "value merging" in Appendix A of
 * "Power-Conserving Computation of Order-Statistics over Sensor Networks" (Greenwald, Khanna 2004)
 * http://www.cis.upenn.edu/~mbgreen/papers/pods04.pdf
 */
QE_STATIC_INLINE void
gks_merge_values(gksummary_t *gk)
{
  int missing = 0;
  size_t src;
  size_t dst = 0;
  tuple_t **d = QE_GET_TUPLES(gk);
  const size_t n = gks_len(gk);

  for (src = 1; src < n; ++src) {
    if (d[dst]->v == d[src]->v) {
      d[dst]->delta += d[src]->g + d[src]->delta;
      missing += d[src]->g;
      continue;
    }

    ++dst;
    /* add in the extra 'g' for the elements we removed */
    d[src]->g += missing;
    missing = 0;
    d[dst] = d[src];
  }

  ptrarray_truncate(gk, dst);
}


QE_STATIC_INLINE gksummary_t *
gks_new()
{
  return ptrarray_make(8, PTRARRAYf_FREE_ELEMS);
}

QE_STATIC_INLINE void
gkstr_free(stream_t *stream)
{
  size_t i;
  const size_t n = ptrarray_nelems(stream->summaries);
  gksummary_t **t = (gksummary_t **)ptrarray_data_pointer(stream->summaries);

  for (i = 0; i < n; ++i)
    ptrarray_free(t[i]);

  ptrarray_free(stream->summaries);
  free(stream);
}

QE_STATIC_INLINE stream_t *
gkstr_new(double epsilon, int n)
{
  const double epsN = epsilon * (double)n;
  const int b = (int)floor(log(epsN) / epsilon);
  stream_t *stream;
  gksummary_t *gk;

  if (b < 0)
    return NULL; /* FIXME error handling */

  stream = (stream_t *)malloc(sizeof(stream_t));
  if (stream == NULL)
    return NULL; /* FIXME error handling */

  stream->n = n;
  stream->b = b;

  stream->summaries = ptrarray_make(2, 0);
  if (stream->summaries == NULL) {
    free(stream);
    return NULL;
  }

  gk = gks_new();
  if (gk == NULL) {
    gkstr_free(stream);
    return NULL;
  }
  ptrarray_push(stream->summaries, gk);

  return stream;
}

static int
gks_tuple_cmp(const void *p1, const void *p2)
{
  tuple_t *t1 = (tuple_t *)p1;
  tuple_t *t2 = (tuple_t *)p2;

  /* Do not need stable sort */
  return (t1->v < t2->v)  ? -1 : 1;
}

QE_STATIC_INLINE int
gks_update(stream_t *stream, double e)
{
  tuple_t *tuple;
  gksummary_t **gks = (gksummary_t **)ptrarray_data_pointer(stream->summaries);
  gksummary_t *gk = gks[0];
  const size_t ntuples = gks_len(gk);
  tuple_t **tuples = QE_GET_TUPLES(gk);

  tuple = gks_new_tuple();
  if (tuple == NULL)
    return 1;

  ptrarray_push(gk, tuple);
  if (ntuples+1 < stream->b)
    return 0; /* done */

  /* -----------------------------------
   * Level 0 is full... PACK IT UP !!!
   * ----------------------------------- */

  /* TODO nlogn */
  /* FIXME validate ptrs and derefs... */
  qsort((void *)*tuples, n, sizeof(tuple_t *), gks_tuple_cmp);

  gks_merge_values(gk);


	sc := prune(s.summary[0], (s.b+1)/2+1)
	s.summary[0] = s.summary[0][:0] // empty

	for k := 1; k < len(s.summary); k++ {

		if len(s.summary[k]) == 0 {
			* --------------------------------------
			   Empty: put compressed summary in sk
			   --------------------------------------
			s.summary[k] = sc // Store it
			return // Done
		}

		* --------------------------------------
		   sk contained a compressed summary
		   --------------------------------------

		tmp := merge(s.summary[k], sc, s.epsilon, s.b*(1<<uint(k)), s.b*(1<<uint(k))) // here we're merging two summaries with s.b * 2^k entries each
		sc = prune(tmp, (s.b+1)/2+1)
		// NOTE: sc is used in next iteration
		// -  it is passed to the next level !

		s.summary[k] = s.summary[k][:0] // Re-initialize
	}

	// fell off the end of our loop -- no more s.summary entries
	s.summary = append(s.summary, sc)
	if debug {
		fmt.Println("fell off the end:", sc.Size())
	}
	s.Dump()
}



/* From http://www.mathcs.emory.edu/~cheung/Courses/584-StreamDB/Syllabus/08-Quantile/Greenwald-D.html "Prune" */
QE_STATIC_INLINE gksummary_t *
gks_prune(gksummary_t *gk, int b)
{
  tuple_t *elt;
  gksummary_t *resgk;
  const size_t input_n_tuples = gks_len(gk);
  tuple_t **tuples = QE_GET_TUPLES(gk);
  size_t gk_idx = 0;
  size_t gk_rmin = 1;
  size_t i;
  
  resgk = gks_new();
  if (resgk == NULL)
    return NULL;

  elt = gks_clone_tuple(tuples[0]); /* TODO consider if that allocation could be avoided */
  ptrarray_push(resgk, elt);

  for (i = 1; i <= b; ++i) {
    const int rank = (int)((double)gks_size(gk) * (double)i / (double)b);

    /* find an element of rank 'rank' in gk */
    while (gk_idx < input_n_tuples-1) {

      if (gk_rmin <= rank && rank < gk_rmin + tuples[gk_idx+1]->g)
        break;

      ++gk_idx;
      gk_rmin += tuples[gk_idx]->g;
    }

    if (gk_idx >= input_n_tuples)
      gk_idx = input_n_tuples - 1;

    /* FIXME check whether this creates a new variable WITHIN this scope only in
     *       the Go version */
    elt = tuples[gk_idx];

    /* FIXME what's the r != nil supposed to do in the Go version? */
    if (/*r != nil &&*/ ptrarray_peek(resgk)->v == elt->v) {
      /* ignore if we've already seen it */
      continue
    }
    // FIXME clone elt!
    ptrarray_push(resgk, elt);
  }

  return resgk;
}
