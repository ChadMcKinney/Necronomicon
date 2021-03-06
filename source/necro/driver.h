/* Copyright (C) Chad McKinney and Curtis McKinney - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */

#ifndef NECRO_DRIVER_H
#define NECRO_DRIVER_H 1

#include <stdlib.h>
#include <stdbool.h>
#include "utility/result.h"

typedef enum
{
    NECRO_TEST_ALL,
    NECRO_TEST_LEXER,
    NECRO_TEST_INTERN,
    NECRO_TEST_PARSER,
    NECRO_TEST_RENAME,
    NECRO_TEST_INFER,
    NECRO_TEST_TYPE,
    NECRO_TEST_MONOMORPHIZE,
    NECRO_TEST_ALIAS,
    NECRO_TEST_CORE,
    NECRO_TEST_PRE_SIMPLIFY,
    NECRO_TEST_LAMBDA_LIFT,
    NECRO_TEST_DEFUNCTIONALIZE,
    NECRO_TEST_STATE_ANALYSIS,
    NECRO_TEST_MACH,
    NECRO_TEST_LLVM,
    NECRO_TEST_JIT,
    NECRO_TEST_COMPILE,
    NECRO_TEST_ARENA_CHAIN_TABLE,
    NECRO_TEST_UNICODE,
    NECRO_TEST_BASE,
} NECRO_TEST;

typedef enum
{
    NECRO_BENCH_ALL,
    NECRO_BENCH_ARCHIVE
} NECRO_BENCH;

typedef enum
{
    NECRO_PHASE_NONE                  = 0,
    NECRO_PHASE_ALL                   = 2,
    NECRO_PHASE_LEX                   = 4,
    NECRO_PHASE_PARSE                 = 8,
    NECRO_PHASE_REIFY                 = 16,
    NECRO_PHASE_BUILD_SCOPES          = 32,
    NECRO_PHASE_RENAME                = 64,
    NECRO_PHASE_DEPENDENCY_ANALYSIS   = 128,
    NECRO_PHASE_INFER                 = 256,
    NECRO_PHASE_MONOMORPHIZE          = 512,
    NECRO_PHASE_TRANSFORM_TO_CORE     = 1024,
    NECRO_PHASE_PRE_SIMPLIFY          = 2048,
    NECRO_PHASE_LAMBDA_LIFT           = 4096,
    NECRO_PHASE_DEFUNCTIONALIZATION   = 8192,
    NECRO_PHASE_STATE_ANALYSIS        = 16384,
    NECRO_PHASE_TRANSFORM_TO_MACHINE  = 32768,
    NECRO_PHASE_CODEGEN               = 65536,
    NECRO_PHASE_JIT                   = 131072,
    NECRO_PHASE_COMPILE               = 262144
} NECRO_PHASE;

typedef enum
{
    NECRO_OPT_OFF = 0,
    NECRO_OPT_ON  = 1
} NECRO_OPT_LEVEL;

struct NecroTimer;
typedef struct
{
    size_t             verbosity;
    struct NecroTimer* timer;
    NECRO_PHASE        compilation_phase;
    NECRO_OPT_LEVEL    opt_level;
} NecroCompileInfo;

static inline NecroCompileInfo necro_test_compile_info()
{
    return (NecroCompileInfo)
    {
        .verbosity         = 0,
        .timer             = NULL,
        .compilation_phase = NECRO_PHASE_JIT,
        .opt_level         = 0,
    };
}

void necro_test(NECRO_TEST test);
void necro_compile(const char* file_name, const char* input_string, size_t input_string_length, NECRO_PHASE compilation_phase, NECRO_OPT_LEVEL opt_level);

#endif // NECRO_DRIVER_H
