#ifndef _LEVEL_DB_XY_THREAD_ANNOTATION_H_
#define _LEVEL_DB_XY_THREAD_ANNOTATION_H_

#if !defined(THREAD_ANNOTATION_ATTRIBUTE__)

#if defined(__clang__)
#define THREAD_ANNOTATION_ATTRIBUTE__(x) __attribute__((x))
#else
#define THREAD_ANNOTATION_ATTRIBUTE__(x) // no-op
#endif                                   // defined(__clang__)

#endif // !defined(THREAD_ANNOTATION_ATTRIBUTE__)

#ifndef GUARDED_BY
#define GUARDED_BY(x) THREAD_ANNOTATION_ATTRIBUTE__(guarded_by(x))
#endif // GUARDED_BY

#ifndef EXCLUSIVE_LOCKS_REQUIRED
#define EXCLUSIVE_LOCKS_REQUIRED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(exclusive_locks_required(__VA_ARGS__))
#endif // EXCLUSIVE_LOCKS_REQUIRED

#endif