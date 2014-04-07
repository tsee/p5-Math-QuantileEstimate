#ifndef PTRARRAY_H_
#define PTRARRAY_H_

#include <stdlib.h>
#include <assert.h>
#include "qe_defs.h"

/* A simple, geometrically growing stack implementation using
 * void * as value type. All API functions are static, many inline. */

/* If set in the flags, this will cause remaining elements 
 * on the stack to be freed when ptrarray_free() is called. */
#define PTRARRAYf_FREE_ELEMS 1

/* Geometric growth of the stack */
#ifndef PTRARRAY_GROWTH_FACTOR
# define PTRARRAY_GROWTH_FACTOR 1.3
#endif

typedef struct {
  void **data;
  unsigned int size;
  unsigned int nextpos;
  unsigned int flags;
} ptrarray_t;

/****************************
 * API
 */

/* Constructor. If flags has PTRARRAYf_FREE_ELEMS set, then ptrarray_free will free()
 * all elements still on the stack. */
QE_STATIC_INLINE ptrarray_t *ptrarray_make(const unsigned int initsize, unsigned int flags_);

/* Release ptrarray's memory. */
QE_STATIC_INLINE void ptrarray_free(ptrarray_t *stack);

/* Add another element at the end */
QE_STATIC_INLINE int ptrarray_push(ptrarray_t *stack, void *elem);

/* Add another element to the end without checking whether there's
 * enough space for it. Purely an optimization to avoid a branch. */
QE_STATIC_INLINE int ptrarray_push_nocheck(ptrarray_t *stack, void *elem);

/* Remove the top element from the array and return it. Returns NULL if
 * the array is empty. */
QE_STATIC_INLINE void *ptrarray_pop(ptrarray_t *stack);

/* Remove end elemnt from array and return it. Doesn't bother checking
 * whether the array still has data, so can result in nasty things if
 * that's not known from your usage context. */
QE_STATIC_INLINE void * ptrarray_pop_nocheck(ptrarray_t *stack);

/* Return the end element without removing it. */
QE_STATIC_INLINE void *ptrarray_peek(ptrarray_t *stack);

/* Number of elements. */
QE_STATIC_INLINE unsigned int ptrarray_nelems(ptrarray_t *stack);

/* Is the array empty? */
QE_STATIC_INLINE int ptrarray_empty(ptrarray_t *stack);



/* Returns the raw pointer to the array contents, base at 0. */
QE_STATIC_INLINE void **ptrarray_data_pointer(ptrarray_t *stack);

/* Force the array to grow to at least the given absolute number of elements.
 * Not necessary to call manually. The array will grow dynamically. */
static int ptrarray_grow(ptrarray_t *stack, unsigned int nelems);

/* If you're done pushing array is there to stay,
 * you can compact. Not usually very useful, though. */
QE_STATIC_INLINE void ptrarray_compact(ptrarray_t *stack);

/* Assert that the array has at least nelems of space left */
QE_STATIC_INLINE int ptrarray_assert_space(ptrarray_t *stack, const unsigned int nelems);

/* Truncate length of ptrarray to given new length (if necessary). Will free the
 * elements if PTRARRAYf_FREE_ELEMS is set. */
QE_STATIC_INLINE void ptrarray_truncate(ptrarray_t *stack, size_t newlen);

/***************************
 * Implementation
 */

QE_STATIC_INLINE ptrarray_t *
ptrarray_make(const unsigned int initsize, unsigned int flags_)
{
  ptrarray_t *stack = (ptrarray_t *)malloc(sizeof(ptrarray_t));
  if (stack == NULL)
    return NULL;

  stack->size = (initsize == 0 ? 16 : initsize);
  stack->nextpos = 0;
  stack->flags = flags_;
  stack->data = malloc(sizeof(void *) * stack->size);

  return stack;
}

QE_STATIC_INLINE void
ptrarray_free(ptrarray_t *stack)
{
  if (stack->flags & PTRARRAYf_FREE_ELEMS) {
    unsigned int i;
    const unsigned int n = stack->nextpos;
    for (i = 1; i <= n; ++i)
      free(stack->data[n-i]);
  }
  
  free(stack->data);
  free(stack);
}

static int
ptrarray_grow(ptrarray_t *stack, unsigned int nelems)
{
  const unsigned int newsize
    = (unsigned int)( ( ( stack->size > nelems
                          ? stack->size
                          : nelems) + 5 /* some static growth for avoiding lots
                                         * of reallocs on very small arrays */
                      ) * PTRARRAY_GROWTH_FACTOR);
  stack->data = realloc(stack->data, sizeof(void *) * newsize);
  if (stack->data == NULL)
    return 1; /* OOM */
  stack->size = newsize;
  return 0;
}

/* local macro to force inline */
#define PTRARRAY_SPACE_ASSERT(s, nelems)            \
    ((s)->size < (s)->nextpos+(nelems)              \
      ? ptrarray_grow((s), (s)->nextpos + (nelems)) \
      : 0)

/* just for the API */
QE_STATIC_INLINE int
ptrarray_assert_space(ptrarray_t *stack, const unsigned int nelems)
{
  return PTRARRAY_SPACE_ASSERT(stack, nelems);
}

QE_STATIC_INLINE int
ptrarray_push(ptrarray_t *stack, void *elem)
{
  if (PTRARRAY_SPACE_ASSERT(stack, 1))
    return 1; /* OOM */
  stack->data[stack->nextpos++] = elem;
  return 0;
}

QE_STATIC_INLINE int
ptrarray_push_nocheck(ptrarray_t *stack, void *elem)
{
  assert(PTRARRAY_SPACE_ASSERT(stack, 1));
  stack->data[stack->nextpos++] = elem;
  return 0;
}

#undef PTRARRAY_SIZE_ASSERT

QE_STATIC_INLINE void *
ptrarray_pop(ptrarray_t *stack)
{
  return stack->nextpos == 0
         ? NULL
         : stack->data[--stack->nextpos];
}

QE_STATIC_INLINE void *
ptrarray_pop_nocheck(ptrarray_t *stack)
{
  assert(stack->nextpos > 0);
  return stack->data[--stack->nextpos];
}


QE_STATIC_INLINE void
ptrarray_truncate(ptrarray_t *stack, size_t newlen)
{
  size_t i;
  const size_t n = ptrarray_nelems(stack);

  if (n <= newlen)
    return;

  if (stack->flags & PTRARRAYf_FREE_ELEMS) {
    for (i = newlen; i < n; ++i) {
      free(stack->data[i]);
    }
  }

  stack->nextpos = newlen;
}

QE_STATIC_INLINE void *
ptrarray_peek(ptrarray_t *stack)
{
  return stack->nextpos == 0
         ? NULL
         : stack->data[stack->nextpos - 1];
}

QE_STATIC_INLINE unsigned int
ptrarray_nelems(ptrarray_t *stack)
{
  return stack->nextpos;
}

QE_STATIC_INLINE int
ptrarray_empty(ptrarray_t *stack)
{
  return (stack->nextpos == 0);
}

QE_STATIC_INLINE void **
ptrarray_data_pointer(ptrarray_t *stack)
{
  return stack->data;
}


/* If you're done pushing and the array is there to stay,
 * you can compact. Not usually very useful, though. */
QE_STATIC_INLINE void
ptrarray_compact(ptrarray_t *stack)
{
  const unsigned int minsize = stack->nextpos + 1;
  if (stack->size > minsize) {
    stack->data = realloc(stack->data, sizeof(void *) * minsize);
    stack->size = minsize;
  }
}

#endif
