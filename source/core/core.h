/* Copyright (C) Chad McKinney and Curtis McKinney - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */

#ifndef CORE_H
#define CORE_H 1

#include <stdio.h>

#include "ast.h"
#include "infer.h"
#include "parser.h"
#include "prim.h"

#if 0
//////////////////////
// Types
//////////////////////

typedef struct
{
    struct NecroCoreAST_Type* typeA;
    struct NecroCoreAST_Type* typeB;
} NecroCoreAST_AppType;

typedef struct
{
    NecroVar id;
} NecroCoreAST_TypeCon;

typedef struct
{
    NecroCoreAST_TypeCon* typeCon;
    struct NecroCoreAST_TypeList* types;
} NecroCoreAST_TyConApp;

typedef struct
{
    union
    {
        NecroVar tyvarTy;
        NecroAST_ConstantType liftTy;
        NecroCoreAST_AppType* appTy;
        NecroCoreAST_TyConApp* tyConApp;
    };

    enum
    {
        NECRO_CORE_AST_TYPE_VAR,
        NECRO_CORE_AST_TYPE_LIT,
        NECRO_CORE_AST_TYPE_APP,
        NECRO_CORE_AST_TYPE_TYCON_APP,
    } type;
} NecroCoreAST_Type;

typedef struct
{
    NecroCoreAST_Type* type;
    NecroCoreAST_Type* next;
} NecroCoreAST_TypeList;
#endif

//////////////////////
// Expressions
//////////////////////
struct NecroCoreAST_Expression;

typedef struct NecroCoreAST_Bind
{
    NecroVar var;
    struct NecroCoreAST_Expression* expr;
    bool                            is_recursive; // Curtis: Metadata for codegen
} NecroCoreAST_Bind;

typedef struct NecroCoreAST_Application
{
    struct NecroCoreAST_Expression* exprA;
    struct NecroCoreAST_Expression* exprB;
    uint32_t                        persistent_slot; // Curtis: Metadata for codegen
} NecroCoreAST_Application;

typedef struct NecroCoreAST_Lambda
{
    struct NecroCoreAST_Expression* arg;
    struct NecroCoreAST_Expression* expr;
} NecroCoreAST_Lambda;

typedef struct NecroCoreAST_Let
{
    struct NecroCoreAST_Expression* bind;
    struct NecroCoreAST_Expression* expr;
} NecroCoreAST_Let;

typedef struct NecroCoreAST_DataCon
{
    NecroVar condid;
    struct NecroCoreAST_Expression* arg_list;
    struct NecroCoreAST_DataCon* next;
} NecroCoreAST_DataCon;

typedef struct NecroCoreAST_DataDecl
{
    NecroVar data_id;
    struct NecroCoreAST_DataCon* con_list;
} NecroCoreAST_DataDecl;

typedef struct NecroCoreAST_CaseAlt
{
    struct NecroCoreAST_Expression* altCon; // NULL altCon is a wild card
    struct NecroCoreAST_Expression* expr;
    struct NecroCoreAST_CaseAlt* next;
} NecroCoreAST_CaseAlt;

typedef struct
{
    struct NecroCoreAST_Expression* expr;
    //NecroVar var; // Don't know what to do with var and type atm
    //struct NecroCoreAST_Type* type;
    NecroCoreAST_CaseAlt* alts;
} NecroCoreAST_Case;

typedef struct
{
    NecroType type;
} NecroCoreAST_Type;

typedef struct
{
    struct NecroCoreAST_Expression* expr;
    struct NecroCoreAST_Expression* next;
} NecroCoreAST_List;

typedef struct NecroCoreAST_Expression
{
    union
    {
        NecroVar var;
        NecroCoreAST_Bind bind;
        NecroAST_Constant_Reified lit;
        NecroCoreAST_Application app;
        NecroCoreAST_Lambda lambda;
        NecroCoreAST_Let let;
        NecroCoreAST_Case case_expr;
        NecroCoreAST_Type type;
        NecroCoreAST_List list;
        NecroCoreAST_DataDecl data_decl;
        NecroCoreAST_DataCon data_con;
    };

    enum
    {
        NECRO_CORE_EXPR_VAR,
        NECRO_CORE_EXPR_BIND,
        NECRO_CORE_EXPR_LIT,
        NECRO_CORE_EXPR_APP,
        NECRO_CORE_EXPR_LAM,
        NECRO_CORE_EXPR_LET,
        NECRO_CORE_EXPR_CASE,
        NECRO_CORE_EXPR_TYPE,
        NECRO_CORE_EXPR_LIST, // used for top decls not language lists
        NECRO_CORE_EXPR_DATA_DECL,
        NECRO_CORE_EXPR_DATA_CON,
        NECRO_CORE_EXPR_COUNT,
        NECRO_CORE_EXPR_UNIMPLEMENTED,
    } expr_type;
} NecroCoreAST_Expression;

static const char* core_ast_names[] =
{
    "NECRO_CORE_EXPR_VAR",
    "NECRO_CORE_EXPR_BIND",
    "NECRO_CORE_EXPR_LIT",
    "NECRO_CORE_EXPR_APP",
    "NECRO_CORE_EXPR_LAM",
    "NECRO_CORE_EXPR_LET",
    "NECRO_CORE_EXPR_CASE",
    "NECRO_CORE_EXPR_TYPE",
    "NECRO_CORE_EXPR_LIST",
    "NECRO_CORE_EXPR_DATA_DECL",
    "NECRO_CORE_EXPR_DATA_CON",
    "NECRO_CORE_EXPR_COUNT",
    "NECRO_CORE_EXPR_UNIMPLEMENTED"
};

typedef enum
{
    NECRO_CORE_TRANSFORMING,
    NECRO_CORE_TRANSFORM_ERROR,
    NECRO_CORE_TRANSFORM_DONE
} NecroParse_CoreTransformState;

typedef struct NecroCoreAST
{
    NecroPagedArena arena;
    NecroCoreAST_Expression* root;
} NecroCoreAST;

typedef struct
{
    NecroCoreAST* core_ast;
    NecroAST_Reified* necro_ast;
    NecroIntern* intern;
    NecroPrimTypes* prim_types;
    NecroParse_CoreTransformState transform_state;
    NecroError error;
} NecroTransformToCore;

void necro_transform_to_core(NecroTransformToCore* core_transform);
void necro_print_core(NecroCoreAST* ast, NecroIntern* intern);

static inline void necro_construct_core_transform(
    NecroTransformToCore* core_transform,
    NecroCoreAST* core_ast,
    NecroAST_Reified* necro_ast,
    NecroIntern* intern,
    NecroPrimTypes* prim_types)
{
    core_transform->error.error_message[0] = '\0';
    core_transform->error.return_code = NECRO_SUCCESS;
    core_transform->core_ast = core_ast;
    core_transform->core_ast->arena = necro_create_paged_arena();
    core_transform->necro_ast = necro_ast;
    core_transform->intern = intern;
    core_transform->prim_types = prim_types;
    core_transform->transform_state = NECRO_CORE_TRANSFORMING;
}

static inline void necro_destruct_core_transform(NecroTransformToCore* core_transform)
{
    necro_destroy_paged_arena(&core_transform->core_ast->arena);
    core_transform->core_ast = NULL;
    core_transform->necro_ast = NULL;
    core_transform->intern = NULL;
    core_transform->transform_state = NECRO_CORE_TRANSFORM_DONE;
}

#endif // CORE_H
