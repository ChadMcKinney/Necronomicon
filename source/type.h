/* Copyright (C) Chad McKinney and Curtis McKinney - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */

#ifndef TYPE_H
#define TYPE_H 1

#include <stdlib.h>
#include <stdbool.h>
#include "utility.h"
#include "intern.h"
#include "ast.h"

//=====================================================
//  Var / Con
//=====================================================
typedef struct
{
    NecroID            id;
    struct NecroScope* scope;
    // NecroSymbol symbol;
} NecroVar;

typedef struct
{
    NecroSymbol symbol;
    NecroID     id;
} NecroCon;

//=====================================================
// NecorType
//=====================================================
struct NecroType;
typedef enum
{
    NECRO_TYPE_VAR,
    NECRO_TYPE_APP,
    NECRO_TYPE_CON,
    NECRO_TYPE_FUN,
    NECRO_TYPE_LIST,
    NECRO_TYPE_LIT,
    NECRO_TYPE_FOR,
} NECRO_TYPE;

typedef struct
{
    NecroVar var;
} NecroTypeVar;

typedef struct
{
    struct NecroType* type1;
    struct NecroType* type2;
} NecroTypeApp;

typedef struct
{
    NecroCon          con;
    struct NecroType* args;
    size_t            arity;
} NecroTypeCon;

typedef struct
{
    struct NecroType* type1;
    struct NecroType* type2;
} NecroTypeFun;

typedef struct
{
    struct NecroType* item;
    struct NecroType* next;
} NecroTypeList;

typedef struct
{
    NecroVar          var;
    struct NecroType* type;
} NecroTypeForAll;

typedef struct NecroType
{
    union
    {
        NecroTypeVar    var;
        NecroTypeApp    app;
        NecroTypeCon    con;
        NecroTypeFun    fun;
        NecroTypeList   list;
        NecroTypeForAll for_all;
    };
    NECRO_TYPE     type;
    NecroSourceLoc source_loc;
} NecroType;

typedef struct
{
    NecroSymbol two;
    NecroSymbol three;
    NecroSymbol four;
    NecroSymbol five;
    NecroSymbol six;
    NecroSymbol seven;
    NecroSymbol eight;
    NecroSymbol nine;
    NecroSymbol ten;
} NecroTupleSymbols;

typedef struct
{
    NecroType*        int_type;
    NecroType*        int_bin_op_type;
    NecroType*        int_compare_type;
    NecroType*        float_type;
    NecroType*        float_bin_op_type;
    NecroType*        float_compare_type;
    NecroType*        audio_type;
    NecroType*        audio_bin_op_type;
    NecroType*        audio_compare_type;
    NecroType*        char_type;
    NecroType*        char_compare_type;
    NecroType*        bool_type;
    NecroType*        bool_compare_type;
    // NecroType* string_type;
    //-----------------------------------
    // Symbols for constructing types
    NecroSymbol       list_symbol;
    NecroTupleSymbols tuple_symbols;
} NecroPrimTypes;

//=====================================================
// NecroTypeEnv
//=====================================================
typedef struct
{
    NecroType** data;
    size_t      capacity;
} NecroTypeEnv;

//=====================================================
// Infer
//=====================================================
struct NecroSymTable;
typedef struct
{
    // NecroPrimSymbols prim_symbols;
    struct NecroSymTable* symtable;
    NecroPrimTypes        prim_types;
    NecroTypeEnv          env;
    NecroPagedArena       arena;
    NecroIntern*          intern;
    NecroError            error;
    size_t                highest_id;
} NecroInfer;

//=====================================================
// API
//=====================================================
NecroInfer       necro_create_infer(NecroIntern* intern, struct NecroSymTable* symtable);
void             necro_destroy_infer(NecroInfer* infer);
void             necro_reset_infer(NecroInfer* infer);
bool             necro_is_infer_error(NecroInfer* infer);

struct NecroScope;
// void             necro_unify(NecroInfer* infer, NecroType* type1, NecroType* type2);
void             necro_unify(NecroInfer* infer, NecroType* type1, NecroType* type2, struct NecroScope* scope);
NecroType*       necro_inst(NecroInfer* infer, NecroType* poly_type, struct NecroScope* scope);
// NecroType*       necro_gen(NecroInfer* infer, NecroType* type);
NecroType*       necro_gen(NecroInfer* infer, NecroType* type, struct NecroScope* scope);
NecroType*       necro_new_name(NecroInfer* infer);
// NecroType*       necro_abs(NecroInfer* infer, NecroType* arg_type, NecroType* result_type);
NecroType*       necro_most_specialized(NecroInfer* infer, NecroType* type);
NecroType*       necro_find(NecroInfer* infer, NecroType* type);

NecroType*       necro_create_type_con(NecroInfer* infer, NecroCon con, NecroType* args, size_t arity);
NecroType*       necro_create_type_fun(NecroInfer* infer, NecroType* type1, NecroType* type2);
NecroType*       necro_create_type_var(NecroInfer* infer, NecroVar var);
NecroType*       necro_create_type_app(NecroInfer* infer, NecroType* type1, NecroType* type2);

NecroType*       necro_get_bin_op_type(NecroInfer* infer, NecroAST_BinOpType bin_op_type);
NecroType*       necro_make_con_1(NecroInfer* infer, NecroSymbol con_symbol, NecroType* arg1);
NecroType*       necro_make_con_2(NecroInfer* infer, NecroSymbol con_symbol, NecroType* arg1, NecroType* arg2);
NecroType*       necro_make_con_3(NecroInfer* infer, NecroSymbol con_symbol, NecroType* arg1, NecroType* arg2, NecroType* arg3);
NecroType*       necro_make_con_4(NecroInfer* infer, NecroSymbol con_symbol, NecroType* arg1, NecroType* arg2, NecroType* arg3, NecroType* arg4);
NecroType*       necro_make_con_5(NecroInfer* infer, NecroSymbol con_symbol, NecroType* arg1, NecroType* arg2, NecroType* arg3, NecroType* arg4, NecroType* arg5);
NecroType*       necro_make_con_6(NecroInfer* infer, NecroSymbol con_symbol, NecroType* arg1, NecroType* arg2, NecroType* arg3, NecroType* arg4, NecroType* arg5, NecroType* arg6);
NecroType*       necro_make_con_7(NecroInfer* infer, NecroSymbol con_symbol, NecroType* arg1, NecroType* arg2, NecroType* arg3, NecroType* arg4, NecroType* arg5, NecroType* arg6, NecroType* arg7);
NecroType*       necro_make_con_8(NecroInfer* infer, NecroSymbol con_symbol, NecroType* arg1, NecroType* arg2, NecroType* arg3, NecroType* arg4, NecroType* arg5, NecroType* arg6, NecroType* arg7, NecroType* arg8);
NecroType*       necro_make_con_9(NecroInfer* infer, NecroSymbol con_symbol, NecroType* arg1, NecroType* arg2, NecroType* arg3, NecroType* arg4, NecroType* arg5, NecroType* arg6, NecroType* arg7, NecroType* arg8, NecroType* arg9);
NecroType*       necro_make_con_10(NecroInfer* infer, NecroSymbol con_symbol, NecroType* arg1, NecroType* arg2, NecroType* arg3, NecroType* arg4, NecroType* arg5, NecroType* arg6, NecroType* arg7, NecroType* arg8, NecroType* arg9, NecroType* arg10);

NecroType*       necro_env_get(NecroInfer* infer, NecroVar var);
void             necro_env_set(NecroInfer* infer, NecroVar var, NecroType* type);

NecroType*       necro_rename_var_for_testing_only(NecroInfer* infer, NecroVar var_to_replace, NecroType* replace_var_with, NecroType* type);
void             necro_print_type_sig(NecroType* type, NecroIntern* intern);
char*            necro_snprintf_type_sig(NecroType* type, NecroIntern* intern, char* buffer, const size_t buffer_length);
const char*      necro_id_as_character_string(NecroInfer* infer, NecroID id);
bool             necro_check_and_print_type_error(NecroInfer* infer);
void             necro_check_type_sanity(NecroInfer* infer, NecroType* type);
void             necro_print_type_test_result(const char* test_name, NecroType* type, const char* test_name2, NecroType* type2, NecroIntern* intern);
void             necro_print_env(NecroInfer* infer);
void             necro_test_type();

#endif // TYPE_H