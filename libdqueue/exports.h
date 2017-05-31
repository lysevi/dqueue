
#ifndef EXPORT_H
#define EXPORT_H

#ifdef SHARED_EXPORTS_BUILT_AS_STATIC
#  define EXPORT
#  define DQUEUE_NO_EXPORT
#else
#  ifndef EXPORT
#    ifdef dqueue_EXPORTS
        /* We are building this library */
#      define EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef DQUEUE_NO_EXPORT
#    define DQUEUE_NO_EXPORT 
#  endif
#endif

#ifndef DQUEUE_DEPRECATED
#  define DQUEUE_DEPRECATED __declspec(deprecated)
#endif

#ifndef DQUEUE_DEPRECATED_EXPORT
#  define DQUEUE_DEPRECATED_EXPORT EXPORT DQUEUE_DEPRECATED
#endif

#ifndef DQUEUE_DEPRECATED_NO_EXPORT
#  define DQUEUE_DEPRECATED_NO_EXPORT DQUEUE_NO_EXPORT DQUEUE_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef DQUEUE_NO_DEPRECATED
#    define DQUEUE_NO_DEPRECATED
#  endif
#endif

#endif
