#ifndef QE_DEFS_H_
#define QE_DEFS_H_

#if DEBUG
#   define QEINLINE
#else
#   define QEINLINE inline
#endif

#define QE_STATIC_INLINE static QEINLINE

#ifndef STMT_START
#   define STMT_START	do
#endif
#ifndef STMT_END
#   define STMT_END	while (0)
#endif

#endif
