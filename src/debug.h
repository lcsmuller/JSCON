#ifndef DEBUG_H_
#define DEBUG_H_


#if DEBUG_MODE == 1 /* DEBUG MODE ACTIVE */
#       define DEBUG_OUT stderr
#       define DEBUG_FMT_PREFIX "[%s:%d] %s()\n\t"
#       define DEBUG_FMT_ARGS __FILE__, __LINE__, __func__
/* @param msg string to be printed in debug mode */
#       define DEBUG_PUTS(msg) fprintf(DEBUG_OUT, DEBUG_FMT_PREFIX "%s\n", DEBUG_FMT_ARGS, msg)
#       define DEBUG_NOTOP_PUTS(msg) fprintf(DEBUG_OUT, "\t%s\n", msg)
/* @param fmt like printf
   @param ... arguments to be parsed into fmt */
#       define __DEBUG_PRINT(fmt, ...) fprintf(DEBUG_OUT, DEBUG_FMT_PREFIX fmt"\n%s", DEBUG_FMT_ARGS, __VA_ARGS__)
#       define DEBUG_PRINT(...) __DEBUG_PRINT(__VA_ARGS__, "")
#       define __DEBUG_NOTOP_PRINT(fmt, ...) fprintf(DEBUG_OUT, "\t"fmt"\n%s", __VA_ARGS__)
#       define DEBUG_NOTOP_PRINT(...) __DEBUG_NOTOP_PRINT(__VA_ARGS__, "")
#       define __DEBUG_ERR(fmt, ...) fprintf(DEBUG_OUT, DEBUG_FMT_PREFIX "ERROR:\t"fmt"\n%s", DEBUG_FMT_ARGS, __VA_ARGS__)
#       define DEBUG_ERR(...) \
        do { \
            __DEBUG_ERR(__VA_ARGS__, ""); \
            abort(); \
        } while (0)
/* @param expr to be checked for its validity
   @param msg to be printed in case of invalidity */
#       define DEBUG_ASSERT(expr, msg) \
        do { \
            if (!(expr)){ \
                DEBUG_ERR("Assert Failed:\t%s\n\tExpected:\t%s", msg, #expr); \
            } \
        } while(0)
#       define DEBUG_ONLY_ASSERT(expr, msg) DEBUG_ASSERT(expr, msg)
/* @param snippet to be executed if debug mode is active */
#       define DEBUG_ONLY(arg) (arg)
#else /* DEBUG MODE INNACTIVE */
#       define DEBUG_PUTS(msg) 
#       define DEBUG_NOTOP_PUTS(msg) 
#       define DEBUG_PRINT(...)
#       define DEBUG_NOTOP_PRINT(...)
#       define DEBUG_ERR(...)
/* DEBUG_ASSERT becomes a proxy for assert */
#       include <assert.h>
#       define DEBUG_ASSERT(expr, msg) assert(expr)
#       define DEBUG_ONLY_ASSERT(expr, msg)
#       define DEBUG_ONLY(arg)
#endif

#endif
