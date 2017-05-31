
#ifndef EXPORT_H
#define EXPORT_H

#ifdef SHARED_EXPORTS_BUILT_AS_STATIC
#  define EXPORT
#  define DEQUEUE_NO_EXPORT
#else
#  ifndef EXPORT
#    ifdef dequeue_EXPORTS
        /* We are building this library */
#      define EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef DEQUEUE_NO_EXPORT
#    define DEQUEUE_NO_EXPORT 
#  endif
#endif

#ifndef DEQUEUE_DEPRECATED
#  define DEQUEUE_DEPRECATED __declspec(deprecated)
#endif

#ifndef DEQUEUE_DEPRECATED_EXPORT
#  define DEQUEUE_DEPRECATED_EXPORT EXPORT DEQUEUE_DEPRECATED
#endif

#ifndef DEQUEUE_DEPRECATED_NO_EXPORT
#  define DEQUEUE_DEPRECATED_NO_EXPORT DEQUEUE_NO_EXPORT DEQUEUE_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef DEQUEUE_NO_DEPRECATED
#    define DEQUEUE_NO_DEPRECATED
#  endif
#endif

#endif
