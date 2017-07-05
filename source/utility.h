/* Copyright (C) Chad McKinney and Curtis McKinney - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */

#ifndef UTILITY_H
#define UTILITY_H 1

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

//=====================================================
// A collection of general purpose structs and functions
//=====================================================

//=====================================================
// NecroVector:
//   * A very simple dynamic array, also known as a "vector".
//   * Has separate length and capacity.
//     When an insertion would increase the length past the capacity
//     the data pointer is reallocated to be twice the size
//     This makes amortized insertions O(1),
//     however worst case is obviously worse than this.
//     Thus this should not be used is contexts where absolutely consistent
//     behavior and no allocations is to be expected!
//   * Values are copied upon insertion.
//   * Values will not have free called upon them when the vector is freed,
//     if this is necessary it is up to the user to free the values
//     contained within before freeing the vector.
//=====================================================
#define NECRO_INTIAL_VECTOR_SIZE 512
#define NECRO_DECLARE_VECTOR(type, camel_type, snake_type)                         \
typedef struct                                                                     \
{                                                                                  \
	type*   data;                                                                  \
	int32_t length;                                                                \
	int32_t capacity;                                                              \
} camel_type##Vector;                                                              \
                                                                                   \
static camel_type##Vector necro_create_##snake_type##_vector()                     \
{                                                                                  \
	return (camel_type##Vector)                                                    \
	{                                                                              \
		(type*) malloc(NECRO_INTIAL_VECTOR_SIZE * sizeof(type)),                   \
		0,                                                                         \
		NECRO_INTIAL_VECTOR_SIZE                                                   \
	};                                                                             \
}                                                                                  \
                                                                                   \
static void necro_destroy_##snake_type##_vector(camel_type##Vector* vec)           \
{                                                                                  \
	vec->length   = 0;                                                             \
	vec->capacity = 0;                                                             \
	free(vec->data);                                                               \
	vec->data     = NULL;                                                          \
}                                                                                  \
                                                                                   \
static void necro_push_##snake_type##_vector(camel_type##Vector* vec, type* item)  \
{                                                                                  \
	if (vec->length >= vec->capacity)                                              \
	{                                                                              \
		vec->capacity = vec->capacity * 2;                                         \
		vec->data     = (type*) realloc(vec->data, vec->capacity * sizeof(type));  \
	}                                                                              \
	assert(vec->data != NULL);                                                     \
	vec->data[vec->length] = *item;                                                \
	vec->length++;                                                                 \
}

//=====================================================
// NecroStringSlice:
//     * A "slice" is a particular view into a larger string
//     * Since the view is part of a larger whole, it is not null-terminated
//     * This requires explicitly holding a length value to
//       accompany the data pointer.
//     * This should NOT be freed manually, as it lives for the length
//       of the owning string.
//=====================================================
typedef struct
{
	const char* data;
	size_t      length;
} NecroStringSlice;
// NECRO_DECLARE_VECTOR(NecroStringSlice, NecroStringSlice, string_slice)

char* necro_dup_string_slice(NecroStringSlice slice);

#endif // UTILITY_H