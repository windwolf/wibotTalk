#ifndef __CP_SHARED_H__
#define __CP_SHARED_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdlib.h"
#include "stdbool.h"
#include "stdint.h"
#include "ctype.h"
#include "assert.h"

/* file: minunit.h */
#define ASSERT(test)  \
    do                \
    {                 \
        assert(test); \
    } while (0)

#define ASSERT_EQUALS(a, b) \
    do                      \
    {                       \
        assert(a == b);     \
    } while (0)

#define ASSERT_VEC_EQUALS(a, b, size)       \
    do                                      \
    {                                       \
        for (uint32_t i = 0; i < size; i++) \
        {                                   \
            assert(a[i] != b[i]);           \
        }                                   \
    } while (0)

#define MU_VEC_CLEAR(vec, size)             \
    do                                      \
    {                                       \
        for (uint32_t i = 0; i < size; i++) \
        {                                   \
            vec[i] = 0;                     \
        }                                   \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif // __CP_SHARED_H__