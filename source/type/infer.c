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
#include "kind.h"
#include "infer.h"

#define NECRO_INFER_DEBUG           1
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
NecroType* necro_infer_simple_assignment(NecroInfer* infer, NecroNode* ast);
void       necro_pat_new_name_go(NecroInfer* infer, NecroNode* ast);
void       necro_gen_pat_go(NecroInfer* infer, NecroNode* ast);
NecroType* necro_infer_pat_assignment(NecroInfer* infer, NecroNode* ast);
NecroType* necro_infer_apats_assignment(NecroInfer* infer, NecroNode* ast);

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
    NECRO_AST_EXPRESSION_SEQUENCE,
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
    NECRO_AST_TOP_DECL,
    NECRO_AST_DECL,
    NECRO_AST_TYPE_CLASS_CONTEXT,
    NECRO_AST_TYPE_CLASS_DECLARATION,
    NECRO_AST_TYPE_CLASS_INSTANCE,
    Correct kind inference
    NECRO_AST_PAT_ASSIGNMENT

    NECRO_AST_DO,
    NECRO_BIND_ASSIGNMENT,
    Declaration dependency analysis (This is how Haskell infers more general types for recursive declarations)

IN PROGRESS:
    Other ops

TODO:
    // NECRO_AST_MODULE,
*/

//=====================================================
// TypeSig
//=====================================================
// TODO: Look at what the fuck is going on here
NecroType* necro_ast_to_type_sig_go(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    if (necro_is_infer_error(infer)) return NULL;
    switch (ast->type)
    {
    case NECRO_AST_VARIABLE:      return necro_create_type_var(infer, (NecroVar) { .id = ast->variable.id, .symbol = ast->variable.symbol });
    case NECRO_AST_TUPLE:         return necro_infer_tuple_type(infer, ast);
    case NECRO_AST_FUNCTION_TYPE: return necro_create_type_fun(infer, necro_ast_to_type_sig_go(infer, ast->function_type.type), necro_ast_to_type_sig_go(infer, ast->function_type.next_on_arrow));
    case NECRO_AST_CONID:
    {
        if (necro_symtable_get(infer->symtable, ast->conid.id)->type == NULL)
            return necro_infer_ast_error(infer, NULL, ast, "Can't find data type: %s", necro_intern_get_string(infer->intern, ast->conid.symbol));
        NecroType* env_con_type = necro_symtable_get(infer->symtable, ast->conid.id)->type;
        NecroType* con_type     = necro_create_type_con(infer, env_con_type->con.con, NULL, env_con_type->con.arity);
        assert(con_type != NULL);
        return con_type;
    }
    case NECRO_AST_CONSTRUCTOR:
    {
        if (necro_symtable_get(infer->symtable, ast->constructor.conid->conid.id)->type == NULL)
            return necro_infer_ast_error(infer, NULL, ast, "Can't find data type: %s", necro_intern_get_string(infer->intern, ast->conid.symbol));
        NecroType* con_args = NULL;
        NecroNode* arg_list = ast->constructor.arg_list;
        size_t arity = 0;
        while (arg_list != NULL)
        {
            if (con_args == NULL)
                con_args = necro_create_type_list(infer, necro_ast_to_type_sig_go(infer, arg_list->list.item), NULL);
            else
                con_args->list.next = necro_create_type_list(infer, necro_ast_to_type_sig_go(infer, arg_list->list.item), NULL);
            arg_list = arg_list->list.next_item;
            arity++;
        }
        NecroType* env_con_type = necro_symtable_get(infer->symtable, ast->constructor.conid->conid.id)->type;
        NecroType* con_type     = necro_create_type_con(infer, env_con_type->con.con, con_args, env_con_type->con.arity);
        assert(con_type != NULL);
        return con_type;
    }
    // Perhaps this is fucked!?
    case NECRO_AST_TYPE_APP:
    {
        NecroType* left   = necro_ast_to_type_sig_go(infer, ast->type_app.ty);
        NecroType* right  = necro_ast_to_type_sig_go(infer, ast->type_app.next_ty);
        if (necro_is_infer_error(infer)) return NULL;
        left->source_loc  = ast->source_loc;
        right->source_loc = ast->source_loc;
        if (left->type == NECRO_TYPE_CON)
        {
            if (left->con.args == NULL)
            {
                left->con.args = necro_create_type_list(infer, right, NULL);
            }
            else if (left->con.args != NULL)
            {
                NecroType* current_arg = left->con.args;
                while (current_arg->list.next != NULL)
                    current_arg = current_arg->list.next;
                current_arg->list.next = necro_create_type_list(infer, right, NULL);
            }
            return left;
        }
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
    if (necro_is_infer_error(infer)) return NULL;

    NecroType* type_sig = necro_ast_to_type_sig_go(infer, ast->type_signature.type);
    if (necro_is_infer_error(infer)) return NULL;

    NecroTypeClassContext* context = necro_ast_to_context(infer, ast->type_signature.context);
    if (necro_is_infer_error(infer)) return NULL;
    context = necro_union_contexts(infer, context, NULL);
    if (necro_is_infer_error(infer)) return NULL;
    if (necro_ambiguous_type_class_check(infer, ast->type_signature.var->variable.symbol, context, type_sig)) return NULL;
    necro_apply_constraints(infer, type_sig, context);

    type_sig = necro_gen(infer, type_sig, ast->type_signature.type->scope);
    if (necro_is_infer_error(infer)) return NULL;

    // Kind inference
    necro_kind_infer(infer, type_sig, type_sig, "While inferring the type of a type signature");
    if (necro_is_infer_error(infer)) return NULL;
    type_sig->type_kind = necro_kind_gen(infer, type_sig->type_kind);
    if (necro_is_infer_error(infer)) return NULL;
    // necro_print_type_sig(type_sig, infer->intern);
    // necro_print_type_sig(type_sig->type_kind, infer->intern);
    necro_kind_unify(infer, type_sig->type_kind, infer->star_type_kind, ast->scope, type_sig, "While inferring the type of a type signature");
    if (necro_is_infer_error(infer)) return NULL;

    // kind check for type context!
    NecroTypeClassContext* curr_context = context;
    while (curr_context != NULL)
    {
        NecroTypeClass* type_class     = curr_context->type_class;
        NecroType*      type_class_var = necro_symtable_get(infer->symtable, type_class->type_var.id)->type;
        NecroType*      var_type       = necro_symtable_get(infer->symtable, curr_context->type_var.id)->type;
        necro_kind_unify(infer, var_type->type_kind, type_class_var->type_kind, ast->scope, type_sig, "While inferring the type of a type signature");
        if (necro_is_infer_error(infer)) return NULL;
        curr_context = curr_context->next;
    }

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

            head = necro_create_type_list(infer, necro_create_type_var(infer, (NecroVar) { .id = ty_vars->list.item->variable.id.id, .symbol = ty_vars->list.item->variable.symbol}), NULL);
            type = head;
        }
        else
        {
            type->list.next = necro_create_type_list(infer, necro_create_type_var(infer, (NecroVar) { .id = ty_vars->list.item->variable.id.id, .symbol = ty_vars->list.item->variable.symbol }), NULL);
            type = type->list.next;
        }
        necro_symtable_get(infer->symtable, ty_vars->list.item->variable.id)->type = necro_new_name(infer, ty_vars->source_loc);
        ty_vars = ty_vars->list.next_item;
    }
    return head;
}

NecroType* necro_create_data_constructor(NecroInfer* infer, NecroNode* ast, NecroType* data_type)
// NecroType* necro_create_data_constructor(NecroInfer* infer, NecroNode* ast)
{
    // WRONG
    // NecroType* data_type = necro_symtable_get(infer->symtable, ast->simple_type.type_con->conid.id)->type;
    NecroType* con_head  = NULL;
    NecroType* con_args  = NULL;
    NecroNode* args_ast  = ast->constructor.arg_list;
    size_t     count     = 0;
    while (args_ast != NULL)
    {
        NecroType* arg = necro_ast_to_type_sig_go(infer, args_ast->list.item);
        if (con_args == NULL)
        {
            con_args = necro_create_type_fun(infer, arg, NULL);
            con_head = con_args;
        }
        else
        {
            con_args->fun.type2 = necro_create_type_fun(infer, arg, NULL);
            con_args            = con_args->fun.type2;
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
    // necro_kind_infer(infer, con_type, con_type, "While declaring a data constructor");
    // necro_kind_unify(infer, con_type->type_kind, infer->star_type_kind, NULL, con_type, "During a data declaration: ");
    con_type = necro_gen(infer, con_type, NULL);
    if (necro_is_infer_error(infer)) return NULL;
    con_type->source_loc   = ast->source_loc;
    con_type->pre_supplied = true;
    necro_symtable_get(infer->symtable, ast->constructor.conid->conid.id)->type           = con_type;
    necro_symtable_get(infer->symtable, ast->constructor.conid->conid.id)->is_constructor = true;
    necro_kind_infer(infer, con_type, con_type, "While declaring a data constructor");
    necro_kind_unify(infer, con_type->type_kind, infer->star_type_kind, NULL, con_type, "During a data declaration: ");
    return con_type;
}

NecroType* necro_infer_simple_type(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_SIMPLE_TYPE);
    if (necro_is_infer_error(infer)) return NULL;
    // Higher-kinded type goes in symtable
    // necro_declare_type(infer, (NecroCon) { .symbol = ast->simple_type.type_con->conid.symbol, .id = ast->simple_type.type_con->conid.id.id }, necro_count_ty_vars(ast->simple_type.type_var_list));
    // if (necro_is_infer_error(infer)) return NULL;
    // Fully applied type used for Data Constructor declaration
    NecroType* type    = necro_create_type_con(infer, (NecroCon) { .symbol = ast->simple_type.type_con->conid.symbol, .id = ast->simple_type.type_con->conid.id.id }, necro_ty_vars_to_args(infer, ast->simple_type.type_var_list), necro_count_ty_vars(ast->simple_type.type_var_list));
    type->source_loc   = ast->source_loc;
    type->pre_supplied = true;
    necro_symtable_get(infer->symtable, ast->simple_type.type_con->conid.id)->type = type;
    // if (necro_is_infer_error(infer)) return NULL;
    // necro_infer_kind(infer, type, infer->star_kind, type, "During data declaration");
    if (necro_is_infer_error(infer)) return NULL;
    // necro_symtable_get(infer->symtable, ast->simple_type.type_con->conid.id)->type->type_kind = necro_new_name(infer, ast->source_loc);
    // necro_symtable_get(infer->symtable, ast->simple_type.type_con->conid.id)->type->type_kind = necro_new_name(infer, ast->source_loc);
    necro_kind_infer(infer, type, type, "During data declaration");
    assert(type->type_kind != NULL);
    // necro_kind_unify(infer, type->type_kind, infer->star_type_kind, NULL, type, "During a data declaration: ");
    // necro_print_type_sig(necro_symtable_get(infer->symtable, ast->simple_type.type_con->conid.id)->type->type_kind, infer->intern);
    // TODO: data declarations in declaration groups, infer kinds everywhere, clean up what will be broken, add missing shit, etc...
    return type;
}

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
    NecroType* rhs = necro_infer_go(infer, ast->apats_assignment.rhs);
    f_type->fun.type2 = rhs;
    // necro_infer_kind(infer, f_head, infer->star_kind, f_head, "While inferring the type of a function declaration: ");
    // necro_kind_infer(infer, f_head, f_head, "While inferring the type of a function declaration: ");
    // necro_kind_unify(infer, f_head->type_kind, infer->star_type_kind, NULL, f_head, "While inferring the type of a function declaration: ");
    necro_unify(infer, proxy_type, f_head, ast->scope, proxy_type, "While inferring the type of a function declaration: ");
    return NULL;
}

NecroType* necro_infer_simple_assignment(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_SIMPLE_ASSIGNMENT);
    if (necro_is_infer_error(infer)) return NULL;
    NecroType* proxy_type = infer->symtable->data[ast->simple_assignment.id.id].type;
    NecroType* rhs_type   = necro_infer_go(infer, ast->simple_assignment.rhs);
    if (infer->error.return_code != NECRO_SUCCESS) return NULL;
    necro_unify(infer, proxy_type, rhs_type, ast->scope, proxy_type, "While inferring the type of an assignment: ");
    return NULL;
}

void necro_ensure_pat_binding_is_monomorphic(NecroInfer* infer, NecroNode* ast)
{
    if (necro_is_infer_error(infer)) return;
    assert(infer != NULL);
    assert(ast != NULL);
    switch (ast->type)
    {
    case NECRO_AST_VARIABLE:
        if (necro_symtable_get(infer->symtable, ast->variable.id)->type->type == NECRO_TYPE_FOR)
            necro_infer_ast_error(infer, necro_symtable_get(infer->symtable, ast->variable.id)->type, ast, "Type error: Polymorphic pattern bindings are illegal.");
        return;
    case NECRO_AST_CONID:    return;
    case NECRO_AST_CONSTANT: return;
    case NECRO_AST_WILDCARD: return;
    case NECRO_AST_TUPLE:
    {
        NecroNode* tuple = ast->tuple.expressions;
        while (tuple != NULL)
        {
            necro_ensure_pat_binding_is_monomorphic(infer, tuple->list.item);
            if (necro_is_infer_error(infer)) return;
            tuple = tuple->list.next_item;
        }
        return;
    }
    case NECRO_AST_EXPRESSION_LIST:
    {
        NecroNode* list = ast->expression_list.expressions;
        while (list != NULL)
        {
            necro_ensure_pat_binding_is_monomorphic(infer, list->list.item);
            if (necro_is_infer_error(infer)) return;
            list = list->list.next_item;
        }
        return;
    }

    case NECRO_AST_BIN_OP_SYM:
        necro_ensure_pat_binding_is_monomorphic(infer, ast->bin_op_sym.left);
        if (necro_is_infer_error(infer)) return;
        necro_ensure_pat_binding_is_monomorphic(infer, ast->bin_op_sym.right);
        return;

    case NECRO_AST_CONSTRUCTOR:
    {
        NecroNode* args = ast->constructor.arg_list;
        while (args != NULL)
        {
            necro_ensure_pat_binding_is_monomorphic(infer, args->list.item);
            if (necro_is_infer_error(infer)) return;
            args = args->list.next_item;
        }
        return;
    }

    default: necro_infer_ast_error(infer, NULL, ast, "Compiler bug: Unimplemented pattern in necro_ensure_pat_binding_is_monomorphic: %d", ast->type); return;
    }
}

NecroType* necro_infer_pat_assignment(NecroInfer* infer, NecroNode* ast)
{
    if (necro_is_infer_error(infer)) return NULL;
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_PAT_ASSIGNMENT);
    assert(ast->pat_assignment.pat != NULL);
    necro_ensure_pat_binding_is_monomorphic(infer, ast->pat_assignment.pat);
    if (necro_is_infer_error(infer)) return NULL;
    NecroType* pat_type = necro_infer_apat(infer, ast->pat_assignment.pat);
    if (necro_is_infer_error(infer)) return NULL;
    NecroType* rhs_type = necro_infer_go(infer, ast->pat_assignment.rhs);
    if (necro_is_infer_error(infer)) return NULL;
    necro_unify(infer, pat_type, rhs_type, ast->scope, rhs_type, "While inferring the type of a pattern declaration: ");
    if (necro_is_infer_error(infer)) return NULL;
    necro_ensure_pat_binding_is_monomorphic(infer, ast->pat_assignment.pat);
    if (necro_is_infer_error(infer)) return NULL;
    ast->necro_type = pat_type;
    return NULL;
}

void necro_pat_new_name_go(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    if (necro_is_infer_error(infer)) return;
    switch (ast->type)
    {
    case NECRO_AST_VARIABLE:
    {
        NecroType* type = necro_symtable_get(infer->symtable, ast->variable.id)->type;
        if (type == NULL)
        {
            NecroType* new_name = necro_new_name(infer, ast->source_loc);
            new_name->source_loc = ast->source_loc;
            new_name->var.scope  = infer->symtable->data[ast->variable.id.id].scope;
            infer->symtable->data[ast->variable.id.id].type        = new_name;
            infer->symtable->data[ast->variable.id.id].type_status = NECRO_TYPE_CHECKING;
            infer->symtable->data[ast->variable.id.id].delay_scope = ast->delay_scope;
        }
        else
        {
            type->pre_supplied = true;
        }
        return;
    }
    case NECRO_AST_CONSTANT: return;
    case NECRO_AST_TUPLE:
    {
        NecroNode* args = ast->tuple.expressions;
        while (args != NULL)
        {
            necro_pat_new_name_go(infer, args->list.item);
            args = args->list.next_item;
        }
        return;
    }
    case NECRO_AST_EXPRESSION_LIST:
    {
        NecroNode* args = ast->expression_list.expressions;
        while (args != NULL)
        {
            necro_pat_new_name_go(infer, args->list.item);
            args = args->list.next_item;
        }
        return;
    }
    case NECRO_AST_WILDCARD: return;
    case NECRO_AST_BIN_OP_SYM:
    {
        necro_pat_new_name_go(infer, ast->bin_op_sym.left);
        necro_pat_new_name_go(infer, ast->bin_op_sym.right);
        return;
    }
    case NECRO_AST_CONID: return;
    case NECRO_AST_CONSTRUCTOR:
    {
        NecroNode* args = ast->constructor.arg_list;
        while (args != NULL)
        {
            necro_pat_new_name_go(infer, args->list.item);
            args = args->list.next_item;
        }
        return;
    }
    default: necro_infer_ast_error(infer, NULL, ast, "Unimplemented pattern in type inference during necro_pat_new_name_go: %d", ast->type); return;
    }
}

void necro_gen_pat_go(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    if (necro_is_infer_error(infer)) return;
    switch (ast->type)
    {
    case NECRO_AST_VARIABLE:
    {
        // TODO: Take a closer look at this!!!!
        NecroSymbolInfo* symbol_info = necro_symtable_get(infer->symtable, ast->variable.id);
        if (symbol_info->type->pre_supplied || symbol_info->type_status == NECRO_TYPE_DONE)
        {
            symbol_info->type_status  = NECRO_TYPE_DONE;
            return;
        }
        // Monomorphism restriction
        // infer->symtable->data[id.id].type                      = necro_gen(infer, proxy_type, infer->symtable->data[id.id].scope->parent);
        symbol_info->type_status = NECRO_TYPE_DONE;
        necro_kind_infer(infer, symbol_info->type, symbol_info->type, "While declaring a pattern variable: ");
        symbol_info->type->type_kind = necro_kind_gen(infer, symbol_info->type->type_kind);
        necro_kind_unify(infer, symbol_info->type->type_kind, infer->star_type_kind, NULL, symbol_info->type, "While declaring a pattern variable: ");
        return;
    }
    case NECRO_AST_CONSTANT: return;
    case NECRO_AST_TUPLE:
    {
        NecroNode* args = ast->tuple.expressions;
        while (args != NULL)
        {
            necro_gen_pat_go(infer, args->list.item);
            args = args->list.next_item;
        }
        return;
    }
    case NECRO_AST_EXPRESSION_LIST:
    {
        NecroNode* args = ast->expression_list.expressions;
        while (args != NULL)
        {
            necro_gen_pat_go(infer, args->list.item);
            args = args->list.next_item;
        }
        return;
    }
    case NECRO_AST_WILDCARD: return;
    case NECRO_AST_BIN_OP_SYM:
    {
        necro_gen_pat_go(infer, ast->bin_op_sym.left);
        necro_gen_pat_go(infer, ast->bin_op_sym.right);
        return;
    }
    case NECRO_AST_CONID: return;
    case NECRO_AST_CONSTRUCTOR:
    {
        NecroNode* args = ast->constructor.arg_list;
        while (args != NULL)
        {
            necro_gen_pat_go(infer, args->list.item);
            args = args->list.next_item;
        }
        return;
    }
    default: necro_infer_ast_error(infer, NULL, ast, "Unimplemented pattern in type inference during pattern generalization: %d", ast->type); return;
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
    assert(ast->variable.id.id <= infer->symtable->count);
    NecroSymbolInfo* symbol_info = necro_symtable_get(infer->symtable, ast->variable.id);
    assert(symbol_info != NULL);

    if (symbol_info->declaration_group != NULL)
        assert(symbol_info->type != NULL);

    // Recursive value Delay check
    if (symbol_info->delay_scope != NULL && symbol_info->type_status != NECRO_TYPE_DONE && ast->variable.var_type == NECRO_VAR_VAR)
    {
        NecroDelayScope* current_delay_scope = ast->delay_scope;
        NecroDelayScope* symbol_delay_scope  = symbol_info->delay_scope;
        if (current_delay_scope == symbol_delay_scope)
            return necro_infer_ast_error(infer, NULL, ast, "%s cannot instantaneously depend on itself.\n Consider adding a delay of some kind, such as: seq {init, %s}",
                necro_intern_get_string(infer->intern, ast->variable.symbol),
                necro_intern_get_string(infer->intern, ast->variable.symbol));
    }

    if (symbol_info->type == NULL)
    {
        symbol_info->type = necro_new_name(infer, ast->source_loc);
        symbol_info->type->var.scope = symbol_info->scope;
        ast->necro_type = symbol_info->type;
        return symbol_info->type;
    }
    else if (necro_is_bound_in_scope(infer, symbol_info->type, ast->scope))
    {
        ast->necro_type = symbol_info->type;
        return symbol_info->type;
    }
    else
    {
        ast->variable.inst_context = NULL;
        NecroType* inst_type       = necro_inst_with_context(infer, symbol_info->type, ast->scope, &ast->variable.inst_context);
        ast->necro_type            = inst_type;
        return inst_type;
    }
    assert(false);
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
    case NECRO_AST_CONSTANT_FLOAT:
    {
        return necro_symtable_get(infer->symtable, infer->prim_types->float_type.id)->type;
        // NecroType* new_name   = necro_new_name(infer, ast->source_loc);
        // new_name->var.context = necro_create_type_class_context(&infer->arena, infer->prim_types->fractional_type_class, (NecroCon) { .id = new_name->var.var.id, .symbol = new_name->var.var.symbol }, NULL);
        // new_name->type_kind   = infer->star_type_kind;
        // // new_name->kind = infer->star_kind;
        // return new_name;
    }
    case NECRO_AST_CONSTANT_INTEGER:
    {
        return necro_symtable_get(infer->symtable, infer->prim_types->int_type.id)->type;
        // NecroType* new_name   = necro_new_name(infer, ast->source_loc);
        // new_name->var.context = necro_create_type_class_context(&infer->arena, infer->prim_types->num_type_class, (NecroCon) { .id = new_name->var.var.id, .symbol = new_name->var.var.symbol }, NULL);
        // new_name->type_kind   = infer->star_type_kind;
        // // new_name->kind = infer->star_kind;
        // return new_name;
    }
    case NECRO_AST_CONSTANT_BOOL:    return necro_symtable_get(infer->symtable, infer->prim_types->bool_type.id)->type;
    case NECRO_AST_CONSTANT_CHAR:    return necro_symtable_get(infer->symtable, infer->prim_types->char_type.id)->type;
    case NECRO_AST_CONSTANT_STRING:  return necro_infer_ast_error(infer, NULL, ast, "Compiler bug: String not implemented....");
    default:                         return necro_infer_ast_error(infer, NULL, ast, "Compiler bug: Unrecognized constant type: %d", ast->constant.type);
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
    return necro_new_name(infer, ast->source_loc);
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
    if (necro_is_infer_error(infer)) return NULL;
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
    NecroType* list_type    = necro_new_name(infer, ast->source_loc);
    list_type->source_loc   = ast->source_loc;
    while (current_cell != NULL)
    {
        necro_unify(infer, list_type, necro_infer_go(infer, current_cell->list.item), ast->scope, list_type, "While inferring the type for a list []: ");
        if (necro_is_infer_error(infer)) return NULL;
        current_cell = current_cell->list.next_item;
    }
    NecroType* list = necro_make_con_1(infer, infer->prim_types->list_type, list_type);
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
    NecroType* list_type    = necro_new_name(infer, ast->source_loc);
    while (current_cell != NULL)
    {
        necro_unify(infer, list_type, necro_infer_pattern(infer, current_cell->list.item), ast->scope, list_type, "While inferring the type for a list [] pattern: ");
        if (necro_is_infer_error(infer)) return NULL;
        current_cell = current_cell->list.next_item;
    }
    NecroType* list = necro_make_con_1(infer, infer->prim_types->list_type, list_type);
    list->source_loc = ast->source_loc;
    return list;
}

//=====================================================
// Expression Sequence
//=====================================================
NecroType* necro_infer_expression_sequence(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_EXPRESSION_SEQUENCE);
    if (necro_is_infer_error(infer)) return NULL;
    NecroNode* current_cell = ast->expression_sequence.expressions;
    NecroType* seq_type     = necro_new_name(infer, ast->source_loc);
    seq_type->source_loc    = ast->source_loc;
    while (current_cell != NULL)
    {
        necro_unify(infer, seq_type, necro_infer_go(infer, current_cell->list.item), ast->scope, seq_type, "While inferring the type for a sequence: ");
        if (necro_is_infer_error(infer)) return NULL;
        current_cell = current_cell->list.next_item;
    }
    NecroType* sequence = necro_make_con_1(infer, infer->prim_types->sequence_type, seq_type);
    sequence->source_loc = ast->source_loc;
    return sequence;
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
    NecroType* result_type  = necro_new_name(infer, ast->source_loc);
    result_type->source_loc = ast->source_loc;
    NecroType* f_type       = necro_create_type_fun(infer, e1_type, result_type);
    f_type->source_loc      = ast->source_loc;
    // necro_infer_kind(infer, f_type, infer->star_kind, f_type, "While inferring the type for a function application: ");
    if (necro_is_infer_error(infer)) return NULL;
    necro_unify(infer, e0_type, f_type, ast->scope, f_type, "While inferring the type for a function application: ");
    // necro_kind_infer(infer, f_type, f_type, "While inferring the type for a function application: ");
    // necro_kind_unify(infer, f_type->type_kind, infer->star_type_kind, NULL, f_type, "While inferring the type for a function application: ");
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
    necro_unify(infer, if_type, necro_symtable_get(infer->symtable, infer->prim_types->bool_type.id)->type, ast->scope, if_type, "While inferring the type of an if/then/else expression: ");
    necro_unify(infer, then_type, else_type, ast->scope, then_type, "While inferring the type of an if/then/else expression: ");
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

    ast->bin_op.inst_context = NULL;
    NecroType* op_type       = necro_inst_with_context(infer, infer->symtable->data[ast->bin_op.id.id].type, ast->scope, &ast->bin_op.inst_context);
    assert(op_type != NULL);

    NecroType* y_type       = necro_infer_go(infer, ast->bin_op.rhs);
    if (necro_is_infer_error(infer)) return NULL;
    NecroType* result_type  = necro_new_name(infer, ast->source_loc);
    result_type->source_loc = ast->source_loc;
    NecroType* bin_op_type  = necro_create_type_fun(infer, x_type, necro_create_type_fun(infer, y_type, result_type));
    necro_unify(infer, op_type, bin_op_type, ast->scope, op_type, "While inferring the type of a bin-op: ");
    ast->necro_type         = bin_op_type;
    if (necro_is_infer_error(infer)) return NULL;
    return result_type;
}

//=====================================================
// Operator Left Section
//=====================================================
NecroType* necro_infer_op_left_section(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_OP_LEFT_SECTION);
    if (necro_is_infer_error(infer)) return NULL;
    NecroType* x_type = necro_infer_go(infer, ast->op_left_section.left);

    ast->op_left_section.inst_context = NULL;
    NecroType* op_type                = necro_inst_with_context(infer, infer->symtable->data[ast->op_left_section.id.id].type, ast->scope, &ast->op_left_section.inst_context);
    assert(op_type != NULL);
    ast->op_left_section.op_necro_type = op_type;

    NecroType* result_type  = necro_new_name(infer, ast->source_loc);
    result_type->source_loc = ast->source_loc;
    NecroType* section_type = necro_create_type_fun(infer, x_type, result_type);
    necro_unify(infer, op_type, section_type, ast->scope, op_type, "While inferring the type of a bin-op left section: ");
    if (necro_is_infer_error(infer)) return NULL;
    ast->necro_type = section_type;
    return result_type;
}

//=====================================================
// Operator Right Section
//=====================================================
NecroType* necro_infer_op_right_section(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_OP_RIGHT_SECTION);
    if (necro_is_infer_error(infer)) return NULL;

    ast->op_right_section.inst_context = NULL;
    NecroType* op_type                = necro_inst_with_context(infer, infer->symtable->data[ast->op_right_section.id.id].type, ast->scope, &ast->op_right_section.inst_context);
    assert(op_type != NULL);
    ast->op_right_section.op_necro_type = op_type;

    NecroType* y_type  = necro_infer_go(infer, ast->op_right_section.right);

    NecroType* x_type       = necro_new_name(infer, ast->source_loc);
    NecroType* result_type  = necro_new_name(infer, ast->source_loc);
    NecroType* bin_op_type  = necro_create_type_fun(infer, x_type, necro_create_type_fun(infer, y_type, result_type));
    necro_unify(infer, op_type, bin_op_type, ast->scope, op_type, "While inferring the type of a bin-op: ");
    NecroType* section_type = necro_create_type_fun(infer, x_type, result_type);
    if (necro_is_infer_error(infer)) return NULL;
    ast->necro_type = section_type;
    return section_type;
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
    case NECRO_AST_TUPLE:           return necro_infer_tuple_pattern(infer, ast);
    case NECRO_AST_EXPRESSION_LIST: return necro_infer_expression_list_pattern(infer, ast);
    case NECRO_AST_WILDCARD:        return necro_new_name(infer, ast->source_loc);

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
        NecroType* data_type  = necro_new_name(infer, ast->source_loc);
        data_type->source_loc = ast->source_loc;
        NecroType* f_type     = necro_create_type_fun(infer, left_type, necro_create_type_fun(infer, right_type, data_type));
        if (necro_is_infer_error(infer)) return NULL;
        necro_unify(infer, constructor_type, f_type, ast->scope, constructor_type, "While inferring the type of a bin-op pattern: ");
        return data_type;
    }

    case NECRO_AST_CONID:
    {
        NecroType* constructor_type  = necro_symtable_get(infer->symtable, ast->conid.id)->type;
        constructor_type             = necro_inst(infer, constructor_type, NULL);
        if (constructor_type == NULL)
            return necro_infer_ast_error(infer, NULL, ast, "Compiler bug: Constructor \'%s\' has not type", necro_intern_get_string(infer->intern, ast->constructor.conid->conid.symbol));
        size_t constructor_args = 0;
        NecroType* con_iter = constructor_type;
        while (con_iter->type == NECRO_TYPE_FUN)
        {
            constructor_args++;
            con_iter = con_iter->fun.type2;
        }
        NecroType* type = necro_infer_conid(infer, ast);
        if (constructor_args != 0)
            return necro_infer_ast_error(infer, type, ast, "Wrong number of arguments for constructor %s. Expected arity: %d, found arity %d", necro_intern_get_string(infer->intern, ast->conid.symbol), constructor_args, 0);
        necro_unify(infer, type, constructor_type, ast->scope, type, "While inferring the type of a constructor pattern: ");
        if (necro_is_infer_error(infer)) return NULL;
        return type;
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

        NecroType* data_type = necro_new_name(infer, ast->source_loc);
        data_type->source_loc = ast->source_loc;
        if (pattern_type_head == NULL)
        {
            pattern_type_head = data_type;
        }
        else
        {
            pattern_type->fun.type2 = data_type;
        }

        // generalize type, inst type, unify against instantiated type
        size_t constructor_args = 0;
        NecroType* con_iter = constructor_type;
        while (con_iter->type == NECRO_TYPE_FUN)
        {
            constructor_args++;
            con_iter = con_iter->fun.type2;
        }
        if (arg_count != constructor_args)
            return necro_infer_ast_error(infer, pattern_type_head, ast, "Wrong number of arguments for constructor %s. Expected arity: %d, found arity %d", necro_intern_get_string(infer->intern, ast->constructor.conid->conid.symbol), constructor_args, arg_count);
        necro_unify(infer, constructor_type, pattern_type_head, ast->scope, pattern_type_head, "While inferring the type of a constructor pattern: ");
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
    NecroType* result_type     = necro_new_name(infer, ast->source_loc);
    result_type->source_loc    = ast->source_loc;
    NecroNode* alternatives    = ast->case_expression.alternatives;
    while (alternatives != NULL)
    {
        NecroNode* alternative = alternatives->list.item;
        necro_unify(infer, expression_type, necro_infer_pattern(infer, alternative->case_alternative.pat), alternatives->scope, expression_type, "While inferring the type of a case expression: ");
        necro_unify(infer, result_type, necro_infer_go(infer, alternative->case_alternative.body), alternatives->scope, result_type, "While inferring the type of a case expression: ");
        alternatives = alternatives->list.next_item;
    }
    return result_type;
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
    NecroType* type = necro_symtable_get(infer->symtable, infer->prim_types->int_type.id)->type;
    if (ast->arithmetic_sequence.from != NULL)
        necro_unify(infer, type, necro_infer_go(infer, ast->arithmetic_sequence.from), ast->scope, type, "While inferring the type of an arithmetic sequence: ");
    if (ast->arithmetic_sequence.then != NULL)
        necro_unify(infer, type, necro_infer_go(infer, ast->arithmetic_sequence.then), ast->scope, type, "While inferring the type of an arithmetic sequence: ");
    if (ast->arithmetic_sequence.to != NULL)
        necro_unify(infer, type, necro_infer_go(infer, ast->arithmetic_sequence.to), ast->scope, type, "While inferring the type of an arithemetic sequence: ");
    NecroType* aseq = necro_make_con_1(infer, infer->prim_types->list_type, type);
    aseq->source_loc = ast->source_loc;
    return aseq;
}

//=====================================================
// Do
//=====================================================
NecroType* necro_infer_do_statement(NecroInfer* infer, NecroNode* ast, NecroType* monad_var)
{
    assert(infer != NULL);
    assert(ast != NULL);
    if (necro_is_infer_error(infer)) return NULL;
    switch(ast->type)
    {

    case NECRO_AST_LET_EXPRESSION: ast->necro_type = necro_infer_let_expression(infer, ast); return NULL;

    case NECRO_BIND_ASSIGNMENT:
    {
        NecroType* var_name                                    = necro_new_name(infer, ast->source_loc);
        infer->symtable->data[ast->bind_assignment.id.id].type = var_name;
        NecroType* rhs_type                                    = necro_infer_go(infer, ast->bind_assignment.expression);
        if (necro_is_infer_error(infer)) return NULL;
        NecroType* result_type = necro_create_type_app(infer, monad_var, var_name);
        necro_unify(infer, rhs_type, result_type, ast->scope, rhs_type, "While inferring the type of a bind assignment: ");
        // don't generalize bind assignment variables
        ast->necro_type = result_type;
        return NULL;
    }

    case NECRO_PAT_BIND_ASSIGNMENT:
    {
        NecroType* pat_type = necro_infer_pattern(infer, ast->pat_bind_assignment.pat);
        if (necro_is_infer_error(infer)) return NULL;
        NecroType* rhs_type = necro_infer_go(infer, ast->pat_bind_assignment.expression);
        if (necro_is_infer_error(infer)) return NULL;
        NecroType* result_type = necro_create_type_app(infer, monad_var, pat_type);
        necro_unify(infer, rhs_type, result_type, ast->scope, rhs_type, "While inferring the type of a pattern bind assignment: ");
        ast->necro_type = result_type;
        return NULL;
    }

    // default: return necro_infer_ast_error(infer, NULL, ast, "Unimplemented ast type in infer_do_statement : %d", ast->type);
    // case NECRO_AST_VARIABLE:            statement_type = necro_infer_var(infer, ast);             break;
    // case NECRO_AST_CONID:               statement_type = necro_infer_conid(infer, ast);           break;
// case NECRO_AST_EXPRESSION_LIST:     statement_type = necro_infer_expression_list(infer, ast); break;
// case NECRO_AST_FUNCTION_EXPRESSION: statement_type = necro_infer_fexpr(infer, ast);           break;
// This should be ok actually?
    default:
    {
        NecroType* statement_type = necro_infer_go(infer, ast);
        NecroType* result_type = necro_create_type_app(infer, monad_var, necro_new_name(infer, ast->source_loc));
        if (necro_is_infer_error(infer)) return NULL;
        necro_unify(infer, statement_type, result_type, ast->scope, statement_type, "While inferring the type of a do statement: ");
        ast->necro_type = result_type;
        return result_type;
    }

    }
}

NecroType* necro_infer_do(NecroInfer* infer, NecroNode* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_DO);
    if (necro_is_infer_error(infer)) return NULL;
    NecroType* monad_var = necro_new_name(infer, ast->source_loc);
    NecroNode* statements = ast->do_statement.statement_list;
    NecroType* statement_type = NULL;
    necro_apply_constraints(infer, monad_var, necro_create_type_class_context(&infer->arena, necro_symtable_get(infer->symtable, infer->prim_types->monad_type_class.id)->type_class, infer->prim_types->monad_type_class, (NecroCon) { .id = monad_var->var.var.id, .symbol = monad_var->var.var.symbol }, NULL));
    while (statements != NULL)
    {
        statement_type = necro_infer_do_statement(infer, statements->list.item, monad_var);
        if (necro_is_infer_error(infer)) return NULL;
        statements = statements->list.next_item;
    }
    ast->necro_type = statement_type;
    ast->do_statement.monad_var = monad_var;
    if (statement_type == NULL)
        return necro_infer_ast_error(infer, NULL, ast, "The final statement in a do block must be an expression");
    else
        return statement_type;
}

//=====================================================
// Declaration groups
//=====================================================
NecroType* necro_infer_declaration_group(NecroInfer* infer, NecroDeclarationGroup* declaration_group)
{
    assert(infer != NULL);
    assert(declaration_group != NULL);
    NecroNode*       ast         = NULL;
    NecroSymbolInfo* symbol_info = NULL;

    //-----------------------------
    // Pass 1, new names
    NecroDeclarationGroup* curr = declaration_group;
    while (curr != NULL)
    {
        if (curr->type_checked) { curr = curr->next; continue; }
        ast = curr->declaration_ast;
        if (necro_is_infer_error(infer)) return NULL;
        switch(ast->type)
        {
        case NECRO_AST_SIMPLE_ASSIGNMENT:
        {
            assert(ast->simple_assignment.id.id < infer->symtable->count);
            symbol_info = necro_symtable_get(infer->symtable, ast->simple_assignment.id);
            symbol_info->delay_scope = ast->delay_scope;
            if (symbol_info->type == NULL)
            {
                NecroType* new_name = necro_new_name(infer, ast->source_loc);
                new_name->var.scope = symbol_info->scope;
                symbol_info->type   = new_name;
            }
            else
            {
                // Hack: For built-ins
                symbol_info->type->pre_supplied = true;
            }
            break;
        }
        case NECRO_AST_APATS_ASSIGNMENT:
        {
            assert(ast->apats_assignment.id.id < infer->symtable->count);
            symbol_info = necro_symtable_get(infer->symtable, ast->apats_assignment.id);
            if (symbol_info->type == NULL)
            {
                NecroType* new_name = necro_new_name(infer, ast->source_loc);
                new_name->var.scope = symbol_info->scope;
                symbol_info->type   = new_name;
            }
            else
            {
                // Hack: For built-ins
                // symbol_info->type->pre_supplied = true;
            }
            break;
        }
        case NECRO_AST_PAT_ASSIGNMENT:
            necro_pat_new_name_go(infer, ast->pat_assignment.pat);
            break;
        case NECRO_AST_DATA_DECLARATION:
            necro_infer_simple_type(infer, ast->data_declaration.simpletype);
            break;
        case NECRO_AST_TYPE_SIGNATURE:
            necro_infer_type_sig(infer, ast);
            break;
        case NECRO_AST_TYPE_CLASS_DECLARATION:
            necro_create_type_class(infer, ast);
            break;
        case NECRO_AST_TYPE_CLASS_INSTANCE:
            necro_create_type_class_instance(infer, ast);
            break;
        default: return necro_infer_ast_error(infer, NULL, ast, "Compiler bug: Unrecognized assignment type: %d", ast->type);
        }
        curr = curr->next;
    }

    //-----------------------------
    // Pass 2, infer types
    curr = declaration_group;
    while (curr != NULL)
    {
        if (curr->type_checked) { curr = curr->next; continue; }
        ast = curr->declaration_ast;
        switch(ast->type)
        {
        case NECRO_AST_SIMPLE_ASSIGNMENT:
            necro_infer_simple_assignment(infer, ast);
            break;
        case NECRO_AST_APATS_ASSIGNMENT:
            necro_infer_apats_assignment(infer, ast);
            break;
        case NECRO_AST_PAT_ASSIGNMENT:
            necro_infer_pat_assignment(infer, ast);
            break;
        case NECRO_AST_DATA_DECLARATION:
        {
            NecroNode* constructor_list = ast->data_declaration.constructor_list;
            while (constructor_list != NULL)
            {
                necro_create_data_constructor(infer, constructor_list->list.item, necro_symtable_get(infer->symtable, ast->data_declaration.simpletype->simple_type.type_con->conid.id)->type);
                if (necro_is_infer_error(infer)) return NULL;
                constructor_list = constructor_list->list.next_item;
            }
            break;
        }
        case NECRO_AST_TYPE_SIGNATURE:
            break;
        case NECRO_AST_TYPE_CLASS_DECLARATION:
            break;
        case NECRO_AST_TYPE_CLASS_INSTANCE:
            break;
        default: return necro_infer_ast_error(infer, NULL, ast, "Compiler bug: Unrecognized assignment type: %d", ast->type);
        }
        if (necro_is_infer_error(infer)) return NULL;
        curr = curr->next;
    }

    // declaration_group->type_checked = true;

    //-----------------------------
    // Pass 3, generalize
    curr = declaration_group;
    while (curr != NULL)
    {
        ast = curr->declaration_ast;
        if (curr->type_checked) { curr = curr->next;  continue; }
        switch(ast->type)
        {
        case NECRO_AST_SIMPLE_ASSIGNMENT:
        {
            symbol_info = necro_symtable_get(infer->symtable, ast->simple_assignment.id);
            if (symbol_info->type->pre_supplied || symbol_info->type_status == NECRO_TYPE_DONE) { symbol_info->type_status = NECRO_TYPE_DONE; curr->type_checked = true; curr = curr->next;  continue; }

            // Monomorphism restriction
            // symbol_info->type = necro_gen(infer, symbol_info->type, symbol_info->scope->parent);

            // necro_infer_kind(infer, symbol_info->type, infer->star_kind, symbol_info->type, "While declaraing a variable: ");
            necro_kind_infer(infer, symbol_info->type, symbol_info->type, "While declaring a variable: ");
            if (necro_is_infer_error(infer)) return NULL;
            symbol_info->type->type_kind = necro_kind_gen(infer, symbol_info->type->type_kind);
            if (necro_is_infer_error(infer)) return NULL;
            necro_kind_unify(infer, symbol_info->type->type_kind, infer->star_type_kind, NULL, symbol_info->type, "While declaring a variable: ");
            if (necro_is_infer_error(infer)) return NULL;
            symbol_info->type_status = NECRO_TYPE_DONE;
            break;
        }
        case NECRO_AST_APATS_ASSIGNMENT:
        {
            symbol_info = necro_symtable_get(infer->symtable, ast->apats_assignment.id);
            if (symbol_info->type->pre_supplied || symbol_info->type_status == NECRO_TYPE_DONE) { symbol_info->type_status = NECRO_TYPE_DONE; curr->type_checked = true; curr = curr->next;  continue; }
            symbol_info->type = necro_gen(infer, symbol_info->type, symbol_info->scope->parent);
            // necro_infer_kind(infer, symbol_info->type, infer->star_kind, symbol_info->type, "While declaraing a variable: ");
            necro_kind_infer(infer, symbol_info->type, symbol_info->type, "While declaring a variable: ");
            if (necro_is_infer_error(infer)) return NULL;
            symbol_info->type->type_kind = necro_kind_gen(infer, symbol_info->type->type_kind);
            if (necro_is_infer_error(infer)) return NULL;
            necro_kind_unify(infer, symbol_info->type->type_kind, infer->star_type_kind, NULL, symbol_info->type, "While declaring a variable: ");
            if (necro_is_infer_error(infer)) return NULL;
            symbol_info->type_status = NECRO_TYPE_DONE;
            break;
        }
        case NECRO_AST_PAT_ASSIGNMENT:
        {
            necro_gen_pat_go(infer, ast->pat_assignment.pat);
            // // symbol_info = necro_symtable_get(infer->symtable, ast->apats_assignment.id);
            // // if (symbol_info->type->pre_supplied || symbol_info->type_status == NECRO_TYPE_DONE) { symbol_info->type_status = NECRO_TYPE_DONE; curr->type_checked = true; curr = curr->next;  continue; }
            // symbol_info->type = necro_gen(infer, symbol_info->type, symbol_info->scope->parent);
            // // necro_infer_kind(infer, symbol_info->type, infer->star_kind, symbol_info->type, "While declaraing a variable: ");
            // necro_kind_infer(infer, symbol_info->type, symbol_info->type, "While declaring a variable: ");
            // if (necro_is_infer_error(infer)) return NULL;
            // symbol_info->type->type_kind = necro_kind_gen(infer, symbol_info->type->type_kind);
            // if (necro_is_infer_error(infer)) return NULL;
            // necro_kind_unify(infer, symbol_info->type->type_kind, infer->star_type_kind, NULL, symbol_info->type, "While declaring a variable: ");
            // if (necro_is_infer_error(infer)) return NULL;
            // symbol_info->type_status = NECRO_TYPE_DONE;
            break;
        }
        case NECRO_AST_DATA_DECLARATION:
        {
            NecroSymbolInfo* simple_type_symbol_info = necro_symtable_get(infer->symtable, ast->data_declaration.simpletype->simple_type.type_con->conid.id);
            simple_type_symbol_info->type->type_kind = necro_kind_gen(infer, simple_type_symbol_info->type->type_kind);
            // necro_print_type_sig(simple_type_symbol_info->type->type_kind, infer->intern);
            break;
        }
        case NECRO_AST_TYPE_SIGNATURE:
            break;
        case NECRO_AST_TYPE_CLASS_DECLARATION:
            break;
        case NECRO_AST_TYPE_CLASS_INSTANCE:
            break;
        default: return necro_infer_ast_error(infer, NULL, ast, "Compiler bug: Unrecognized assignment type: %d", ast->type);
        }
        if (necro_is_infer_error(infer)) return NULL;
        curr->type_checked = true;
        curr               = curr->next;
    }

    return NULL;
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

    //----------------------------------------------------
    // Infer types for declaration groups
    NecroDeclarationGroupList* groups = ast->declaration.group_list;
    while (groups != NULL)
    {
        if (groups->declaration_group != NULL)
        {
            necro_infer_declaration_group(infer, groups->declaration_group);
            if (necro_is_infer_error(infer)) return NULL;
        }
        groups = groups->next;
    }

    // Declarations themselves have no types
    return NULL;
}

//=====================================================
// Top Declaration
//=====================================================
NecroType* necro_infer_top_declaration(NecroInfer* infer, NecroNode* ast)
{
    if (necro_is_infer_error(infer)) return NULL;
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_TOP_DECL);

    //----------------------------------------------------
    // Iterate declaration groups
    NecroDeclarationGroupList* groups = ast->top_declaration.group_list;
    while (groups != NULL)
    {
        if (groups->declaration_group != NULL)
        {
            necro_infer_declaration_group(infer, groups->declaration_group);
            if (necro_is_infer_error(infer)) return NULL;
        }
        groups = groups->next;
    }

    // Declarations themselves have no types
    return NULL;
}

//=====================================================
// Infer Go
//=====================================================
NecroType* necro_infer_go(NecroInfer* infer, NecroNode* ast)
{
    // TRACE_INFER("necro_infer_go\n");
    if (necro_is_infer_error(infer)) return NULL;
    assert(infer != NULL);
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
    case NECRO_AST_OP_LEFT_SECTION:        return necro_infer_op_left_section(infer, ast);
    case NECRO_AST_OP_RIGHT_SECTION:       return necro_infer_op_right_section(infer, ast);
    case NECRO_AST_IF_THEN_ELSE:           return necro_infer_if_then_else(infer, ast);

    case NECRO_AST_RIGHT_HAND_SIDE:        return necro_infer_right_hand_side(infer, ast);
    case NECRO_AST_LAMBDA:                 return necro_infer_lambda(infer, ast);
    case NECRO_AST_LET_EXPRESSION:         return necro_infer_let_expression(infer, ast);
    case NECRO_AST_DECL:                   return necro_infer_declaration(infer, ast);
    case NECRO_AST_TOP_DECL:               return necro_infer_top_declaration(infer, ast);
    case NECRO_AST_TUPLE:                  return necro_infer_tuple(infer, ast);
    case NECRO_AST_EXPRESSION_LIST:        return necro_infer_expression_list(infer, ast);
    case NECRO_AST_EXPRESSION_SEQUENCE:    return necro_infer_expression_sequence(infer, ast);
    case NECRO_AST_CASE:                   return necro_infer_case(infer, ast);
    case NECRO_AST_WILDCARD:               return necro_infer_wildcard(infer, ast);
    case NECRO_AST_ARITHMETIC_SEQUENCE:    return necro_infer_arithmetic_sequence(infer, ast);
    case NECRO_AST_DO:                     return necro_infer_do(infer, ast);

    // case NECRO_AST_SIMPLE_ASSIGNMENT:      return NULL;
    // case NECRO_AST_APATS_ASSIGNMENT:       return NULL;
    // case NECRO_AST_PAT_ASSIGNMENT:         return NULL;
    // case NECRO_AST_TYPE_CLASS_DECLARATION: return NULL;
    // case NECRO_AST_TYPE_CLASS_INSTANCE:    return NULL;
    // case NECRO_AST_DATA_DECLARATION:       return NULL;

    case NECRO_AST_TYPE_SIGNATURE:         return necro_infer_type_sig(infer, ast);

    default:                               return necro_infer_ast_error(infer, NULL, ast, "AST type %d has not been implemented for type inference", ast->type);
    }
    return NULL;
}

NecroType* necro_infer(NecroInfer* infer, NecroNode* ast)
{
    NecroType* result = necro_infer_go(infer, ast);
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
        size_t count2 = snprintf(infer->error.error_message + count1, NECRO_MAX_ERROR_MESSAGE_LENGTH, "\n Found in type: ");
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