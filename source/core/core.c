/* Copyright (C) Chad McKinney and Curtis McKinney - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */

#include "core.h"
#include "core_create.h"
#include "type.h"

#define NECRO_CORE_DEBUG 0
#if NECRO_CORE_DEBUG
#define TRACE_CORE(...) printf(__VA_ARGS__)
#else
#define TRACE_CORE(...)
#endif

static inline void print_tabs(uint32_t num_tabs)
{
    for (uint32_t i = 0; i < num_tabs; ++i)
    {
        printf(STRING_TAB);
    }
}

void necro_print_core_node(NecroCoreAST_Expression* ast_node, NecroIntern* intern, uint32_t depth);

void necro_print_core_node(NecroCoreAST_Expression* ast_node, NecroIntern* intern, uint32_t depth)
{
    assert(ast_node);
    assert(intern);
    print_tabs(depth);

    switch (ast_node->expr_type)
    {
    case NECRO_CORE_EXPR_VAR:
        printf("(Var: %s, %d)\n", ast_node->var.symbol->str, ast_node->var.id.id);
        break;

    case NECRO_CORE_EXPR_BIND:
        if (!ast_node->bind.is_recursive)
            printf("(Bind: %s, %d)\n", ast_node->bind.var.symbol->str, ast_node->bind.var.id.id);
        else
            printf("(BindRec: %s, %d)\n", ast_node->bind.var.symbol->str, ast_node->bind.var.id.id);
        necro_print_core_node(ast_node->bind.expr, intern, depth + 1);
        break;

    case NECRO_CORE_EXPR_LIT:
        {
            switch (ast_node->lit.type)
            {
            case NECRO_AST_CONSTANT_FLOAT:
            case NECRO_AST_CONSTANT_FLOAT_PATTERN:
                printf("(%f)\n", ast_node->lit.double_literal);
                break;

            case NECRO_AST_CONSTANT_INTEGER:
            case NECRO_AST_CONSTANT_INTEGER_PATTERN:
    #if WIN32
                printf("(%lli)\n", ast_node->lit.int_literal);
    #else
                printf("(%li)\n", ast_node->constant.int_literal);
    #endif
                break;
            case NECRO_AST_CONSTANT_STRING:
                {
                    const char* string = ast_node->lit.symbol->str;
                    if (string)
                        printf("(\"%s\")\n", string);
                }
                break;

            case NECRO_AST_CONSTANT_CHAR:
            case NECRO_AST_CONSTANT_CHAR_PATTERN:
                printf("(\'%c\')\n", ast_node->lit.char_literal);
                break;

            // case NECRO_AST_CONSTANT_BOOL:
            //     printf("(%s)\n", ast_node->lit.boolean_literal ? "True" : "False");
            //     break;

            default:
                assert(false && "[necro_print_core_node] Unhandled literal type!");
                break;
            }
        }
        break;

    case NECRO_CORE_EXPR_APP:
        puts("(App)");
        necro_print_core_node(ast_node->app.exprA, intern, depth + 1);
        necro_print_core_node(ast_node->app.exprB, intern, depth + 1);
        break;

    case NECRO_CORE_EXPR_LAM:
        puts("(\\ ->)");
        necro_print_core_node(ast_node->lambda.arg, intern, depth + 1);
        necro_print_core_node(ast_node->lambda.expr, intern, depth + 1);
        break;

    case NECRO_CORE_EXPR_LET:
        {
            puts("(Let)");
            necro_print_core_node(ast_node->let.bind, intern, depth + 1);
            necro_print_core_node(ast_node->let.expr, intern, depth + 1);
        }
        break;

    case NECRO_CORE_EXPR_CASE:
        {
            puts("(Case)");
            necro_print_core_node(ast_node->case_expr.expr, intern, depth + 1);
            NecroCoreAST_CaseAlt* alt = ast_node->case_expr.alts;

            printf("\r");
            print_tabs(depth + 1);
            puts("(Of)");

            while (alt)
            {
                print_tabs(depth + 2);
                printf("(AltCon)\n");
                if (alt->altCon)
                {
                    necro_print_core_node(alt->altCon, intern, depth + 3);
                }
                else
                {
                    print_tabs(depth + 3);
                    printf("_\n");
                }

                necro_print_core_node(alt->expr, intern, depth + 3);
                alt = alt->next;
            }
        }
        break;

    case NECRO_CORE_EXPR_LIST:
        {
            puts("(CORE_EXPR_LIST)");
            // NecroCoreAST_List* list_expr = &ast_node->list;
            NecroCoreAST_Expression* list_expr = ast_node;
            while (list_expr)
            {
                if (list_expr->list.expr)
                {
                    necro_print_core_node(list_expr->list.expr, intern, depth + 1);
                }
                else
                {
                    print_tabs(depth + 1);
                    printf("_\n");
                }
                list_expr = list_expr->list.next;
            }
        }
        break;

    case NECRO_CORE_EXPR_DATA_DECL:
        {
            printf("(Data %s)\n", ast_node->data_decl.data_id.symbol->str);
            NecroCoreAST_DataCon* con = ast_node->data_decl.con_list;
            while (con)
            {
                NecroCoreAST_Expression con_expr;
                con_expr.expr_type = NECRO_CORE_EXPR_DATA_CON;
                con_expr.data_con = *con;
                necro_print_core_node(&con_expr, intern, depth + 1);
                con = con->next;
            }
        }
        break;

    case NECRO_CORE_EXPR_DATA_CON:
        printf("(DataCon: %s, %d)\n", ast_node->data_con.condid.symbol->str, ast_node->data_con.condid.id.id);
        if (ast_node->data_con.arg_list)
        {
            necro_print_core_node(ast_node->data_con.arg_list, intern, depth + 1);
        }
        break;
    case NECRO_CORE_EXPR_TYPE:
    {
        // char buf[1024];
        // char* buf_end = necro_type_sig_snprintf(ast_node->type.type, intern, buf, 1024);
        // *buf_end = '\0';
        // printf("(Type: %s)\n", buf);
        printf("(Type: ");
        necro_type_print(ast_node->type.type);
        break;
    }

    default:
        printf("necro_print_core_node printing expression type unimplemented!: %s\n", core_ast_names[ast_node->expr_type]);
        assert(false && "necro_print_core_node printing expression type unimplemented!");
        break;
    }
}

void necro_print_core(NecroCoreAST* ast, NecroIntern* intern)
{
    assert(ast != NULL);
    necro_print_core_node(ast->root, intern, 0);
}

NecroTransformToCore necro_empty_core_transform()
{
    return (NecroTransformToCore)
    {
        .necro_ast  = NULL,
        .core_ast   = NULL,
        .intern     = NULL,
        .prim_types = NULL,
        .symtable   = NULL,
    };
}

void necro_construct_core_transform(
    NecroTransformToCore* core_transform,
    NecroCoreAST* core_ast,
    NecroAstArena* necro_ast,
    NecroIntern* intern,
    NecroPrimTypes* prim_types,
    NecroSymTable* symtable,
    NecroScopedSymTable* scoped_symtable)
{
    core_transform->core_ast = core_ast;
    core_transform->core_ast->arena = necro_paged_arena_create();
    core_transform->necro_ast = necro_ast;
    core_transform->intern = intern;
    core_transform->prim_types = prim_types;
    core_transform->transform_state = NECRO_CORE_TRANSFORMING;
    core_transform->symtable = symtable;

    NecroCoreConstructors* constructors = &core_transform->constructors;
    constructors->twoTuple =    necro_scoped_symtable_get_top_level_symbol_var(scoped_symtable, "(,)");
    constructors->threeTuple =  necro_scoped_symtable_get_top_level_symbol_var(scoped_symtable, "(,,)");
    constructors->fourTuple =   necro_scoped_symtable_get_top_level_symbol_var(scoped_symtable, "(,,,)");
    constructors->fiveTuple =   necro_scoped_symtable_get_top_level_symbol_var(scoped_symtable, "(,,,,)");
    constructors->sixTuple =    necro_scoped_symtable_get_top_level_symbol_var(scoped_symtable, "(,,,,,)");
    constructors->sevenTuple =  necro_scoped_symtable_get_top_level_symbol_var(scoped_symtable, "(,,,,,,)");
    constructors->eightTuple =  necro_scoped_symtable_get_top_level_symbol_var(scoped_symtable, "(,,,,,,,)");
    constructors->nineTuple =   necro_scoped_symtable_get_top_level_symbol_var(scoped_symtable, "(,,,,,,,,)");
    constructors->tenTuple =    necro_scoped_symtable_get_top_level_symbol_var(scoped_symtable, "(,,,,,,,,,)");
}

NecroCoreAST_Expression* necro_transform_to_core_impl(NecroTransformToCore* core_transform, NecroAst* necro_ast_node);

NecroCoreAST_Expression* necro_transform_bin_op(NecroTransformToCore* core_transform, NecroAst* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_BIN_OP);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    // 1 + 2 --> (+) 1 2 --> ((+) 1) 2 --> App (App (+) 1) 2
    NecroAstBinOp* bin_op = &necro_ast_node->bin_op;

    NecroCoreAST_Expression* inner_core_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    inner_core_expr->expr_type = NECRO_CORE_EXPR_APP;
    inner_core_expr->app.persistent_slot = 0; // Curtis: Metadata for codegen (would really prefer a constructor...)
    NecroCoreAST_Application* inner_core_app = &inner_core_expr->app;

    NecroCoreAST_Expression* var_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    var_expr->expr_type = NECRO_CORE_EXPR_VAR;
    // var_expr->var.symbol = bin_op->symbol; // TODO: CONVERT / FINISH! Core system needs to be converted to NecroAstSymbol system!!!!!!
    // var_expr->var.id = bin_op->id; // TODO: CONVERT / FINISH! Core system needs to be converted to NecroAstSymbol system!!!!!!

    inner_core_app->exprA = var_expr;
    inner_core_app->exprB = necro_transform_to_core_impl(core_transform, bin_op->lhs);

    NecroCoreAST_Expression* outer_core_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    outer_core_expr->expr_type = NECRO_CORE_EXPR_APP;
    outer_core_expr->app.persistent_slot = 0; // Curtis: Metadata for codegen (would really prefer a constructor...)
    NecroCoreAST_Application* outer_core_app = &outer_core_expr->app;
    outer_core_app->exprA = inner_core_expr;
    outer_core_app->exprB = necro_transform_to_core_impl(core_transform, bin_op->rhs);
    return outer_core_expr;
}

// TODO/Note from Curtis: Boolean literals in necrolang don't exist. They need to be excised from the compiler completely.
NecroCoreAST_Expression* necro_transform_if_then_else(NecroTransformToCore* core_transform, NecroAst* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_IF_THEN_ELSE);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    NecroAstIfThenElse* ast_if_then_else = &necro_ast_node->if_then_else;
    NecroCoreAST_Expression* core_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    core_expr->expr_type = NECRO_CORE_EXPR_CASE;

    NecroCoreAST_Case* core_case = &core_expr->case_expr;
    core_case->expr = necro_transform_to_core_impl(core_transform, ast_if_then_else->if_expr);
    core_case->expr->necro_type = ast_if_then_else->if_expr->necro_type;
    core_case->type = necro_ast_node->necro_type;

    NecroCoreAST_CaseAlt* true_alt = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_CaseAlt));
    true_alt->expr = necro_transform_to_core_impl(core_transform, ast_if_then_else->then_expr);
    true_alt->altCon = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    // true_alt->altCon->lit.boolean_literal = true;
    // true_alt->altCon->lit.type = NECRO_AST_CONSTANT_BOOL;
    true_alt->altCon->var        = necro_con_to_var(necro_prim_types_get_data_con_from_symbol(core_transform->prim_types, necro_intern_string(core_transform->intern, "True")));
    true_alt->altCon->expr_type  = NECRO_CORE_EXPR_VAR;
    true_alt->altCon->necro_type = necro_symtable_get(core_transform->symtable, core_transform->prim_types->bool_type.id)->type;
    true_alt->next = NULL;

    NecroCoreAST_CaseAlt* false_alt = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_CaseAlt));
    false_alt->expr = necro_transform_to_core_impl(core_transform, ast_if_then_else->else_expr);
    // false_alt->altCon = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    // false_alt->altCon->lit.boolean_literal = false;
    // false_alt->altCon->lit.type = NECRO_AST_CONSTANT_BOOL;
    false_alt->altCon = NULL;
    false_alt->next = NULL;
    true_alt->next = false_alt;

    core_case->alts = true_alt;
    return core_expr;
}

NecroCoreAST_Expression* necro_transform_simple_assignment(NecroTransformToCore* core_transform, NecroAst* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_SIMPLE_ASSIGNMENT);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    NecroAstSimpleAssignment* simple_assignment = &necro_ast_node->simple_assignment;
    NecroCoreAST_Expression* core_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    core_expr->expr_type = NECRO_CORE_EXPR_BIND;
    NecroCoreAST_Bind* core_bind = &core_expr->bind;
    // core_bind->var.symbol = simple_assignment->variable_name; // TODO: CONVERT / FINISH! Core system needs to be converted to NecroAstSymbol system!!!!!!
    // core_bind->var.id = simple_assignment->id; // TODO: CONVERT / FINISH! Core system needs to be converted to NecroAstSymbol system!!!!!!
    core_bind->is_recursive = simple_assignment->is_recursive;
    core_bind->expr = necro_transform_to_core_impl(core_transform, simple_assignment->rhs);
    necro_symtable_get(core_transform->symtable, core_bind->var.id)->core_ast = core_expr;
    return core_expr;
}

NecroCoreAST_Expression* necro_transform_apats_assignment(NecroTransformToCore* core_transform, NecroAst* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_APATS_ASSIGNMENT);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    NecroAstApatsAssignment* apats_assignment = &necro_ast_node->apats_assignment;
    NecroCoreAST_Expression* core_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    core_expr->expr_type = NECRO_CORE_EXPR_BIND;
    NecroCoreAST_Bind* core_bind = &core_expr->bind;
    // core_bind->var.symbol = apats_assignment->variable_name; // TODO: CONVERT / FINISH! Core system needs to be converted to NecroAstSymbol system!!!!!!
    // core_bind->var.id = apats_assignment->id; // TODO: CONVERT / FINISH! Core system needs to be converted to NecroAstSymbol system!!!!!!
    core_bind->is_recursive = false;
    NecroSymbolInfo* info = necro_symtable_get(core_transform->symtable, core_bind->var.id);
    info->core_ast = core_expr;

    if (apats_assignment->apats)
    {
        NecroAst lambda_node;
        lambda_node.type = NECRO_AST_LAMBDA;
        lambda_node.lambda.apats = apats_assignment->apats;
        lambda_node.lambda.expression = apats_assignment->rhs;
        core_bind->expr = necro_transform_to_core_impl(core_transform, &lambda_node);
    }
    else
    {
        core_bind->expr = necro_transform_to_core_impl(core_transform, apats_assignment->rhs);
    }

    return core_expr;
}

NecroCoreAST_Expression* necro_transform_pat_assignment(NecroTransformToCore* core_transform, NecroAst* necro_ast_node)
{
    assert(false && "[necro_transform_pat_assignment] Not implemented yet, soon TM!");
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_PAT_ASSIGNMENT);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    // NecroAstApatsAssignment* apats_assignment = &necro_ast_node->apats_assignment; // TODO: CONVERT / FINISH! Core system needs to be converted to NecroAstSymbol system!!!!!!
    NecroCoreAST_Expression* core_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    core_expr->expr_type = NECRO_CORE_EXPR_BIND;
    NecroCoreAST_Bind* core_bind = &core_expr->bind;
    // core_bind->var.symbol = apats_assignment->variable_name; // TODO: CONVERT / FINISH! Core system needs to be converted to NecroAstSymbol system!!!!!!
    // core_bind->var.id = apats_assignment->id; // TODO: CONVERT / FINISH! Core system needs to be converted to NecroAstSymbol system!!!!!!
    core_bind->is_recursive = false;

    //necro_ast_node->pat_assignment.pat->pattern_expression.expressions->

    return NULL;
}

NecroCoreAST_Expression* necro_transform_right_hand_side(NecroTransformToCore* core_transform, NecroAst* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_RIGHT_HAND_SIDE);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    NecroAstRightHandSide* right_hand_side = &necro_ast_node->right_hand_side;
    if (right_hand_side->declarations)
    {
        assert(right_hand_side->declarations->type == NECRO_AST_DECL);
        NecroDeclarationGroupList* group_list = right_hand_side->declarations->declaration.group_list;
        NecroCoreAST_Expression* core_let_expr = NULL;
        NecroCoreAST_Let* core_let = NULL;
        NecroCoreAST_Expression* rhs_expression = necro_transform_to_core_impl(core_transform, right_hand_side->expression);
        while (group_list)
        {
            NecroDeclarationGroup* group = group_list->declaration_group;
            while (group != NULL)
            {
                assert(group);
                assert(group->declaration_ast->type == NECRO_AST_SIMPLE_ASSIGNMENT);
                // NOTE - Curtis: Why ^^^? Kicking the can on nested functions!?!?!?
                NecroAstSimpleAssignment* simple_assignment = &group->declaration_ast->simple_assignment;
                NecroCoreAST_Expression* let_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
                let_expr->expr_type = NECRO_CORE_EXPR_LET;
                NecroCoreAST_Let* let = &let_expr->let;
                let->bind = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
                let->bind->expr_type = NECRO_CORE_EXPR_BIND;
                // let->bind->bind.var.id = simple_assignment->id; // TODO: CONVERT / FINISH! Core system needs to be converted to NecroAstSymbol system!!!!!!
                // let->bind->bind.var.symbol = simple_assignment->variable_name; // TODO: CONVERT CORE TO USE NEW NecroAstSymbol system!
                let->bind->bind.is_recursive = simple_assignment->is_recursive;
                necro_symtable_get(core_transform->symtable, let->bind->bind.var.id)->core_ast = let_expr;
                let->bind->bind.expr = necro_transform_to_core_impl(core_transform, simple_assignment->rhs);
                if (core_let)
                {
                    core_let->expr = let_expr;
                }
                else
                {
                    core_let_expr = let_expr;
                }

                core_let = let;
                core_let->expr = rhs_expression;
                group = group->next;
            }
            group_list = group_list->next;
        }

        return core_let_expr;
    }
    else
    {
        return necro_transform_to_core_impl(core_transform, right_hand_side->expression);
    }
}

NecroCoreAST_Expression* necro_transform_let(NecroTransformToCore* core_transform, NecroAst* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_LET_EXPRESSION);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    NecroAstLetExpression* reified_ast_let = &necro_ast_node->let_expression;
    if (reified_ast_let->declarations)
    {
        assert(reified_ast_let->declarations->type == NECRO_AST_DECL);
        NecroDeclarationGroupList* group_list = reified_ast_let->declarations->declaration.group_list;
        NecroCoreAST_Expression* core_let_expr = NULL;
        NecroCoreAST_Let* core_let = NULL;
        NecroCoreAST_Expression* rhs_expression = necro_transform_to_core_impl(core_transform, reified_ast_let->expression);
        while (group_list)
        {
            NecroDeclarationGroup* group = group_list->declaration_group;
            while (group != NULL)
            {
                assert(group);
                assert(group->declaration_ast->type == NECRO_AST_SIMPLE_ASSIGNMENT);
                NecroAstSimpleAssignment* simple_assignment = &group->declaration_ast->simple_assignment;
                NecroCoreAST_Expression* let_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
                let_expr->expr_type = NECRO_CORE_EXPR_LET;
                NecroCoreAST_Let* let = &let_expr->let;
                let->bind = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
                let->bind->expr_type = NECRO_CORE_EXPR_BIND;
                // let->bind->bind.var.id = simple_assignment->id; // TODO: CONVERT / FINISH! Core system needs to be converted to NecroAstSymbol system!!!!!!
                // let->bind->bind.var.symbol = simple_assignment->variable_name; // TODO: Convert core over to use new NecroAstSymbol system!!!!
                necro_symtable_get(core_transform->symtable, let->bind->bind.var.id)->core_ast = let_expr;
                let->bind->bind.expr = necro_transform_to_core_impl(core_transform, simple_assignment->rhs);
                let->bind->bind.is_recursive = simple_assignment->is_recursive;
                if (core_let)
                {
                    core_let->expr = let_expr;
                }
                else
                {
                    core_let_expr = let_expr;
                }

                core_let = let;
                core_let->expr = rhs_expression;
                group = group->next;
            }
            group_list = group_list->next;
        }

        return core_let_expr;
    }
    else
    {
        assert(false && "Let requires a binding!");
        return NULL;
    }
}

NecroCoreAST_Expression* necro_transform_function_expression(NecroTransformToCore* core_transform, NecroAst* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_FUNCTION_EXPRESSION);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    NecroCoreAST_Expression* core_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    core_expr->expr_type = NECRO_CORE_EXPR_APP;
    core_expr->app.exprA = necro_transform_to_core_impl(core_transform, necro_ast_node->fexpression.aexp);
    core_expr->app.exprB = necro_transform_to_core_impl(core_transform, necro_ast_node->fexpression.next_fexpression);
    core_expr->app.persistent_slot = 0; // Curtis: Metadata for codegen (would really prefer a constructor...)
    return core_expr;
}

NecroCoreAST_Expression* necro_transform_constant(NecroTransformToCore* core_transform, NecroAst* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_CONSTANT);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    NecroCoreAST_Expression* core_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    core_expr->expr_type = NECRO_CORE_EXPR_LIT;
    core_expr->lit = necro_ast_node->constant;
    return core_expr;
}

NecroCoreAST_Expression* necro_transform_data_constructor(NecroTransformToCore* core_transform, NecroAst* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_CONSTRUCTOR);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    NecroAstConstructor* ast_constructor = &necro_ast_node->constructor;
    NecroCoreAST_Expression* core_datacon_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    core_datacon_expr->expr_type = NECRO_CORE_EXPR_DATA_CON;

    NecroCoreAST_DataCon* core_datacon = &core_datacon_expr->data_con;
    // core_datacon->condid.id = ast_constructor->conid->conid.id; // TODO: CONVERT / FINISH! Core system needs to be converted to NecroAstSymbol system!!!!!!
    // core_datacon->condid.symbol = ast_constructor->conid->conid.symbol; // TODO: CONVERT / FINISH! Core system needs to be converted to NecroAstSymbol system!!!!!!
    core_datacon->next = NULL;
    core_datacon->arg_list = NULL;

    NecroCoreAST_Expression* current_core_arg = NULL;
    NecroAst* arg_list = ast_constructor->arg_list;

    //---------------------------------------------------------------
    // Curtis: Note, with more complex types this simply breaks down.
    // Please look at this more closely and consider revising
    // (i.e. for Function types don't simply store the type in the constructor ,
    // and correctly handle Type Apps which currently simply break with an assert in necro_transform_to_core_impl).
    // It makes little sense to have a halfway house between NecroType and NecroCoreAST_Expression
    // Pick one and make the entire constructor reify into that
    while (arg_list)
    {
        NecroAst* ast_item = arg_list->list.item;
        NecroCoreAST_Expression* next_core_arg_data_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
        switch (ast_item->type)
        {
        case NECRO_AST_VARIABLE:
            {
                NecroCoreAST_Expression* next_core_arg = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
                next_core_arg->expr_type = NECRO_CORE_EXPR_VAR;
                // next_core_arg->var.id = ast_item->variable.id; // TODO: CONVERT / FINISH! Core system needs to be converted to NecroAstSymbol system!!!!!!
                // next_core_arg->var.symbol = ast_item->variable.symbol; // TODO: CONVERT / FINISH! Core system needs to be converted to NecroAstSymbol system!!!!!!

                next_core_arg_data_expr->expr_type  = NECRO_CORE_EXPR_LIST;
                next_core_arg_data_expr->list.expr = next_core_arg;
                next_core_arg_data_expr->list.next = NULL;
            }
            break;

        case NECRO_AST_TYPE_APP:
            {
                NecroAstTypeApp* type_app = &ast_item->type_app;
                NecroCoreAST_Expression* next_core_arg = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
                next_core_arg->expr_type = NECRO_CORE_EXPR_APP;
                next_core_arg->app.exprA = necro_transform_to_core_impl(core_transform, type_app->ty);
                next_core_arg->app.exprB = necro_transform_to_core_impl(core_transform, type_app->next_ty);
                next_core_arg->app.persistent_slot = 0; // Curtis: Metadata for codegen (would really prefer a constructor...)
                next_core_arg_data_expr->expr_type = NECRO_CORE_EXPR_LIST;
                next_core_arg_data_expr->list.expr = next_core_arg;
                next_core_arg_data_expr->list.next = NULL;
            }
            break;

        case NECRO_AST_CONSTRUCTOR:
            {
                NecroCoreAST_Expression* next_core_arg = necro_transform_to_core_impl(core_transform, ast_item);
                next_core_arg_data_expr->expr_type = NECRO_CORE_EXPR_LIST;
                next_core_arg_data_expr->list.expr = next_core_arg;
                next_core_arg_data_expr->list.next = NULL;
            }
            break;

        case NECRO_AST_WILDCARD:
            next_core_arg_data_expr->expr_type = NECRO_CORE_EXPR_LIST;
            next_core_arg_data_expr->list.expr = NULL;
            next_core_arg_data_expr->list.next = NULL;
            break;
        case NECRO_AST_CONID:
            {
                NecroCoreAST_Expression* next_core_arg = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
                next_core_arg->expr_type               = NECRO_CORE_EXPR_DATA_CON;
                // next_core_arg->data_con.condid         = (NecroVar) { .id = ast_item->conid.id, .symbol = ast_item->conid.symbol }; // TODO: CONVERT / FINISH! Core system needs to be converted to NecroAstSymbol system!!!!!!
                next_core_arg->data_con.arg_list       = NULL;
                next_core_arg->data_con.next           = NULL;
                next_core_arg_data_expr->expr_type = NECRO_CORE_EXPR_LIST;
                next_core_arg_data_expr->list.expr = next_core_arg;
                next_core_arg_data_expr->list.next = NULL;
            }
            break;
        case NECRO_AST_FUNCTION_TYPE:
        {
            NecroAstFunctionType* type_fun = &ast_item->function_type;
            UNUSED(type_fun);
            NecroCoreAST_Expression* next_core_arg = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
            next_core_arg->expr_type = NECRO_CORE_EXPR_TYPE;
            next_core_arg->type.type = ast_item->necro_type;
            // next_core_arg->lambda. = necro_transform_to_core_impl(core_transform, type_app->ty);
            // next_core_arg->app.exprB = necro_transform_to_core_impl(core_transform, type_app->next_ty);
            // next_core_arg->app.persistent_slot = 0; // Curtis: Metadata for codegen (would really prefer a constructor...)
            next_core_arg_data_expr->expr_type = NECRO_CORE_EXPR_LIST;
            next_core_arg_data_expr->list.expr = next_core_arg;
            next_core_arg_data_expr->list.next = NULL;
            break;
        }
        default:
            assert(false && "Unexpected node type when transforming data constructor arg!");
        }

        if (current_core_arg)
        {
            current_core_arg->list.next = next_core_arg_data_expr;
        }
        else
        {
            core_datacon->arg_list = next_core_arg_data_expr;
        }

        current_core_arg = next_core_arg_data_expr;
        arg_list = arg_list->list.next_item;
    }

    return core_datacon_expr;
}

NecroCoreAST_Expression* necro_transform_data_decl(NecroTransformToCore* core_transform, NecroAst* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_DATA_DECLARATION);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    NecroAstDataDeclaration* data_decl = &necro_ast_node->data_declaration;
    assert(data_decl->simpletype->type == NECRO_AST_SIMPLE_TYPE);
    NecroAstSimpleType* simple_type = &data_decl->simpletype->simple_type;
    assert(simple_type->type_con->type == NECRO_AST_CONID);
    // NecroAstConId* conid = &simple_type->type_con->conid;

    NecroCoreAST_Expression* core_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    core_expr->expr_type = NECRO_CORE_EXPR_DATA_DECL;
    // core_expr->data_decl.data_id.id = conid->id; // TODO: CONVERT / FINISH! Core system needs to be converted to NecroAstSymbol system!!!!!!
    // core_expr->data_decl.data_id.symbol = conid->symbol; // TODO: CONVERT / FINISH! Core system needs to be converted to NecroAstSymbol system!!!!!!
    core_expr->data_decl.con_list = NULL;

    NecroCoreAST_DataCon* current_core_data_con = NULL;

    assert(data_decl->constructor_list->type == NECRO_AST_LIST_NODE);
    // NecroAST_ListNode_Reified* list_node = &data_decl->constructor_list->list;
    NecroAst* list_node = data_decl->constructor_list;
    assert(list_node);
    while (list_node)
    {
        assert(list_node->list.item->type == NECRO_AST_CONSTRUCTOR);
        // NecroAST_Constructor_Reified* con = list_node->list.item->constructor;
        // NecroASTNode* con = list_node->list.item;

        NecroCoreAST_DataCon* next_core_data_con = NULL;
        switch (list_node->list.item->type)
        {
        case NECRO_AST_CONSTRUCTOR:
            next_core_data_con = &necro_transform_data_constructor(core_transform, list_node->list.item)->data_con;
            break;
        default:
            assert(false && "Unexpected type during data declaration core transformation!");
            break;
        }

        if (core_expr->data_decl.con_list == NULL)
        {
            core_expr->data_decl.con_list = next_core_data_con;
        }
        else
        {
            current_core_data_con->next = next_core_data_con;
        }

        current_core_data_con = next_core_data_con;
        list_node = list_node->list.next_item;
    }

    return core_expr;
}

NecroCoreAST_Expression* necro_transform_conid(NecroTransformToCore* core_transform, NecroAst* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_CONID);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    NecroCoreAST_Expression* core_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    core_expr->expr_type = NECRO_CORE_EXPR_VAR;
    // core_expr->var.id = necro_ast_node->conid.id; // TODO: CONVERT / FINISH! Core system needs to be converted to NecroAstSymbol system!!!!!!
    // core_expr->var.symbol = necro_ast_node->conid.symbol; // TODO: CONVERT / FINISH! Core system needs to be converted to NecroAstSymbol system!!!!!!
    return core_expr;
}

NecroCoreAST_Expression* necro_transform_top_decl(NecroTransformToCore* core_transform, NecroAst* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_TOP_DECL);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    NecroCoreAST_Expression* top_expression = NULL;
    NecroCoreAST_Expression* next_node = NULL;
    NecroDeclarationGroupList* group_list = necro_ast_node->top_declaration.group_list;
    assert(group_list);
    while (group_list)
    {
        NecroDeclarationGroup* group = group_list->declaration_group;
        while (group != NULL)
        {
            assert(group);
            NecroCoreAST_Expression* last_node = next_node;
            NecroCoreAST_Expression* expression = necro_transform_to_core_impl(core_transform, (NecroAst*) group->declaration_ast);
            next_node = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
            next_node->expr_type = NECRO_CORE_EXPR_LIST;
            next_node->list.expr = expression;
            next_node->list.next = NULL;
            if (top_expression == NULL)
            {
                top_expression = next_node;
            }
            else
            {
                last_node->list.next = next_node;
            }
            group = group->next;
        }
        group_list = group_list->next;
    }

    assert(top_expression);
    return top_expression;
}

NecroCoreAST_Expression* necro_transform_lambda(NecroTransformToCore* core_transform, NecroAst* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_LAMBDA);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    NecroAstLambda* lambda = &necro_ast_node->lambda;
    NecroCoreAST_Expression* core_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    core_expr->expr_type = NECRO_CORE_EXPR_LAM;
    core_expr->necro_type = necro_ast_node->necro_type;

    NecroCoreAST_Expression* last_lambda = NULL;
    NecroCoreAST_Expression* current_lambda = core_expr;

    NecroAstApats* apats = &lambda->apats->apats;
    while (apats)
    {
        NecroAst* apat = apats->apat;
        UNUSED(apat);
        current_lambda->lambda.arg = necro_transform_to_core_impl(core_transform, apats->apat);
        current_lambda->lambda.expr = NULL;
        NecroType* type = necro_symtable_get(core_transform->symtable, current_lambda->lambda.arg->var.id)->type;
        UNUSED(type);

        if (apats->next_apat)
        {
            last_lambda = current_lambda;
            current_lambda = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
            current_lambda->expr_type = NECRO_CORE_EXPR_LAM;
            current_lambda->lambda.arg = NULL;
            current_lambda->lambda.expr = NULL;
        }

        if (last_lambda)
        {
            last_lambda->lambda.expr = current_lambda;
        }

        apats = apats->next_apat ? &apats->next_apat->apats : NULL;
    }

    current_lambda->lambda.expr = necro_transform_to_core_impl(core_transform, lambda->expression);
    // necro_symtable_get(core_transform->symtable, current_lambda->lambda.arg->var.id)->core_ast = current_lambda->lambda.expr;
    return core_expr;
}

NecroCoreAST_Expression* necro_transform_expression_list(NecroTransformToCore* core_transform, NecroAst* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_EXPRESSION_LIST);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    NecroCoreAST_Expression* core_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    core_expr->expr_type = NECRO_CORE_EXPR_VAR;
    core_expr->var.symbol = core_transform->prim_types->list_type.symbol;
    core_expr->var.id = core_transform->prim_types->list_type.id;

    while (necro_ast_node->expression_list.expressions)
    {
        NecroCoreAST_Expression* cons_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
        cons_expr->expr_type = NECRO_CORE_EXPR_VAR;
        //cons_expr->var.symbol = core_transform->prim_types-; // FINISH THIS!
        cons_expr->var.id = core_transform->prim_types->list_type.id;
    }

    return core_expr;
}

NecroCoreAST_Expression* necro_transform_tuple(NecroTransformToCore* core_transform, NecroAst* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_TUPLE);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    NecroCoreAST_Expression* tuple_app_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    tuple_app_expr->expr_type = NECRO_CORE_EXPR_APP;
    tuple_app_expr->app.persistent_slot = 0; // Curtis: Metadata for codegen (would really prefer a constructor...)

    NecroCoreAST_Expression* var_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    var_expr->expr_type = NECRO_CORE_EXPR_VAR;

    NecroCoreAST_Expression* app_expr = tuple_app_expr;
    app_expr->app.exprA = var_expr;
    int tuple_count = 0;
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // Curtis: Hey, This is throwing a warning about mismatched types! Double check!!!!!!
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    NecroAstList* tuple_node = &necro_ast_node->tuple.expressions->list;

    while (tuple_node)
    {
        app_expr->app.exprB = necro_transform_to_core_impl(core_transform, tuple_node->item);
        tuple_node = &tuple_node->next_item->list;
        ++tuple_count;

        if (tuple_node)
        {
            NecroCoreAST_Expression* prev_app_expr = app_expr;
            app_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
            app_expr->expr_type = NECRO_CORE_EXPR_APP;
            app_expr->app.exprA = prev_app_expr;
            app_expr->app.persistent_slot = 0; // Curtis: Metadata for codegen (would really prefer a constructor...)
        }
    }

    assert(tuple_count > 1 && "[necro_transform_tuple] unhandled size of tuple!");
    assert(tuple_node == NULL);

    switch (tuple_count)
    {
    case 2:
        var_expr->var = core_transform->constructors.twoTuple;
        break;
    case 3:
        var_expr->var = core_transform->constructors.threeTuple;
        break;
    case 4:
        var_expr->var = core_transform->constructors.fourTuple;
        break;
    case 5:
        var_expr->var = core_transform->constructors.fiveTuple;
        break;
    case 6:
        var_expr->var = core_transform->constructors.sixTuple;
        break;
    case 7:
        var_expr->var = core_transform->constructors.sevenTuple;
        break;
    case 8:
        var_expr->var = core_transform->constructors.eightTuple;
        break;
    case 9:
        var_expr->var = core_transform->constructors.nineTuple;
        break;
    case 10:
        var_expr->var = core_transform->constructors.tenTuple;
        break;
    default:
        assert(false && "[necro_transform_tuple] unhandled size of tuple!");
       break;
    }

    return app_expr;
}

NecroCoreAST_Expression* necro_transform_case(NecroTransformToCore* core_transform, NecroAst* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_CASE);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    NecroAstCase* case_ast = &necro_ast_node->case_expression;
    NecroCoreAST_Expression* core_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    core_expr->expr_type = NECRO_CORE_EXPR_CASE;
    NecroCoreAST_Case* core_case = &core_expr->case_expr;
    core_case->expr = necro_transform_to_core_impl(core_transform, case_ast->expression);
    core_case->expr->necro_type = case_ast->expression->necro_type;
    core_case->alts = NULL;
    core_case->type = necro_ast_node->necro_type;

    NecroAst* list_node = case_ast->alternatives;
    // NecroAST_Node_Reified* alt_list_node = case_ast->alternatives;
    // NecroAST_ListNode_Reified* list_node = &alt_list_node->list;
    NecroCoreAST_CaseAlt* case_alt = NULL;

    while (list_node)
    {
        NecroAst* alt_node = list_node->list.item;
        assert(list_node->list.item->type == NECRO_AST_CASE_ALTERNATIVE);
        NecroAstCaseAlternative* alt = &alt_node->case_alternative;

        NecroCoreAST_CaseAlt* last_case_alt = case_alt;
        case_alt = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_CaseAlt));
        case_alt->expr = necro_transform_to_core_impl(core_transform, alt->body);
        case_alt->next = NULL;
        case_alt->altCon = NULL;

        switch (alt->pat->type)
        {

        case NECRO_AST_CONSTANT:
            case_alt->altCon = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
            case_alt->altCon->expr_type = NECRO_CORE_EXPR_LIT;
            case_alt->altCon->lit = alt->pat->constant;
            break;

        case NECRO_AST_WILDCARD:
            case_alt->altCon = NULL;
            break;

        case NECRO_AST_CONSTRUCTOR:
            case_alt->altCon = necro_transform_data_constructor(core_transform, alt->pat);
            assert(case_alt->altCon->expr_type == NECRO_CORE_EXPR_DATA_CON);
            break;

        case NECRO_AST_CONID:
            {
                case_alt->altCon = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
                case_alt->altCon->expr_type = NECRO_CORE_EXPR_DATA_CON;

                NecroCoreAST_DataCon* core_datacon = &case_alt->altCon->data_con;
                // core_datacon->condid.id = alt->pat->conid.id; // TODO: CONVERT / FINISH! Core system needs to be converted to NecroAstSymbol system!!!!!!
                // core_datacon->condid.symbol = alt->pat->conid.symbol; // TODO: CONVERT / FINISH! Needs to be converted to NecroAstSymbol system!!!!!!
                core_datacon->next = NULL;
                core_datacon->arg_list = NULL;
            }
            break;

        case NECRO_AST_TUPLE:
            case_alt->altCon = necro_transform_tuple(core_transform, alt->pat);
            break;

        default:
            printf("necro_transform_case pattern type not implemented!: %d\n", alt->pat->type);
            assert(false && "necro_transform_case pattern type not implemented!");
            return NULL;
        }

        if (last_case_alt)
        {
            last_case_alt->next = case_alt;
        }
        else
        {
            core_case->alts = case_alt;
        }

        list_node = list_node->list.next_item;
    }

    return core_expr;
}

NecroCoreAST_Expression* necro_transform_variable(NecroTransformToCore* core_transform, NecroAst* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_VARIABLE);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    NecroCoreAST_Expression* core_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    core_expr->expr_type = NECRO_CORE_EXPR_VAR;
    // core_expr->var.id = necro_ast_node->variable.id; // TODO: CONVERT / FINISH! Core system needs to be converted to NecroAstSymbol system!!!!!!
    // core_expr->var.symbol = necro_ast_node->variable.symbol; // TODO: CONVERT / FINISH! Core system needs to be converted to NecroAstSymbol system!!!!!!
    return core_expr;
}

NecroCoreAST_Expression* necro_transform_to_core_impl(NecroTransformToCore* core_transform, NecroAst* necro_ast_node)
{
    assert(necro_ast_node);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    switch (necro_ast_node->type)
    {
    case NECRO_AST_BIN_OP:
        return necro_transform_bin_op(core_transform, necro_ast_node);

    case NECRO_AST_IF_THEN_ELSE:
        return necro_transform_if_then_else(core_transform, necro_ast_node);

    case NECRO_AST_TOP_DECL:
        return necro_transform_top_decl(core_transform, necro_ast_node);

    case NECRO_AST_SIMPLE_ASSIGNMENT:
        return necro_transform_simple_assignment(core_transform, necro_ast_node);

    case NECRO_AST_APATS_ASSIGNMENT:
        return necro_transform_apats_assignment(core_transform, necro_ast_node);

    case NECRO_AST_PAT_ASSIGNMENT:
        return necro_transform_pat_assignment(core_transform, necro_ast_node);

    case NECRO_AST_RIGHT_HAND_SIDE:
        return necro_transform_right_hand_side(core_transform, necro_ast_node);

    case NECRO_AST_LET_EXPRESSION:
        return necro_transform_let(core_transform, necro_ast_node);

    case NECRO_AST_FUNCTION_EXPRESSION:
        return necro_transform_function_expression(core_transform, necro_ast_node);

    case NECRO_AST_CONSTANT:
        return necro_transform_constant(core_transform, necro_ast_node);

    case NECRO_AST_LAMBDA:
        return necro_transform_lambda(core_transform, necro_ast_node);

    case NECRO_AST_EXPRESSION_LIST:
        return necro_transform_expression_list(core_transform, necro_ast_node);

    case NECRO_AST_TUPLE:
        return necro_transform_tuple(core_transform, necro_ast_node);

    case NECRO_AST_CASE:
        return necro_transform_case(core_transform, necro_ast_node);

    case NECRO_AST_VARIABLE:
        return necro_transform_variable(core_transform, necro_ast_node);

    case NECRO_AST_DATA_DECLARATION:
        return necro_transform_data_decl(core_transform, necro_ast_node);

    case NECRO_AST_CONSTRUCTOR:
        return necro_transform_data_constructor(core_transform, necro_ast_node);

    case NECRO_AST_CONID:
        return necro_transform_conid(core_transform, necro_ast_node);

    case NECRO_AST_TYPE_SIGNATURE:
        // NOTE: Nothing to do here
        return NULL;

    default:
        printf("necro_transform_to_core transforming AST type unimplemented!: %d\n", necro_ast_node->type);
        assert(false && "necro_transform_to_core transforming AST type unimplemented!\n");
        break;
    }

    assert(false && "necro_transform_to_core transforming AST type unimplemented!\n");
    return NULL;
}

void necro_transform_to_core(NecroTransformToCore* core_transform)
{
    assert(core_transform);
    assert(core_transform->necro_ast);
    assert(core_transform->necro_ast->root);
    core_transform->transform_state = NECRO_SUCCESS;
    core_transform->core_ast->root = necro_transform_to_core_impl(core_transform, core_transform->necro_ast->root);
}

NecroCoreAST necro_empty_core_ast()
{
    return (NecroCoreAST)
    {
        .arena = necro_paged_arena_empty(),
        .root  = NULL,
    };
}

void necro_destroy_core_ast(NecroCoreAST* ast)
{
    necro_paged_arena_destroy(&ast->arena);
    ast->root = NULL;
}
