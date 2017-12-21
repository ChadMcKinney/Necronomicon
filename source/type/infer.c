/* Copyright (C) Chad McKinney and Curtis McKinney - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */

#include <stdio.h>
#include <inttypes.h>
#include <stdarg.h>
#include "symtable.h"
#include "prim.h"
#include "type_class.h"
#include "infer.h"

#define NECRO_INFER_DEBUG           0
#if NECRO_INFER_DEBUG
#define TRACE_INFER(...) printf(__VA_ARGS__)
#else
#define TRACE_INFER(...)
#endif

// Forward declarations
NecroType* necro_infer_go(NecroInfer* infer, NecroNode* ast);
NecroType* necro_infer_apat(NecroInfer* infer, NecroNode* ast);
NecroType* necro_infer_pattern(NecroInfer* infer, NecroNode* ast);
NecroType* necro_infer_constant(NecroInfer* infer, NecroNode* ast);
NecroType* necro_infer_var(NecroInfer* infer, NecroNode* ast);
NecroType* necro_infer_tuple_type(NecroInfer* infer, NecroNode* ast);

/*
DONE:
    NECRO_AST_UN_OP,
    NECRO_AST_UNDEFINED,
    NECRO_AST_CONSTANT,
    NECRO_AST_BIN_OP,
    NECRO_AST_IF_THEN_ELSE,
    NECRO_AST_SIMPLE_ASSIGNMENT,
    NECRO_AST_RIGHT_HAND_SIDE,
    NECRO_AST_FUNCTION_EXPRESSION,
    NECRO_AST_VARIABLE,
    NECRO_AST_LET_EXPRESSION,
    NECRO_AST_TUPLE,
    NECRO_AST_EXPRESSION_LIST,
    NECRO_AST_APATS_ASSIGNMENT,
    NECRO_AST_WILDCARD,
    NECRO_AST_LIST_NODE,
    NECRO_AST_LAMBDA,
    NECRO_AST_ARITHMETIC_SEQUENCE,
    NECRO_AST_CASE,
    NECRO_AST_CASE_ALTERNATIVE,
    Pattern
    Apat
    NECRO_AST_CONSTRUCTOR,
    NECRO_AST_SIMPLE_TYPE,
    NECRO_AST_DATA_DECLARATION,
    NECRO_AST_TYPE_SIGNATURE,
    NECRO_AST_FUNCTION_TYPE,
    NECRO_AST_CONID
    NECRO_AST_TYPE_APP,
    MULTIPLE assignments with same name!
    NECRO_AST_BIN_OP_SYM,

IN PROGRESS:
    NECRO_AST_TOP_DECL,
    NECRO_AST_DECL,

TODO:

    NECRO_AST_DO,          (Need type classes)
    NECRO_BIND_ASSIGNMENT, (Need type classes)

    NECRO_AST_TYPE_CLASS_CONTEXT,
    NECRO_AST_TYPE_CLASS_DECLARATION,
    NECRO_AST_TYPE_CLASS_INSTANCE,

    Declaration dependency analysis (This is how Haskell infers more general types for recursive declarations)

    // NECRO_AST_MODULE,
*/

//=====================================================
// TypeSig
//=====================================================
NecroType* necro_copy_constructor(NecroInfer* infer, NecroType* con_type)
{
    assert(infer != NULL);
    assert(con_type != NULL);
    assert(con_type->type == NECRO_TYPE_CON);
    return necro_create_type_con(infer, con_type->con.con, NULL, con_type->con.arity);
}

NecroType* necro_ast_to_type_sig_go(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    if (necro_is_infer_error(infer)) return NULL;
    switch (ast->type)
    {
    case NECRO_AST_VARIABLE: return necro_create_type_var(infer, (NecroVar) { ast->variable.id });
    // case NECRO_AST_CONID:         return necro_create_type_con(infer, (NecroCon) { .symbol = ast->conid.symbol, .id = ast->conid.id }, NULL, 0);
    case NECRO_AST_CONID:
        if (necro_symtable_get(infer->symtable, ast->conid.id)->type == NULL)
            return necro_infer_ast_error(infer, NULL, ast, "Can't find data type: %s", necro_intern_get_string(infer->intern, ast->conid.symbol));
        else
            return necro_copy_constructor(infer, necro_symtable_get(infer->symtable, ast->conid.id)->type);
        // return necro_create_type_con(infer, (NecroCon) { .symbol = ast->conid.symbol, .id = ast->conid.id }, NULL, 0);
    case NECRO_AST_TUPLE:         return necro_infer_tuple_type(infer, ast);
    case NECRO_AST_FUNCTION_TYPE: return necro_create_type_fun(infer, necro_ast_to_type_sig_go(infer, ast->function_type.type), necro_ast_to_type_sig_go(infer, ast->function_type.next_on_arrow));
    case NECRO_AST_CONSTRUCTOR:
    {
        // NecroType* con_type = necro_create_type_con(infer, (NecroCon) { .symbol = ast->constructor.conid->conid.symbol, .id = ast->constructor.conid->conid.id }, NULL, 0);
        if (necro_symtable_get(infer->symtable, ast->conid.id)->type == NULL)
            return necro_infer_ast_error(infer, NULL, ast, "Can't find data type: %s", necro_intern_get_string(infer->intern, ast->conid.symbol));
        NecroType* con_type = necro_copy_constructor(infer, necro_symtable_get(infer->symtable, ast->conid.id)->type);
        NecroNode* arg_list = ast->constructor.arg_list;
        size_t arity = 0;
        while (arg_list != NULL)
        {
            // con_type->con.arity += 1;
            if (con_type->con.args == NULL)
                con_type->con.args = necro_create_type_list(infer, necro_ast_to_type_sig_go(infer, arg_list->list.item), NULL);
            else
                con_type->con.args->list.next = necro_create_type_list(infer, necro_ast_to_type_sig_go(infer, arg_list->list.item), NULL);
            arg_list = arg_list->list.next_item;
            arity++;
        }
        // if (arity != con_type->con.arity)
        //     return necro_infer_ast_error(infer, con_type, ast, "Mismatched arity for type %s. Expected arity: %d, found arity %d", necro_intern_get_string(infer->intern, con_type->con.con.symbol), con_type->con.arity, arity);
        return con_type;
    }

    case NECRO_AST_TYPE_APP:
    {
        NecroType* left   = necro_ast_to_type_sig_go(infer, ast->type_app.ty);
        NecroType* right  = necro_ast_to_type_sig_go(infer, ast->type_app.next_ty);
        left->source_loc  = ast->source_loc;
        right->source_loc = ast->source_loc;
        if (left->type == NECRO_TYPE_CON)
        {
            if (left->con.args == NULL)
            {
                left->con.args   = necro_create_type_list(infer, right, NULL);
                // left->con.arity += 1;
            }
            else if (left->con.args != NULL)
            {
                NecroType* current_arg = left->con.args;
                while (current_arg->list.next != NULL)
                    current_arg = current_arg->list.next;
                current_arg->list.next = necro_create_type_list(infer, right, NULL);
                // left->con.arity += 1;
            }
            return left;
        }
        // else if (left->type == NECRO_TYPE_VAR)
        // {
        //     NecroType* type = necro_create_type_app(infer, left, right);
        //     necro_unify(infer, type, left, ast->scope);
        //     if (necro_is_infer_error(infer)) return NULL;
        //     return type;
        // }
        else
        {
            return necro_create_type_app(infer, left, right);
        }

    }

    default: return necro_infer_ast_error(infer, NULL, ast, "Unimplemented type signature case: %d", ast->type);
    }
}

NecroType* necro_infer_type_sig(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_TYPE_SIGNATURE);

    NecroType* type_sig = necro_ast_to_type_sig_go(infer, ast->type_signature.type);
    if (necro_is_infer_error(infer)) return NULL;
    necro_check_type_sanity(infer, type_sig);
    if (necro_is_infer_error(infer)) return NULL;

    NecroTypeClassContext* context = necro_union_contexts(infer, necro_ast_to_context(infer, infer->type_class_env, ast->type_signature.context), NULL);
    if (necro_ambiguous_type_class_check(infer, ast->type_signature.var->variable.symbol, context, type_sig)) return NULL;
    necro_add_constraints_to_ty_vars(infer, type_sig, necro_create_ty_var_context_list(infer, NULL, context));

    type_sig = necro_gen(infer, type_sig, ast->type_signature.type->scope);
    if (necro_is_infer_error(infer)) return NULL;

    necro_check_type_sanity(infer, type_sig);
    if (necro_is_infer_error(infer)) return NULL;

    type_sig->pre_supplied = true;
    type_sig->source_loc   = ast->source_loc;

    necro_symtable_get(infer->symtable, ast->type_signature.var->variable.id)->type = type_sig;
    return NULL;
}

//=====================================================
// Data Declaration
//=====================================================
size_t necro_count_ty_vars(NecroNode* ty_vars)
{
    size_t count = 0;
    while (ty_vars != NULL)
    {
        count++;
        ty_vars = ty_vars->list.next_item;
    }
    return count;
}

NecroType* necro_ty_vars_to_args(NecroInfer* infer, NecroNode* ty_vars)
{
    NecroType* type = NULL;
    NecroType* head = NULL;
    while (ty_vars != NULL)
    {
        assert(ty_vars->type == NECRO_AST_LIST_NODE);
        if (type == NULL)
        {
            head = necro_create_type_list(infer, necro_create_type_var(infer, (NecroVar) { .id = ty_vars->list.item->variable.id.id }), NULL);
            type = head;
        }
        else
        {
            type->list.next = necro_create_type_list(infer, necro_create_type_var(infer, (NecroVar) { .id = ty_vars->list.item->variable.id.id }), NULL);
            type = type->list.next;
        }
        ty_vars = ty_vars->list.next_item;
    }
    return head;
}

NecroType* necro_create_data_constructor(NecroInfer* infer, NecroNode* ast, NecroType* data_type)
{
    NecroType* con_head = NULL;
    NecroType* con_args = NULL;
    NecroNode* args_ast = ast->constructor.arg_list;
    size_t     count    = 0;
    while (args_ast != NULL)
    {
        if (con_args == NULL)
        {
            con_args = necro_create_type_fun(infer, necro_ast_to_type_sig_go(infer, args_ast->list.item), NULL);
            con_head = con_args;
        }
        else
        {
            con_args->fun.type2 = necro_create_type_fun(infer, necro_ast_to_type_sig_go(infer, args_ast->list.item), NULL);
            con_args = con_args->fun.type2;
        }
        if (necro_is_infer_error(infer)) return NULL;
        args_ast = args_ast->list.next_item;
        count++;
    }
    NecroType* con_type = NULL;
    if (con_args == NULL)
    {
        con_type = data_type;
    }
    else
    {
        con_args->fun.type2 = data_type;
        con_type            = con_head;
    }
    con_type = necro_gen(infer, con_type, NULL);
    if (necro_is_infer_error(infer)) return NULL;
    con_type->source_loc   = ast->source_loc;
    con_type->pre_supplied = true;
    necro_symtable_get(infer->symtable, ast->constructor.conid->conid.id)->type = con_type;
    return con_type;
}

NecroType* necro_infer_simple_type(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_SIMPLE_TYPE);
    if (necro_is_infer_error(infer)) return NULL;
    NecroType* type = necro_create_type_con(infer, (NecroCon) { .symbol = ast->simple_type.type_con->conid.symbol, .id = ast->simple_type.type_con->conid.id.id }, necro_ty_vars_to_args(infer, ast->simple_type.type_var_list), necro_count_ty_vars(ast->simple_type.type_var_list));
    if (necro_is_infer_error(infer)) return NULL;
    necro_check_type_sanity(infer, type);
    if (necro_is_infer_error(infer)) return NULL;
    type->source_loc   = ast->source_loc;
    type->pre_supplied = true;
    necro_symtable_get(infer->symtable, ast->simple_type.type_con->conid.id)->type = type;
    necro_print_type_sig(type, infer->intern);
    return type;
}

NecroType* necro_infer_data_declaration(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_DATA_DECLARATION);
    if (necro_is_infer_error(infer)) return NULL;
    NecroType* data_type        = necro_infer_simple_type(infer, ast->data_declaration.simpletype);
    if (necro_is_infer_error(infer)) return NULL;
    NecroNode* constructor_list = ast->data_declaration.constructor_list;
    while (constructor_list != NULL)
    {
        necro_create_data_constructor(infer, constructor_list->list.item, data_type);
        if (necro_is_infer_error(infer)) return NULL;
        constructor_list = constructor_list->list.next_item;
    }
    return NULL;
}

//=====================================================
// Constant
//=====================================================
NecroType* necro_infer_constant(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_CONSTANT);
    if (necro_is_infer_error(infer)) return NULL;
    switch (ast->constant.type)
    {
    case NECRO_AST_CONSTANT_FLOAT:   return necro_symtable_get(infer->symtable, infer->prim_types.float_type.id)->type;
    case NECRO_AST_CONSTANT_INTEGER: return necro_symtable_get(infer->symtable, infer->prim_types.int_type.id)->type;
    case NECRO_AST_CONSTANT_BOOL:    return necro_symtable_get(infer->symtable, infer->prim_types.bool_type.id)->type;
    case NECRO_AST_CONSTANT_CHAR:    return necro_symtable_get(infer->symtable, infer->prim_types.char_type.id)->type;
    case NECRO_AST_CONSTANT_STRING:  return necro_infer_ast_error(infer, NULL, ast, "Compiler bug: String not implemented....");
    default:                         return necro_infer_ast_error(infer, NULL, ast, "Compiler bug: Unrecognized constant type: %d", ast->constant.type);
    }
}

//=====================================================
// Variable
//=====================================================
NecroType* necro_infer_var(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_VARIABLE);
    if (necro_is_infer_error(infer)) return NULL;
    NecroType* var_type = infer->symtable->data[ast->variable.id.id].type;
    if (var_type == NULL)
    {
        var_type                = necro_new_name(infer);
        var_type->source_loc    = ast->source_loc;
        var_type->var.var.scope = infer->symtable->data[ast->variable.id.id].scope;
        infer->symtable->data[ast->variable.id.id].type = var_type;
        return var_type;
    }
    else
    {
        return necro_inst(infer, var_type, ast->scope);
    }
}

//=====================================================
// ConID
//=====================================================
NecroType* necro_infer_conid(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_CONID);
    if (necro_is_infer_error(infer)) return NULL;
    NecroType* con_type = infer->symtable->data[ast->variable.id.id].type;
    if (con_type == NULL)
    {
        return necro_infer_ast_error(infer, con_type, ast, "Unknown Constructor");
    }
    else
    {
        return necro_inst(infer, con_type, ast->scope);
    }
}

//=====================================================
// Wildcard
//=====================================================
NecroType* necro_infer_wildcard(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_WILDCARD);
    if (necro_is_infer_error(infer)) return NULL;
    return necro_new_name(infer);
}

//=====================================================
// Tuple
//=====================================================
NecroType* necro_infer_tuple(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_TUPLE);
    if (necro_is_infer_error(infer)) return NULL;
    NecroNode* current_expression = ast->tuple.expressions;
    NecroType* types_head         = NULL;
    NecroType* tuple_types        = NULL;
    while (current_expression != NULL)
    {
        assert(current_expression->type == NECRO_AST_LIST_NODE);
        if (tuple_types == NULL)
        {
            tuple_types = necro_create_type_list(infer, necro_infer_go(infer, current_expression->list.item), NULL);
            types_head  = tuple_types;
        }
        else
        {
            tuple_types->list.next = necro_create_type_list(infer, necro_infer_go(infer, current_expression->list.item), NULL);
            tuple_types = tuple_types->list.next;
        }
        current_expression = current_expression->list.next_item;
    }
    NecroType* tuple = necro_make_tuple_con(infer, types_head);
    tuple->source_loc = ast->source_loc;
    return tuple;
}

NecroType* necro_infer_tuple_pattern(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_TUPLE);
    if (necro_is_infer_error(infer)) return NULL;
    NecroNode* current_expression = ast->tuple.expressions;
    NecroType* types_head         = NULL;
    NecroType* tuple_types        = NULL;
    while (current_expression != NULL)
    {
        assert(current_expression->type == NECRO_AST_LIST_NODE);
        if (tuple_types == NULL)
        {
            tuple_types = necro_create_type_list(infer, necro_infer_pattern(infer, current_expression->list.item), NULL);
            types_head  = tuple_types;
        }
        else
        {
            tuple_types->list.next = necro_create_type_list(infer, necro_infer_pattern(infer, current_expression->list.item), NULL);
            tuple_types = tuple_types->list.next;
        }
        current_expression = current_expression->list.next_item;
    }
    NecroType* tuple = necro_make_tuple_con(infer, types_head);
    tuple->source_loc = ast->source_loc;
    return tuple;
}

NecroType* necro_infer_tuple_type(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_TUPLE);
    if (necro_is_infer_error(infer)) return NULL;
    NecroNode* current_expression = ast->tuple.expressions;
    NecroType* types_head         = NULL;
    NecroType* tuple_types        = NULL;
    while (current_expression != NULL)
    {
        assert(current_expression->type == NECRO_AST_LIST_NODE);
        if (tuple_types == NULL)
        {
            tuple_types = necro_create_type_list(infer, necro_ast_to_type_sig_go(infer, current_expression->list.item), NULL);
            types_head  = tuple_types;
        }
        else
        {
            tuple_types->list.next = necro_create_type_list(infer, necro_ast_to_type_sig_go(infer, current_expression->list.item), NULL);
            tuple_types = tuple_types->list.next;
        }
        current_expression = current_expression->list.next_item;
    }
    NecroType* tuple = necro_make_tuple_con(infer, types_head);
    tuple->source_loc = ast->source_loc;
    return tuple;
}

//=====================================================
// Expression List
//=====================================================
NecroType* necro_infer_expression_list(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_EXPRESSION_LIST);
    if (necro_is_infer_error(infer)) return NULL;
    NecroNode* current_cell = ast->expression_list.expressions;
    NecroType* list_type    = necro_new_name(infer);
    list_type->source_loc   = ast->source_loc;
    while (current_cell != NULL)
    {
        necro_unify(infer, list_type, necro_infer_go(infer, current_cell->list.item), ast->scope);
        if (necro_is_infer_error(infer)) return NULL;
        current_cell = current_cell->list.next_item;
    }
    NecroType* list = necro_make_con_1(infer, infer->prim_types.list_type.symbol, list_type);
    list->source_loc = ast->source_loc;
    return list;
}

NecroType* necro_infer_expression_list_pattern(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_EXPRESSION_LIST);
    if (necro_is_infer_error(infer)) return NULL;
    NecroNode* current_cell = ast->expression_list.expressions;
    NecroType* list_type    = necro_new_name(infer);
    while (current_cell != NULL)
    {
        necro_unify(infer, list_type, necro_infer_pattern(infer, current_cell->list.item), ast->scope);
        if (necro_is_infer_error(infer)) return NULL;
        current_cell = current_cell->list.next_item;
    }
    NecroType* list = necro_make_con_1(infer, infer->prim_types.list_type.symbol, list_type);
    list->source_loc = ast->source_loc;
    return list;
}

//=====================================================
// Function Expression
//=====================================================
NecroType* necro_infer_fexpr(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_FUNCTION_EXPRESSION);
    if (necro_is_infer_error(infer)) return NULL;
    NecroType* e0_type     = necro_infer_go(infer, ast->fexpression.aexp);
    NecroType* e1_type     = necro_infer_go(infer, ast->fexpression.next_fexpression);
    if (necro_is_infer_error(infer)) return NULL;
    NecroType* result_type  = necro_new_name(infer);
    result_type->source_loc = ast->source_loc;
    NecroType* f_type       = necro_create_type_fun(infer, e1_type, result_type);
    f_type->source_loc      = ast->source_loc;
    necro_unify(infer, e0_type, f_type, ast->scope);
    return result_type;
}

//=====================================================
// If then else
//=====================================================
NecroType* necro_infer_if_then_else(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_IF_THEN_ELSE);
    if (necro_is_infer_error(infer)) return NULL;
    NecroType* if_type   = necro_infer_go(infer, ast->if_then_else.if_expr);
    NecroType* then_type = necro_infer_go(infer, ast->if_then_else.then_expr);
    NecroType* else_type = necro_infer_go(infer, ast->if_then_else.else_expr);
    if (necro_is_infer_error(infer)) return NULL;
    necro_unify(infer, if_type, necro_symtable_get(infer->symtable, infer->prim_types.bool_type.id)->type, ast->scope);
    necro_unify(infer, then_type, else_type, ast->scope);
    if (necro_is_infer_error(infer)) return NULL;
    return then_type;
}

//=====================================================
// BinOp
//=====================================================
NecroType* necro_infer_bin_op(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_BIN_OP);
    if (necro_is_infer_error(infer)) return NULL;
    NecroType* x_type       = necro_infer_go(infer, ast->bin_op.lhs);
    NecroType* op_type      = necro_get_bin_op_type(infer, ast->bin_op.type);
    NecroType* y_type       = necro_infer_go(infer, ast->bin_op.rhs);
    if (necro_is_infer_error(infer)) return NULL;
    NecroType* result_type  = necro_new_name(infer);
    result_type->source_loc = ast->source_loc;
    NecroType* bin_op_type  = necro_create_type_fun(infer, x_type, necro_create_type_fun(infer, y_type, result_type));
    necro_unify(infer, op_type, bin_op_type, ast->scope);
    if (necro_is_infer_error(infer)) return NULL;
    return result_type;
}

//=====================================================
// Apat
//=====================================================
NecroType* necro_infer_apat(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    if (necro_is_infer_error(infer)) return NULL;
    switch (ast->type)
    {
    case NECRO_AST_CONSTANT:        return necro_infer_constant(infer, ast);
    case NECRO_AST_VARIABLE:        return necro_infer_var(infer, ast);
    case NECRO_AST_CONID:           return necro_infer_conid(infer, ast);
    case NECRO_AST_TUPLE:           return necro_infer_tuple_pattern(infer, ast);
    case NECRO_AST_EXPRESSION_LIST: return necro_infer_expression_list_pattern(infer, ast);
    case NECRO_AST_WILDCARD:        return necro_new_name(infer);

    case NECRO_AST_BIN_OP_SYM:
    {
        assert(ast->bin_op_sym.op->type == NECRO_AST_CONID);
        NecroType* constructor_type = necro_symtable_get(infer->symtable, ast->bin_op_sym.op->conid.id)->type;
        if (constructor_type == NULL)
            return necro_infer_ast_error(infer, NULL, ast, "Compiler bug: Constructor \'%s\' has not type", necro_intern_get_string(infer->intern, ast->bin_op_sym.op->conid.symbol));
        constructor_type      = necro_inst(infer, constructor_type, NULL);
        if (necro_is_infer_error(infer)) return NULL;
        NecroType* left_type  = necro_infer_apat(infer, ast->bin_op_sym.left);
        NecroType* right_type = necro_infer_apat(infer, ast->bin_op_sym.right);
        NecroType* data_type  = necro_new_name(infer);
        data_type->source_loc = ast->source_loc;
        NecroType* f_type     = necro_create_type_fun(infer, left_type, necro_create_type_fun(infer, right_type, data_type));
        if (necro_is_infer_error(infer)) return NULL;
        necro_unify(infer, constructor_type, f_type, ast->scope);
        if (necro_is_infer_error(infer)) return NULL;
        necro_check_type_sanity(infer, data_type);
        return data_type;
    }

    case NECRO_AST_CONSTRUCTOR:
    {
        NecroType* constructor_type  = necro_symtable_get(infer->symtable, ast->constructor.conid->conid.id)->type;
        if (constructor_type == NULL)
            return necro_infer_ast_error(infer, NULL, ast, "Compiler bug: Constructor \'%s\' has not type", necro_intern_get_string(infer->intern, ast->constructor.conid->conid.symbol));
        constructor_type             = necro_inst(infer, constructor_type, NULL);
        if (necro_is_infer_error(infer)) return NULL;
        NecroType* pattern_type_head = NULL;
        NecroType* pattern_type      = NULL;
        NecroNode* ast_args          = ast->constructor.arg_list;
        size_t arg_count = 0;
        while (ast_args != NULL)
        {
            if (pattern_type_head == NULL)
            {
                pattern_type_head = necro_create_type_fun(infer, necro_infer_apat(infer, ast_args->list.item), NULL);
                pattern_type      = pattern_type_head;
            }
            else
            {
                pattern_type->fun.type2 = necro_create_type_fun(infer, necro_infer_apat(infer, ast_args->list.item), NULL);
                pattern_type            = pattern_type->fun.type2;
            }
            if (necro_is_infer_error(infer)) return NULL;
            arg_count++;
            ast_args = ast_args->list.next_item;
        }

        NecroType* data_type = necro_new_name(infer);
        data_type->source_loc = ast->source_loc;
        if (pattern_type_head == NULL)
        {
            pattern_type_head = data_type;
        }
        else
        {
            pattern_type->fun.type2 = data_type;
        }

        necro_check_type_sanity(infer, pattern_type_head);
        if (necro_is_infer_error(infer)) return NULL;

        size_t constructor_args = 0;
        NecroType* con_iter = constructor_type;
        while (con_iter->type == NECRO_TYPE_FUN)
        {
            constructor_args++;
            con_iter = con_iter->fun.type2;
        }
        if (arg_count != constructor_args)
            necro_infer_ast_error(infer, pattern_type_head, ast, "Wrong number of arguments for constructor %s. Expected arity: %d, found arity %d", necro_intern_get_string(infer->intern, ast->constructor.conid->conid.symbol), constructor_args, arg_count);

        necro_unify(infer, constructor_type, pattern_type_head, ast->scope);
        necro_check_type_sanity(infer, data_type);
        if (necro_is_infer_error(infer)) return NULL;
        return data_type;
    }

    default: return necro_infer_ast_error(infer, NULL, ast, "Unimplemented pattern in type inference: %d", ast->type);
    }
}

//=====================================================
// Pattern
//=====================================================
NecroType* necro_infer_pattern(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    if (necro_is_infer_error(infer)) return NULL;
    return necro_infer_apat(infer, ast);
}

//=====================================================
// Cast
//=====================================================
NecroType* necro_infer_case(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_CASE);
    if (necro_is_infer_error(infer)) return NULL;
    NecroType* expression_type = necro_infer_go(infer, ast->case_expression.expression);
    NecroType* result_type     = necro_new_name(infer);
    result_type->source_loc    = ast->source_loc;
    NecroNode* alternatives    = ast->case_expression.alternatives;
    while (alternatives != NULL)
    {
        NecroNode* alternative = alternatives->list.item;
        necro_unify(infer, expression_type, necro_infer_pattern(infer, alternative->case_alternative.pat), alternatives->scope);
        necro_unify(infer, result_type, necro_infer_go(infer, alternative->case_alternative.body), alternatives->scope);
        alternatives = alternatives->list.next_item;
    }
    return result_type;
}

//=====================================================
// Apats Assignment
//=====================================================
NecroType* necro_infer_apats_assignment(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_APATS_ASSIGNMENT);
    if (necro_is_infer_error(infer)) return NULL;
    NecroNode* apats      = ast->apats_assignment.apats;
    NecroType* f_type     = NULL;
    NecroType* f_head     = NULL;
    if (necro_is_infer_error(infer)) return NULL;
    while (apats != NULL)
    {
        assert(apats->type == NECRO_AST_APATS);
        if (necro_is_infer_error(infer)) return NULL;
        if (f_type == NULL)
        {
            f_type = necro_create_type_fun(infer, necro_infer_apat(infer, apats->apats.apat), NULL);
            f_head = f_type;
        }
        else
        {
            f_type->fun.type2 = necro_create_type_fun(infer, necro_infer_apat(infer, apats->apats.apat), NULL);
            f_type = f_type->fun.type2;
        }
        apats = apats->apats.next_apat;
    }
    if (necro_is_infer_error(infer)) return NULL;
    NecroType* proxy_type = infer->symtable->data[ast->apats_assignment.id.id].type;
    if (proxy_type->pre_supplied)
    {
        f_type->fun.type2 = necro_new_name(infer);
        f_type->fun.type2->source_loc = ast->source_loc;
        necro_unify(infer, proxy_type, f_head, ast->scope);
    }
    NecroType* rhs = necro_infer_go(infer, ast->apats_assignment.rhs);
    f_type->fun.type2 = rhs;
    necro_unify(infer, proxy_type, f_head, ast->scope);
    return NULL;
}

//=====================================================
// Lambda
//=====================================================
NecroType* necro_infer_lambda(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_LAMBDA);
    if (necro_is_infer_error(infer)) return NULL;
    NecroNode* apats  = ast->lambda.apats;
    NecroType* f_type = NULL;
    NecroType* f_head = NULL;
    while (apats != NULL)
    {
        assert(apats->type == NECRO_AST_APATS);
        if (necro_is_infer_error(infer)) return NULL;
        if (f_type == NULL)
        {
            f_type = necro_create_type_fun(infer, necro_infer_apat(infer, apats->apats.apat), NULL);
            f_head = f_type;
        }
        else
        {
            f_type->fun.type2 = necro_create_type_fun(infer, necro_infer_apat(infer, apats->apats.apat), NULL);
            f_type = f_type->fun.type2;
        }
        apats = apats->apats.next_apat;
    }
    if (necro_is_infer_error(infer)) return NULL;
    f_type->fun.type2 = necro_infer_go(infer, ast->lambda.expression);
    return f_head;
}

//=====================================================
// Simple Assignment
//=====================================================
NecroType* necro_infer_simple_assignment(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_SIMPLE_ASSIGNMENT);
    if (necro_is_infer_error(infer)) return NULL;
    NecroType* proxy_type = infer->symtable->data[ast->simple_assignment.id.id].type;
    NecroType* rhs_type   = necro_infer_go(infer, ast->simple_assignment.rhs);
    if (infer->error.return_code != NECRO_SUCCESS) return NULL;
    necro_unify(infer, proxy_type, rhs_type, ast->scope);
    return NULL;
}

//=====================================================
// RightHandSide
//=====================================================
NecroType* necro_infer_right_hand_side(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_RIGHT_HAND_SIDE);
    if (necro_is_infer_error(infer)) return NULL;
    necro_infer_go(infer, ast->right_hand_side.declarations);
    return necro_infer_go(infer, ast->right_hand_side.expression);
}

//=====================================================
// Let
//=====================================================
NecroType* necro_infer_let_expression(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_LET_EXPRESSION);
    if (necro_is_infer_error(infer)) return NULL;
    necro_infer_go(infer, ast->let_expression.declarations);
    return necro_infer_go(infer, ast->let_expression.expression);
}

//=====================================================
// Arithmetic Sequence
//=====================================================
NecroType* necro_infer_arithmetic_sequence(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_ARITHMETIC_SEQUENCE);
    if (necro_is_infer_error(infer)) return NULL;
    NecroType* type = necro_symtable_get(infer->symtable, infer->prim_types.int_type.id)->type;
    if (ast->arithmetic_sequence.from != NULL)
        necro_unify(infer, type, necro_infer_go(infer, ast->arithmetic_sequence.from), ast->scope);
    if (ast->arithmetic_sequence.then != NULL)
        necro_unify(infer, type, necro_infer_go(infer, ast->arithmetic_sequence.then), ast->scope);
    if (ast->arithmetic_sequence.to != NULL)
        necro_unify(infer, type, necro_infer_go(infer, ast->arithmetic_sequence.to), ast->scope);
    NecroType* aseq = necro_make_con_1(infer, infer->prim_types.list_type.symbol, type);
    aseq->source_loc = ast->source_loc;
    return aseq;
}

//=====================================================
// Declaration
//=====================================================
NecroType* necro_infer_declaration(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_DECL);
    if (necro_is_infer_error(infer)) return NULL;
    NecroNode* current_decl = ast;

    //----------------------------------------------------
    // Pass 0, add type signatures
    while (current_decl != NULL)
    {
        assert(current_decl->type == NECRO_AST_DECL);
        switch (current_decl->declaration.declaration_impl->type)
        {
        case NECRO_AST_TYPE_SIGNATURE: necro_infer_type_sig(infer, current_decl->declaration.declaration_impl); break;
        default: break;
        };
        current_decl = current_decl->declaration.next_declaration;
    }
    if (necro_is_infer_error(infer)) return NULL;

    //----------------------------------------------------
    // Pass 1, create new names to serve as proxy types (This is the easy solution to recursive let, but produces less general types)
    current_decl = ast;
    while (current_decl != NULL)
    {
        assert(current_decl->type == NECRO_AST_DECL);
        switch (current_decl->declaration.declaration_impl->type)
        {
        case NECRO_AST_SIMPLE_ASSIGNMENT:
            if (necro_symtable_get(infer->symtable, current_decl->declaration.declaration_impl->simple_assignment.id)->type == NULL)
            {
                NecroType* new_name = necro_new_name(infer);
                new_name->source_loc = current_decl->declaration.declaration_impl->source_loc;
                new_name->var.var.scope = infer->symtable->data[current_decl->declaration.declaration_impl->simple_assignment.id.id].scope;
                infer->symtable->data[current_decl->declaration.declaration_impl->simple_assignment.id.id].type = new_name;
            }
            break;
        case NECRO_AST_APATS_ASSIGNMENT:
            if (necro_symtable_get(infer->symtable, current_decl->declaration.declaration_impl->apats_assignment.id)->type == NULL)
            {
                NecroType* new_name = necro_new_name(infer);
                new_name->source_loc = current_decl->declaration.declaration_impl->source_loc;
                new_name->var.var.scope = infer->symtable->data[current_decl->declaration.declaration_impl->apats_assignment.id.id].scope;
                infer->symtable->data[current_decl->declaration.declaration_impl->apats_assignment.id.id].type = new_name;
            }
            break;
        case NECRO_AST_TYPE_SIGNATURE: break; // Do Nothing
        default: return necro_infer_ast_error(infer, NULL, ast, "Compiler bug: Unknown or unimplemented declaration type: %d", current_decl->declaration.declaration_impl->type);
        }
        if (necro_is_infer_error(infer)) return NULL;
        current_decl = current_decl->declaration.next_declaration;
    }
    if (necro_is_infer_error(infer)) return NULL;

    //----------------------------------------------------
    // Pass 2, Infer types for declarations, then unify assignments with their proxy types
    current_decl = ast;
    while (current_decl != NULL)
    {
        assert(current_decl->type == NECRO_AST_DECL);
        necro_infer_go(infer, current_decl->declaration.declaration_impl);
        if (necro_is_infer_error(infer)) return NULL;
        current_decl = current_decl->declaration.next_declaration;
    }

    //----------------------------------------------------
    // Pass 3, Generalize declared types
    current_decl = ast;
    while (current_decl != NULL)
    {
        assert(current_decl->type == NECRO_AST_DECL);
        switch (current_decl->declaration.declaration_impl->type)
        {
        case NECRO_AST_SIMPLE_ASSIGNMENT:
            if (!necro_symtable_get(infer->symtable, current_decl->declaration.declaration_impl->simple_assignment.id)->type->pre_supplied)
            {
                NecroVar   var        = (NecroVar) { current_decl->declaration.declaration_impl->simple_assignment.id };
                NecroType* proxy_type = infer->symtable->data[var.id.id].type;
                infer->symtable->data[var.id.id].type = necro_gen(infer, proxy_type, infer->symtable->data[var.id.id].scope->parent);
            }
            break;
        case NECRO_AST_APATS_ASSIGNMENT:
            if (!necro_symtable_get(infer->symtable, current_decl->declaration.declaration_impl->apats_assignment.id)->type->pre_supplied)
            {
                NecroVar   var        = (NecroVar) { current_decl->declaration.declaration_impl->apats_assignment.id };
                NecroType* proxy_type = infer->symtable->data[var.id.id].type;
                infer->symtable->data[var.id.id].type = necro_gen(infer, proxy_type, infer->symtable->data[var.id.id].scope->parent);
            }
            break;
        case NECRO_AST_TYPE_SIGNATURE: break; // Do Nothing
        default: return necro_infer_ast_error(infer, NULL, ast, "Compiler bug: Unknown or unimplemented declaration type: %d", current_decl->declaration.declaration_impl->type);
        }
        if (necro_is_infer_error(infer)) return NULL;
        current_decl = current_decl->declaration.next_declaration;
    }
    if (necro_is_infer_error(infer)) return NULL;

    // Declarations themselves have no types
    return NULL;
}

//=====================================================
// Top Declaration
//=====================================================
NecroType* necro_infer_top_declaration(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_TOP_DECL);
    if (necro_is_infer_error(infer)) return NULL;
    NecroNode* current_decl = ast;

    //----------------------------------------------------
    // Pass -4, Data Declarations
    current_decl = ast;
    while (current_decl != NULL)
    {
        assert(current_decl->type == NECRO_AST_TOP_DECL);
        if (current_decl->top_declaration.declaration->type == NECRO_AST_DATA_DECLARATION)
        {
            necro_infer_data_declaration(infer, current_decl->top_declaration.declaration);
        }
        if (necro_is_infer_error(infer)) return NULL;
        current_decl = current_decl->top_declaration.next_top_decl;
    }
    if (necro_is_infer_error(infer)) return NULL;

    //----------------------------------------------------
    // Pass -3, Type Class Declarations Pass 1
    current_decl = ast;
    while (current_decl != NULL)
    {
        assert(current_decl->type == NECRO_AST_TOP_DECL);
        if (current_decl->top_declaration.declaration->type == NECRO_AST_TYPE_CLASS_DECLARATION)
        {
            necro_create_type_class_declaration_pass1(infer, infer->type_class_env, current_decl->top_declaration.declaration);
        }
        if (necro_is_infer_error(infer)) return NULL;
        current_decl = current_decl->top_declaration.next_top_decl;
    }
    if (necro_is_infer_error(infer)) return NULL;

    //----------------------------------------------------
    // Pass -2, Type Class Declarations Pass 2
    current_decl = ast;
    while (current_decl != NULL)
    {
        assert(current_decl->type == NECRO_AST_TOP_DECL);
        if (current_decl->top_declaration.declaration->type == NECRO_AST_TYPE_CLASS_DECLARATION)
        {
            necro_create_type_class_declaration_pass2(infer, infer->type_class_env, current_decl->top_declaration.declaration);
        }
        if (necro_is_infer_error(infer)) return NULL;
        current_decl = current_decl->top_declaration.next_top_decl;
    }
    if (necro_is_infer_error(infer)) return NULL;

    //----------------------------------------------------
    // Pass -1, Type Class Instance
    current_decl = ast;
    while (current_decl != NULL)
    {
        assert(current_decl->type == NECRO_AST_TOP_DECL);
        if (current_decl->top_declaration.declaration->type == NECRO_AST_TYPE_CLASS_INSTANCE)
        {
            necro_create_type_class_instance(infer, (NecroTypeClassEnv*)infer->type_class_env, current_decl->top_declaration.declaration);
        }
        if (necro_is_infer_error(infer)) return NULL;
        current_decl = current_decl->top_declaration.next_top_decl;
    }
    if (necro_is_infer_error(infer)) return NULL;

    //----------------------------------------------------
    // Pass 0, add type signatures
    current_decl = ast;
    while (current_decl != NULL)
    {
        assert(current_decl->type == NECRO_AST_TOP_DECL);
        switch (current_decl->top_declaration.declaration->type)
        {
        case NECRO_AST_TYPE_SIGNATURE: necro_infer_type_sig(infer, current_decl->top_declaration.declaration); break;
        default: break;
        };
        if (necro_is_infer_error(infer)) return NULL;
        current_decl = current_decl->top_declaration.next_top_decl;
    }
    if (necro_is_infer_error(infer)) return NULL;

    //----------------------------------------------------
    // Pass 1, create new names to serve as proxy types (This is the easy solution to recursive let, but produces less general types)
    current_decl = ast;
    while (current_decl != NULL)
    {
        assert(current_decl->type == NECRO_AST_TOP_DECL);
        switch (current_decl->top_declaration.declaration->type)
        {
        case NECRO_AST_SIMPLE_ASSIGNMENT:
            if (necro_symtable_get(infer->symtable, current_decl->top_declaration.declaration->simple_assignment.id)->type == NULL)
            {
                NecroType* new_name = necro_new_name(infer);
                new_name->source_loc = current_decl->top_declaration.declaration->source_loc;
                new_name->var.var.scope = infer->symtable->data[current_decl->top_declaration.declaration->simple_assignment.id.id].scope;
                infer->symtable->data[current_decl->top_declaration.declaration->simple_assignment.id.id].type = new_name;
            }
            break;
        case NECRO_AST_APATS_ASSIGNMENT:
            if (necro_symtable_get(infer->symtable, current_decl->top_declaration.declaration->apats_assignment.id)->type == NULL)
            {
                NecroType* new_name = necro_new_name(infer);
                new_name->source_loc = current_decl->top_declaration.declaration->source_loc;
                new_name->var.var.scope = infer->symtable->data[current_decl->declaration.declaration_impl->apats_assignment.id.id].scope;
                infer->symtable->data[current_decl->declaration.declaration_impl->apats_assignment.id.id].type = new_name;
            }
            break;
        case NECRO_AST_TYPE_SIGNATURE: break; // Do Nothing
        case NECRO_AST_DATA_DECLARATION: break; // Do Nothing
        case NECRO_AST_TYPE_CLASS_DECLARATION: break; // Do Nothing
        case NECRO_AST_TYPE_CLASS_INSTANCE: break; // Do Nothing
        default: return necro_infer_ast_error(infer, NULL, ast, "Compiler bug: Unknown or unimplemented declaration type: %d", current_decl->top_declaration.declaration->type);
        }
        if (necro_is_infer_error(infer)) return NULL;
        current_decl = current_decl->top_declaration.next_top_decl;
    }
    if (necro_is_infer_error(infer)) return NULL;

    //----------------------------------------------------
    // Pass 2, Infer types for rhs of declarations
    current_decl = ast;
    while (current_decl != NULL)
    {
        assert(current_decl->type == NECRO_AST_TOP_DECL);
        necro_infer_go(infer, current_decl->top_declaration.declaration);
        if (necro_is_infer_error(infer)) return NULL;
        current_decl = current_decl->top_declaration.next_top_decl;
    }
    if (necro_is_infer_error(infer)) return NULL;

    //----------------------------------------------------
    // Pass 3, Infer types for rhs of declarations
    current_decl = ast;
    while (current_decl != NULL)
    {
        assert(current_decl->type == NECRO_AST_TOP_DECL);
        switch (current_decl->top_declaration.declaration->type)
        {
        case NECRO_AST_SIMPLE_ASSIGNMENT:
            if (!necro_symtable_get(infer->symtable, current_decl->top_declaration.declaration->simple_assignment.id)->type->pre_supplied)
            {
                NecroVar   var        = (NecroVar) { current_decl->top_declaration.declaration->simple_assignment.id };
                NecroType* proxy_type = infer->symtable->data[var.id.id].type;
                infer->symtable->data[var.id.id].type = necro_gen(infer, proxy_type, infer->symtable->data[var.id.id].scope->parent);
            }
            break;
        case NECRO_AST_APATS_ASSIGNMENT:
            if (!necro_symtable_get(infer->symtable, current_decl->top_declaration.declaration->apats_assignment.id)->type->pre_supplied)
            {
                NecroVar   var        = (NecroVar) { current_decl->top_declaration.declaration->apats_assignment.id };
                NecroType* proxy_type = infer->symtable->data[var.id.id].type;
                infer->symtable->data[var.id.id].type = necro_gen(infer, proxy_type, infer->symtable->data[var.id.id].scope->parent);
            }
            break;
        case NECRO_AST_TYPE_SIGNATURE: break; // Do Nothing
        case NECRO_AST_DATA_DECLARATION: break; // Do Nothing
        case NECRO_AST_TYPE_CLASS_DECLARATION: break; // Do Nothing
        case NECRO_AST_TYPE_CLASS_INSTANCE: break; // Do Nothing
        default: return necro_infer_ast_error(infer, NULL, ast, "Compiler bug: Unknown or unimplemented declaration type: %d", current_decl->top_declaration.declaration->type);
        }
        if (necro_is_infer_error(infer)) return NULL;
        current_decl = current_decl->top_declaration.next_top_decl;
    }
    if (necro_is_infer_error(infer)) return NULL;

    // Declarations themselves have no types
    return NULL;
}

//=====================================================
// Infer Go
//=====================================================
NecroType* necro_infer_go(NecroInfer* infer, NecroNode* ast)
{
    // TRACE_INFER("necro_infer_go\n");
    assert(infer != NULL);
    if (infer->error.return_code != NECRO_SUCCESS)
        return NULL;
    if (ast == NULL)
        return NULL;
    if (ast->source_loc.character != 0 && ast->source_loc.line != 0 && ast->source_loc.character != 0)
        infer->error.source_loc = ast->source_loc;
    switch (ast->type)
    {
    case NECRO_AST_CONSTANT:               return necro_infer_constant(infer, ast);
    case NECRO_AST_VARIABLE:               return necro_infer_var(infer, ast);
    case NECRO_AST_CONID:                  return necro_infer_conid(infer, ast);
    case NECRO_AST_FUNCTION_EXPRESSION:    return necro_infer_fexpr(infer, ast);
    case NECRO_AST_BIN_OP:                 return necro_infer_bin_op(infer, ast);
    case NECRO_AST_IF_THEN_ELSE:           return necro_infer_if_then_else(infer, ast);
    case NECRO_AST_SIMPLE_ASSIGNMENT:      return necro_infer_simple_assignment(infer, ast);
    case NECRO_AST_APATS_ASSIGNMENT:       return necro_infer_apats_assignment(infer, ast);
    case NECRO_AST_RIGHT_HAND_SIDE:        return necro_infer_right_hand_side(infer, ast);
    case NECRO_AST_LAMBDA:                 return necro_infer_lambda(infer, ast);
    case NECRO_AST_LET_EXPRESSION:         return necro_infer_let_expression(infer, ast);
    case NECRO_AST_DECL:                   return necro_infer_declaration(infer, ast);
    case NECRO_AST_TOP_DECL:               return necro_infer_top_declaration(infer, ast);
    case NECRO_AST_TUPLE:                  return necro_infer_tuple(infer, ast);
    case NECRO_AST_EXPRESSION_LIST:        return necro_infer_expression_list(infer, ast);
    case NECRO_AST_CASE:                   return necro_infer_case(infer, ast);
    case NECRO_AST_WILDCARD:               return necro_infer_wildcard(infer, ast);
    case NECRO_AST_ARITHMETIC_SEQUENCE:    return necro_infer_arithmetic_sequence(infer, ast);
    case NECRO_AST_TYPE_SIGNATURE:         return NULL;
    case NECRO_AST_DATA_DECLARATION:       return NULL;
    case NECRO_AST_TYPE_CLASS_DECLARATION: return NULL;
    case NECRO_AST_TYPE_CLASS_INSTANCE:    return NULL;
    default:                               return necro_infer_ast_error(infer, NULL, ast, "AST type %d has not been implemented for type inference", ast->type);
    }
    return NULL;
}

NecroType* necro_infer(NecroInfer* infer, NecroNode* ast)
{
    NecroType* result = necro_infer_go(infer, ast);
    necro_check_type_sanity(infer, result);
    if (infer->error.return_code != NECRO_SUCCESS)
        return NULL;
    return result;
}

NecroType* necro_infer_ast_error(NecroInfer* infer, NecroType* type, NecroNode* ast, const char* error_message, ...)
{
    if (infer->error.return_code != NECRO_SUCCESS)
        return NULL;
    va_list args;
	va_start(args, error_message);

    NecroSourceLoc source_loc = (type != NULL) ? type->source_loc : infer->error.source_loc;
    size_t count1 = necro_verror(&infer->error, source_loc, error_message, args);
    if (type != NULL)
    {
        size_t count2 = snprintf(infer->error.error_message + count1, NECRO_MAX_ERROR_MESSAGE_LENGTH, "\n    Found in type:\n        ");
        necro_snprintf_type_sig(type, infer->intern, infer->error.error_message + count1 + count2, NECRO_MAX_ERROR_MESSAGE_LENGTH);
    }

    va_end(args);

    return NULL;
}

//=====================================================
// Testing
//=====================================================
void necro_test_infer()
{
    necro_announce_phase("NecroInfer");
}