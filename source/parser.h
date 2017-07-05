/* Copyright (C) Chad McKinney and Curtis McKinney - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */

#ifndef PARSER_H
#define PARSER_H 1

#include <stdlib.h>
#include <stdbool.h>
#include "lexer.h"
#include "arena.h"

// Local offset into AST arena
typedef uint32_t NecroAST_LocalPtr;

typedef enum
{
    NECRO_AST_UNDEFINED,
    NECRO_AST_ADD
} NecroAST_NodeType;

typedef struct
{
    uint8_t _pad;
} NecroAST_Undefined;

typedef struct
{
    NecroAST_LocalPtr lhs;
    NecroAST_LocalPtr rhs;
} NecroAST_Add;

typedef struct
{
    union
    {
        NecroAST_Add add;
    };

    NecroAST_NodeType type;
} NecroAST_Node;

typedef struct
{
    NecroArena arena;
} NecroAST;

static inline NecroAST_Node* ast_get_node(NecroAST* ast, NecroAST_LocalPtr local_ptr)
{
    return ((NecroAST_Node*) ast->arena.region) + local_ptr;
}

static inline NecroAST_Node* ast_get_root_node(NecroAST* ast)
{
    return (NecroAST_Node*) ast->arena.region;
}

typedef enum
{
    parse_successful,
    parse_error
} NecroParse_Result;

NecroParse_Result parse_ast(NecroLexToken** tokens, size_t num_tokens, NecroAST* ast);
bool parse_expression(NecroLexToken** tokens, size_t num_tokens);
void print_ast(NecroAST* ast);

#endif // PARSER_H
