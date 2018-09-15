/* Copyright (C) Chad McKinney and Curtis McKinney - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */

#ifndef ARENA_H
#define ARENA_H 1

// A very simple region based allocator. At the moment this doesn't support de-alloc within the arena
// This arena won't invalidate pointers unless arena_allow_realloc is used.

#include <stdlib.h>
#include <stdint.h>

typedef struct
{
    char*  region;
    size_t capacity;
    size_t size;
} NecroArena;

NecroArena necro_arena_empty();
NecroArena necro_arena_create(size_t capacity);
void       necro_arena_destroy(NecroArena* arena);

typedef enum
{
    NECRO_ARENA_FIXED,
    NECRO_ARENA_REALLOC
} NECRO_ARENA_ALLOC_POLICY;

void* necro_arena_alloc(NecroArena* arena, size_t size, NECRO_ARENA_ALLOC_POLICY alloc_policy);

//=====================================================
// NecroPagedArena
//=====================================================
typedef struct NecroArenaPage
{
    struct NecroArenaPage* next;
} NecroArenaPage;

typedef struct
{
    NecroArenaPage* pages;
    char*           data;
    size_t          size;
    size_t          count;
} NecroPagedArena;

NecroPagedArena necro_paged_arena_empty();
NecroPagedArena necro_paged_arena_create();
void            necro_paged_arena_destroy(NecroPagedArena* arena);
void*           necro_paged_arena_alloc(NecroPagedArena* arena, size_t size);

//=====================================================
// NecroSnapshotArena
//=====================================================
// NOTE: Pointers Retrieved from the arena are unstable!
typedef struct
{
    size_t count;
} NecroArenaSnapshot;

typedef struct
{
    char*  data;
    size_t count;
    size_t size;
} NecroSnapshotArena;

NecroSnapshotArena necro_snapshot_arena_empty();
NecroSnapshotArena necro_snapshot_arena_create();
void               necro_snapshot_arena_destroy(NecroSnapshotArena* arena);
void*              necro_snapshot_arena_alloc(NecroSnapshotArena* arena, size_t bytes);
NecroArenaSnapshot necro_snapshot_arena_get(NecroSnapshotArena* arena);
void               necro_snapshot_arena_rewind(NecroSnapshotArena* arena, NecroArenaSnapshot snapshot);
char*              necro_snapshot_arena_concat_strings(NecroSnapshotArena* arena, uint32_t string_count, const char** strings);

#endif // ARENA_H
