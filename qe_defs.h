#ifndef QE_DEFS_H_
#define QE_DEFS_H_

#define QE_OK 0
#define QE_ERROR_FULL 1
#define QE_ERROR_OOM 42

#if DEBUG
#   define QEINLINE
#else
#   define QEINLINE inline
#endif

#ifndef STMT_START
#   define STMT_START	do
#endif
#ifndef STMT_END
#   define STMT_END	while (0)
#endif

#endif
