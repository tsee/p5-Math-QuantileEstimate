#include "quant_est.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

struct stream_struct {
  summaries_t *summaries; /* AoA of tuples */
  double epsilon;
  int n;
  int b; /* block size */
};


/**************************************************
 * Tuple functions
 **************************************************/

QE_STATIC_INLINE tuple_t *
gktuple_new()
{
  tuple_t *t = calloc(1, sizeof(tuple_t));

  if (t == NULL)
    return NULL;

  return t;
}

QE_STATIC_INLINE tuple_t *
gktuple_clone(tuple_t *proto)
{
  tuple_t *res = malloc(sizeof(tuple_t));
  if (res == NULL)
    return NULL;

  memcpy(res, proto, sizeof(tuple_t));
  return res;
}

/**************************************************
 * gksummary_t functions
 **************************************************/

QE_STATIC_INLINE gksummary_t *
gks_new()
{
  return ptrarray_make(8, PTRARRAYf_FREE_ELEMS);
}

QE_STATIC_INLINE void
gks_free(gksummary_t *gk)
{
  ptrarray_free(gk);
}

/* N tuples in summary */
QE_STATIC_INLINE unsigned int
gks_len(gksummary_t *gk)
{
  return ptrarray_nelems(gk);
}

/* N items that the summary represents */
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

QE_STATIC_INLINE void
gks_clear(gksummary_t *gk)
{
  ptrarray_truncate(gk, 0);
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

/* From http://www.mathcs.emory.edu/~cheung/Courses/584-StreamDB/Syllabus/08-Quantile/Greenwald-D.html "Prune" */
QE_STATIC_INLINE gksummary_t *
gks_prune(gksummary_t *gk, int b)
{
  tuple_t *elt;
  tuple_t *tmp_tuple;
  gksummary_t *resgk;
  const size_t input_n_tuples = gks_len(gk);
  tuple_t **tuples = QE_GET_TUPLES(gk);
  size_t gk_idx = 0;
  size_t gk_rmin = 1;
  size_t i;
  
  resgk = gks_new();
  if (resgk == NULL)
    return NULL;

  tmp_tuple = gktuple_clone(tuples[0]); /* TODO consider if that allocation could be avoided */
  if (tmp_tuple == NULL)
    return NULL;
  ptrarray_push(resgk, tmp_tuple);

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

    if (((tuple_t *)ptrarray_peek(resgk))->v == elt->v) {
      /* ignore if we've already seen it */
      continue;
    }

    tmp_tuple = gktuple_clone(elt); /* TODO consider if that allocation could be avoided */
    if (tmp_tuple == NULL)
      return NULL;
    ptrarray_push(resgk, tmp_tuple);
  }

  return resgk;
}


/* This is the Merge algorithm from
 * http://www.cs.ubc.ca/~xujian/paper/quant.pdf .  It is much simpler than the
 * MERGE algorithm at
 * http://www.mathcs.emory.edu/~cheung/Courses/584-StreamDB/Syllabus/08-Quantile/Greenwald-D.html
 * or "COMBINE" in http://www.cis.upenn.edu/~mbgreen/papers/chapter.pdf
 * "Quantiles and Equidepth Histograms over Streams" (Greenwald, Khanna 2005) */
/* Takes ownership of the input summaries */
QE_STATIC_INLINE gksummary_t *
gks_merge(gksummary_t * s1, gksummary_t *s2, double epsilon, int N1, int N2)
{
  gksummary_t *smerge;
  size_t i1 = 0;
  size_t i2 = 0;
  int rmin = 0;
  int k = 0;
  tuple_t **s1t = QE_GET_TUPLES(s1);
  tuple_t **s2t = QE_GET_TUPLES(s2);
  const size_t n1 = gks_len(s1);
  const size_t n2 = gks_len(s2);
  tuple_t *newt;
  tuple_t *t = NULL;

  if (gks_len(s1) == 0) {
    gks_free(s1);
    return s2;
  }

  if (gks_len(s2) == 0) {
    gks_free(s2);
    return s1;
  }

  s1t[0]->g = 1;
  s2t[0]->g = 1;

  while (i1 < n1 || i2 < n2) {
    if (i1 < n1 && i2 < n2) {
      if (s1t[i1]->v <= s2t[i2]->v)
        t = s1t[i1++];
      else
        t = s2t[i2++];
    }
    else if (i1 < n1 && i2 >= n2) { /* technically, just i1 < n1 should be enough */
      t = s1t[i1++];
    }
    else if (i1 >= n1 && i2 < n2) { /* technically, just i2 < n2 needed */
      t = s2t[i2++];
    }
    else {
      abort(); /* never happens */
    }

    newt = gktuple_new();
    newt->v = t->v;
    newt->g = t->g;

    ++k;
    /* If you're following along with the paper, the Algorithm has
     * a typo on lines 9 and 11.  The summation is listed as going
     * from 1..k , which doesn't make any sense.  It should be
     * 1..l, the number of summaries we're merging.  In this case,
     * l=2, so we just add the sizes of the sets. */
    if (k == 1) {
      newt->delta = (int)(epsilon * ((double)N1 + (double)N2));
    }
    else {
      const int rmax = rmin + (int)(2*epsilon * ((double)N1 + (double)N2));
      rmin += newt->g;
      newt->delta = rmax - rmin;
      ptrarray_push(smerge, newt);
    }
  } /* end while */


  /* all done
   * The merged list might have duplicate elements -- merge them. */
  gks_merge_values(smerge);

  return smerge;
}



/**************************************************
 * stream_t functions
 **************************************************/

void
gkstr_free(stream_t *stream)
{
  size_t i;
  const size_t n = ptrarray_nelems(stream->summaries);
  gksummary_t **t = (gksummary_t **)ptrarray_data_pointer(stream->summaries);

  for (i = 0; i < n; ++i)
    gks_free(t[i]);

  ptrarray_free(stream->summaries);
  free(stream);
}

stream_t *
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
gkstr_tuple_cmp(const void *p1, const void *p2)
{
  tuple_t *t1 = (tuple_t *)p1;
  tuple_t *t2 = (tuple_t *)p2;

  /* Do not need stable sort */
  return (t1->v < t2->v)  ? -1 : 1;
}

int
gkstr_update(stream_t *stream, double e)
{
  tuple_t *tuple;
  gksummary_t **gks = (gksummary_t **)ptrarray_data_pointer(stream->summaries);
  gksummary_t *gk = gks[0];
  gksummary_t *tmp_summary;
  const size_t ntuples = gks_len(gk);
  tuple_t **tuples = QE_GET_TUPLES(gk);
  size_t k;
  size_t n_summaries;

  tuple = gktuple_new();
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
  qsort((void *)*tuples, ntuples, sizeof(tuple_t *), gkstr_tuple_cmp);

  gks_merge_values(gk);

  tmp_summary = gks_prune(gk, (stream->b+1)/2+1);
  gks_clear(gk); /* TODO this frees the tuples, but they could have instead been stolen */

  n_summaries = ptrarray_nelems(stream->summaries);
  for (k = 1; k < n_summaries; ++k) {
    gksummary_t *tmp;

    if (gks_len(gks[k]) == 0) {
      /* --------------------------------------
       * Empty: put compressed summary in sk
       * -------------------------------------- */
      gks_free(gks[k]); /* FIXME this is just terrible. Such a waste! */
      gks[k] = tmp_summary; /* Store it */
      return;
    }

    /* --------------------------------------
     * sk contained a compressed summary
     * -------------------------------------- */

    /* here we're merging two summaries with s.b * 2^k entries each */
    /* The gks_merge takes ownership of the two summaries passed in */
    tmp = gks_merge(
      gks[k],
      tmp_summary,
      stream->epsilon,
      stream->b * (1<<((unsigned int)k)),
      stream->b * (1<<((unsigned int)k))
    );
    tmp_summary = gks_prune(tmp, (stream->b+1)/2+1);
    /* NOTE: tmp_summary is used in next iteration
     * -  it is passed to the next level ! */

    gks[k] = gks_new(); /* Re-initialize FIXME avoid alloc */
  }

  /* fell off the end of our loop -- no more stream->summaries entries */
  ptrarray_push(stream->summaries, tmp_summary);
}

/* !! Must call Finish to allow processing queries */
void
gkstream_finish(stream_t *s)
{
  gksummary_t **gks = (gksummary_t **)ptrarray_data_pointer(s->summaries);
  gksummary_t *gk = gks[0];
  const size_t ntuples = gks_len(gk);
  tuple_t **tuples = QE_GET_TUPLES(gk);
  size_t size;
  size_t i;
  size_t n_summaries = ptrarray_nelems(s->summaries);

  /* TODO As per Damian, wouldn't have to merge into the summary at [0]. Could just use fresh summary to keep stream updateable. */
  qsort((void *)*tuples, ntuples, sizeof(tuple_t *), gkstr_tuple_cmp);
  gks_merge_values(gk);
  size = gks_len(gk);

  for (i = 1; i < n_summaries; ++i) {
    gk = gks_merge(gk, gks[i], s->epsilon, size, s->b * (1<<((unsigned int)i)));
    size += s->b * (1 << ((unsigned int)i));
  }
  gks[0] = gk; /* TODO see above */
}

/* GK query */
double
gkstream_query(stream_t *s, double q)
{
  /* convert quantile to rank */
  const int r = (int)(q * (double)s->n);
  int rmin = 0;
  int rmin_next;
  size_t i;
  gksummary_t **gks = (gksummary_t **)ptrarray_data_pointer(s->summaries);
  gksummary_t *gk = gks[0];
  const size_t ntuples = gks_len(gk);
  tuple_t **tuples = QE_GET_TUPLES(gk);


  for (i = 0; i < ntuples; ++i) {
    tuple_t *t = tuples[i];

    if (i+1 == ntuples)
      return t->v;

    rmin += t->g;
    rmin_next = rmin + tuples[i+1]->g;

    if (rmin <= r && r < rmin_next)
      return t->v;
  }

  abort(); /* not reached */
}


