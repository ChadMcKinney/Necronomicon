/* Copyright (C) Chad McKinney and Curtis McKinney - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */

#ifndef DRIVER_H
#define DRIVER_H 1

#include <stdlib.h>
#include <stdbool.h>

typedef enum
{
    NECRO_TEST_ALL,
    NECRO_TEST_VM,
    NECRO_TEST_DVM,
    NECRO_TEST_SYMTABLE,
    NECRO_TEST_SLAB,
    NECRO_TEST_TREADMILL,
    NECRO_TEST_LEXER,
    NECRO_TEST_INTERN,
    NECRO_TEST_VAULT,
    NECRO_TEST_ARCHIVE,
    NECRO_TEST_REGION,
} NECRO_TEST;

typedef enum
{
    NECRO_BENCH_ALL,
    NECRO_BENCH_ARCHIVE
} NECRO_BENCH;

typedef enum
{
    NECRO_PHASE_NONE,
    NECRO_PHASE_ALL,
    NECRO_PHASE_LEX_PRE_LAYOUT,
    NECRO_PHASE_LEX,
    NECRO_PHASE_PARSE,
    NECRO_PHASE_REIFY,
    NECRO_PHASE_BUILD_SCOPES,
    NECRO_PHASE_RENAME,
} NECRO_PHASE;

void necro_test(NECRO_TEST test);
void necro_compile(const char* input_string, NECRO_PHASE compilation_phase);

#endif // DRIVER_H