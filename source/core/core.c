/* Copyright (C) Chad McKinney and Curtis McKinney - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */

#include "core.h"

#define NECRO_CORE_DEBUG 0
#if NECRO_CORE_DEBUG
#define TRACE_CORE(...) printf(__VA_ARGS__)
#else
#define TRACE_CORE(...)
#endif

static inline print_tabs(uint32_t num_tabs)
{
    for (uint32_t i = 0; i < num_tabs; ++i)
    {
        printf(STRING_TAB);
    }
}

void necro_print_data_con(NecroCoreAST_DataCon* data_con, NecroIntern* intern, uint32_t depth);
void necro_print_core_node(NecroCoreAST_Expression* ast_node, NecroIntern* intern, uint32_t depth);

void necro_print_data_con_arg(NecroCoreAST_DataExpr* data_con_arg, NecroIntern* intern, uint32_t depth)
{
    print_tabs(depth);
    switch (data_con_arg->dataExpr_type)
    {
    case NECRO_CORE_DATAEXPR_DATACON:
        necro_print_data_con(data_con_arg->dataCon, intern, depth + 1);
        break;

    case NECRO_CORE_DATAEXPR_EXPR:
        necro_print_core_node(data_con_arg->expr, intern, depth + 1);
        break;

    default:
        assert(false && "Unexpected data expr type!?");
        break;
    }
}

void necro_print_data_con(NecroCoreAST_DataCon* data_con, NecroIntern* intern, uint32_t depth)
{
    print_tabs(depth);
    printf("(DataCon: %s)", necro_intern_get_string(intern, data_con->condid.symbol));
    NecroCoreAST_DataExpr* data_con_arg = &data_con->arg_list;
    while (data_con_arg)
    {
        necro_print_data_con_arg(data_con_arg, intern, depth + 1);
        data_con_arg = data_con_arg->next;
    }
}

void necro_print_core_node(NecroCoreAST_Expression* ast_node, NecroIntern* intern, uint32_t depth)
{
    assert(ast_node);
    assert(intern);
    print_tabs(depth);

    switch (ast_node->expr_type)
    {
    case NECRO_CORE_EXPR_VAR:
        printf("(Var: %s, %d)\n", necro_intern_get_string(intern, ast_node->var.symbol), ast_node->var.id);
        break;

    case NECRO_CORE_EXPR_BIND:
        printf("(Bind: %s, %d)\n", necro_intern_get_string(intern, ast_node->bind.var.symbol), ast_node->bind.var.id);
        necro_print_core_node(ast_node->bind.expr, intern, depth + 1);
        break;

    case NECRO_CORE_EXPR_LIT:
        {
            switch (ast_node->lit.type)
            {
            case NECRO_AST_CONSTANT_FLOAT:
                printf("(%f)\n", ast_node->lit.double_literal);
                break;
            case NECRO_AST_CONSTANT_INTEGER:
    #if WIN32
                printf("(%lli)\n", ast_node->lit.int_literal);
    #else
                printf("(%li)\n", ast_node->constant.int_literal);
    #endif
                break;
            case NECRO_AST_CONSTANT_STRING:
                {
                    const char* string = necro_intern_get_string(intern, ast_node->lit.symbol);
                    if (string)
                        printf("(\"%s\")\n", string);
                }
                break;
            case NECRO_AST_CONSTANT_CHAR:
                printf("(\'%c\')\n", ast_node->lit.char_literal);
                break;
            case NECRO_AST_CONSTANT_BOOL:
                printf("(%s)\n", ast_node->lit.boolean_literal ? " True" : "False");
                break;
            }
        }
        break;

    case NECRO_CORE_EXPR_APP:
        puts("(App)");
        necro_print_core_node(ast_node->app.exprA, intern, depth + 1);
        necro_print_core_node(ast_node->app.exprB, intern, depth + 1);
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

                char s_alt_con[512];
                switch (alt->altCon.altCon_type)
                {
                case NECRO_CORE_CASE_ALT_DATA:
                    sprintf_s(s_alt_con, 512, "Data (Unimplemented!!!)");
                    break;

                case NECRO_CORE_CASE_ALT_LITERAL:
                    switch (alt->altCon.lit.type)
                    {
                    case NECRO_AST_CONSTANT_FLOAT:
                        sprintf_s(s_alt_con, 512, "(%f)", alt->altCon.lit.double_literal);
                        break;
                    case NECRO_AST_CONSTANT_INTEGER:
#if WIN32
                        sprintf_s(s_alt_con, 512, "(%lli)", alt->altCon.lit.int_literal);
#else
                        sprintf_s(s_alt_con, 512, "(%li)", alt->altCon.lit.int_literal);
#endif
                        break;
                    case NECRO_AST_CONSTANT_STRING:
                    {
                        const char* string = necro_intern_get_string(intern, alt->altCon.lit.symbol);
                        if (string)
                            sprintf_s(s_alt_con, 512, "(\"%s\")", string);
                        else
                            assert(false);
                    }
                    break;
                    case NECRO_AST_CONSTANT_CHAR:
                        sprintf_s(s_alt_con, 512, "(\'%c\')", alt->altCon.lit.char_literal);
                        break;
                    case NECRO_AST_CONSTANT_BOOL:
                        sprintf_s(s_alt_con, 512, "(%s)", alt->altCon.lit.boolean_literal ? " True" : "False");
                        break;
                    default:
                        assert(false);
                        break;
                    }
                    break;

                case NECRO_CORE_CASE_ALT_DEFAULT:
                    sprintf_s(s_alt_con, 512, "_");
                    break;

                default:
                    assert(false);
                    break;
                }

                printf("(AltCon: %s)\n", s_alt_con);
                necro_print_core_node(alt->expr, intern, depth + 3);
                alt = alt->next;
            }
        }
        break;

    case NECRO_CORE_EXPR_LIST:
        {
            puts("(CORE_EXPR_LIST)");
            NecroCoreAST_List* list_expr = &ast_node->list;
            while (list_expr)
            {
                necro_print_core_node(list_expr->expr, intern, depth + 1);
                list_expr = list_expr->next;
            }
        }
        break;

    case NECRO_CORE_EXPR_DATA:
        {
            printf("(Data %s)", necro_intern_get_string(intern, ast_node->data_decl.data_id.symbol));
            NecroCoreAST_DataCon* con = ast_node->data_decl.con_list;
            while (con)
            {
                necro_print_data_con(ast_node->data_decl.con_list, intern, depth + 1);
                con = con->next;
            }
        }
        break;

    default:
        printf("necro_print_core_node printing expression type unimplemented!: %s\n", core_ast_names[ast_node->expr_type]);
        assert(false);
        break;
    }
}

void necro_print_core(NecroCoreAST* ast, NecroIntern* intern)
{
    assert(ast != NULL);
    necro_print_core_node(ast->root, intern, 0);
}

NecroCoreAST_Expression* necro_transform_to_core_impl(NecroTransformToCore* core_transform, NecroAST_Node_Reified* necro_ast_node);

NecroCoreAST_Expression* necro_transform_bin_op(NecroTransformToCore* core_transform, NecroAST_Node_Reified* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_BIN_OP);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    // 1 + 2 --> (+) 1 2 --> ((+) 1) 2 --> App (App (+) 1) 2
    NecroAST_BinOp_Reified* bin_op = &necro_ast_node->bin_op;

    NecroCoreAST_Expression* inner_core_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    inner_core_expr->expr_type = NECRO_CORE_EXPR_APP;
    NecroCoreAST_Application* inner_core_app = &inner_core_expr->app;

    NecroCoreAST_Expression* var_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    var_expr->expr_type = NECRO_CORE_EXPR_VAR;
    var_expr->var.symbol = bin_op->symbol;
    var_expr->var.id = bin_op->id;

    inner_core_app->exprA = var_expr;
    inner_core_app->exprB = necro_transform_to_core_impl(core_transform, bin_op->lhs);

    NecroCoreAST_Expression* outer_core_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    outer_core_expr->expr_type = NECRO_CORE_EXPR_APP;
    NecroCoreAST_Application* outer_core_app = &outer_core_expr->app;
    outer_core_app->exprA = inner_core_expr;
    outer_core_app->exprB = necro_transform_to_core_impl(core_transform, bin_op->rhs);
    return outer_core_expr;
}

NecroCoreAST_Expression* necro_transform_simple_assignment(NecroTransformToCore* core_transform, NecroAST_Node_Reified* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_SIMPLE_ASSIGNMENT);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    NecroAST_SimpleAssignment_Reified* simple_assignment = &necro_ast_node->simple_assignment;
    NecroCoreAST_Expression* core_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    core_expr->expr_type = NECRO_CORE_EXPR_BIND;
    NecroCoreAST_Bind* core_bind = &core_expr->bind;
    core_bind->var.symbol = simple_assignment->variable_name;
    core_bind->var.id = simple_assignment->id;
    core_bind->expr = necro_transform_to_core_impl(core_transform, simple_assignment->rhs);
    return core_expr;
}

NecroCoreAST_Expression* necro_transform_right_hand_side(NecroTransformToCore* core_transform, NecroAST_Node_Reified* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_RIGHT_HAND_SIDE);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    NecroAST_RightHandSide_Reified* right_hand_side = &necro_ast_node->right_hand_side;
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
            assert(group);
            assert(group->declaration_ast->type == NECRO_AST_SIMPLE_ASSIGNMENT);
            NecroAST_SimpleAssignment_Reified* simple_assignment = &group->declaration_ast->simple_assignment;
            NecroCoreAST_Expression* let_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
            let_expr->expr_type = NECRO_CORE_EXPR_LET;
            NecroCoreAST_Let* let = &let_expr->let;
            let->bind = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
            let->bind->expr_type = NECRO_CORE_EXPR_BIND;
            let->bind->bind.var.id = simple_assignment->id;
            let->bind->bind.var.symbol = simple_assignment->variable_name;
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
            group_list = group_list->next;
        }

        return core_let_expr;
    }
    else
    {
        return necro_transform_to_core_impl(core_transform, right_hand_side->expression);
    }
}

NecroCoreAST_Expression* necro_transform_let(NecroTransformToCore* core_transform, NecroAST_Node_Reified* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_LET_EXPRESSION);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    NecroAST_LetExpression_Reified* reified_ast_let = &necro_ast_node->let_expression;
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
            assert(group);
            assert(group->declaration_ast->type == NECRO_AST_SIMPLE_ASSIGNMENT);
            NecroAST_SimpleAssignment_Reified* simple_assignment = &group->declaration_ast->simple_assignment;
            NecroCoreAST_Expression* let_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
            let_expr->expr_type = NECRO_CORE_EXPR_LET;
            NecroCoreAST_Let* let = &let_expr->let;
            let->bind = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
            let->bind->expr_type = NECRO_CORE_EXPR_BIND;
            let->bind->bind.var.id = simple_assignment->id;
            let->bind->bind.var.symbol = simple_assignment->variable_name;
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
            group_list = group_list->next;
        }

        return core_let_expr;
    }
    else
    {
        assert(false && "Let requires a binding!");
    }
}

NecroCoreAST_Expression* necro_transform_constant(NecroTransformToCore* core_transform, NecroAST_Node_Reified* necro_ast_node)
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

NecroCoreAST_Expression* necro_transform_data_constructor(NecroTransformToCore* core_transform, NecroAST_Node_Reified* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_CONSTRUCTOR);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    NecroAST_Constructor_Reified* ast_constructor = &necro_ast_node->constructor;

    NecroAST_Node_Reified* arg_list = ast_constructor->arg_list;
    while (arg_list)
    {

        arg_list = arg_list->list.next_item;
    }

    return NULL;
}

NecroCoreAST_Expression* necro_transform_data_decl(NecroTransformToCore* core_transform, NecroAST_Node_Reified* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_DATA_DECLARATION);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    NecroAST_DataDeclaration_Reified* data_decl = &necro_ast_node->data_declaration;
    assert(data_decl->simpletype->type == NECRO_AST_SIMPLE_TYPE);
    NecroAST_SimpleType_Reified* simple_type = &data_decl->simpletype->simple_type;
    assert(simple_type->type_con->type == NECRO_AST_CONID);
    NecroAST_ConID_Reified* conid = &simple_type->type_con->conid;

    NecroCoreAST_Expression* core_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    core_expr->expr_type = NECRO_CORE_EXPR_DATA;
    core_expr->data_decl.data_id.id = conid->id;
    core_expr->data_decl.data_id.symbol = conid->symbol;
    
    NecroCoreAST_DataCon* core_data_con = NULL;
    NecroCoreAST_DataCon* current_core_data_con = NULL;

    assert(data_decl->constructor_list->type == NECRO_AST_LIST_NODE);
    NecroAST_ListNode_Reified* list_node = &data_decl->constructor_list->list;
    assert(list_node);
    while (list_node)
    {
        assert(list_node->item->type == NECRO_AST_CONSTRUCTOR);
        NecroAST_Constructor_Reified* con = &list_node->item->constructor;

        NecroCoreAST_DataCon* next_core_data_con = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_DataCon));

        if (core_data_con == NULL)
        {
            core_data_con = current_core_data_con = next_core_data_con;
        }
        else
        {
            current_core_data_con->next = next_core_data_con;
            current_core_data_con = next_core_data_con;
        }

        switch (list_node->item->type)
        {
        case NECRO_AST_CONSTRUCTOR:
            current_core_data_con->arg_list.expr = necro_transform_data_constructor(core_transform, list_node->item);
        default:
            assert(false && "Unexpected type during data declaration core transformation!");
            break;
        }

        current_core_data_con->arg_list.next = NULL;
        list_node = list_node->next_item;
    }

/*
    NecroCoreAST_Expression* con_list_expr = NULL;
    NecroCoreAST_Expression* current_con_list_expr = NULL;

    assert(data_decl->constructor_list->type == NECRO_AST_LIST_NODE);
    NecroAST_ListNode_Reified* list_node = &data_decl->constructor_list->list;
    assert(list_node);
    while (list_node)
    {
        assert(list_node->item->type == NECRO_AST_CONSTRUCTOR);
        NecroAST_Constructor_Reified* con = &list_node->item->constructor;

        NecroCoreAST_Expression* next_con_list_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
        next_con_list_expr->expr_type = NECRO_CORE_EXPR_LIST;

        if (con_list_expr == NULL)
        {
            con_list_expr = current_con_list_expr = next_con_list_expr;
        }
        else
        {
            current_con_list_expr->list.next = next_con_list_expr;
            current_con_list_expr = next_con_list_expr;
        }

        current_con_list_expr->list.expr = necro_transform_data_constructor(core_transform, list_node->item);
        current_con_list_expr->list.next = NULL;
        list_node = list_node->next_item;
    }
    */

    return core_expr;
}

NecroCoreAST_Expression* necro_transform_top_decl(NecroTransformToCore* core_transform, NecroAST_Node_Reified* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_TOP_DECL);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    NecroCoreAST_Expression* top_expression = NULL;
    NecroCoreAST_Expression* next_node = NULL;

    // Data declarations
    {
        NecroAST_Node_Reified* current_decl_node = &necro_ast_node->top_declaration.declaration;
        while (current_decl_node != NULL)
        {
            assert(current_decl_node->type == NECRO_AST_TOP_DECL);
            NecroAST_TopDeclaration_Reified* top_decl = &current_decl_node->top_declaration.declaration;
            if (top_decl->declaration->type == NECRO_AST_DATA_DECLARATION)
            {
                NecroCoreAST_Expression* last_node = next_node;
                NecroCoreAST_Expression* expression = necro_transform_data_decl(core_transform, top_decl->declaration);
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
            }
            current_decl_node = top_decl->next_top_decl;
        }
    }

    // Expression AST
    NecroDeclarationGroupList* group_list = necro_ast_node->top_declaration.group_list;
    assert(group_list);
    while (group_list)
    {
        NecroDeclarationGroup* group = group_list->declaration_group;
        assert(group);
        NecroCoreAST_Expression* last_node = next_node;
        NecroCoreAST_Expression* expression = necro_transform_to_core_impl(core_transform, (NecroAST_Node_Reified*) group->declaration_ast);
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

        group_list = group_list->next;
    }

    assert(top_expression);
    return top_expression;
}

NecroCoreAST_Expression* necro_transform_lambda(NecroTransformToCore* core_transform, NecroAST_Node_Reified* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_CASE);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    // IMPLEMENT THIS!
    assert(false);
}

NecroCoreAST_Expression* necro_transform_case(NecroTransformToCore* core_transform, NecroAST_Node_Reified* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_CASE);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    NecroAST_Case_Reified* case_ast = &necro_ast_node->case_expression;
    NecroCoreAST_Expression* core_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    core_expr->expr_type = NECRO_CORE_EXPR_CASE;
    NecroCoreAST_Case* core_case = &core_expr->case_expr;
    core_case->expr = necro_transform_to_core_impl(core_transform, case_ast->expression);
    core_case->alts = NULL;

    NecroAST_Node_Reified* alt_list_node = case_ast->alternatives;
    NecroAST_ListNode_Reified* list_node = &alt_list_node->list;
    NecroCoreAST_CaseAlt* case_alt = NULL;

    while (list_node)
    {
        NecroAST_Node_Reified* alt_node = list_node->item;
        assert(list_node->item->type == NECRO_AST_CASE_ALTERNATIVE);
        NecroAST_CaseAlternative_Reified* alt = &alt_node->case_alternative;

        NecroCoreAST_CaseAlt* last_case_alt = case_alt;
        case_alt = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_CaseAlt));
        case_alt->expr = necro_transform_to_core_impl(core_transform, alt->body);
        case_alt->next = NULL;

        switch (alt->pat->type)
        {

        case NECRO_AST_CONSTANT:
            case_alt->altCon.altCon_type = NECRO_CORE_CASE_ALT_LITERAL;
            case_alt->altCon.lit = alt->pat->constant;
            break;

        case NECRO_AST_WILDCARD:
            case_alt->altCon.altCon_type = NECRO_CORE_CASE_ALT_DEFAULT;
            case_alt->altCon._defaultPadding = 0;
            break;

        default:
            printf("necro_transform_case pattern type not implemented!: %d\n", alt->pat->type);
            assert(false);
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

        list_node = list_node->next_item;
    }

    return core_expr;
}

NecroCoreAST_Expression* necro_transform_variable(NecroTransformToCore* core_transform, NecroAST_Node_Reified* necro_ast_node)
{
    assert(core_transform);
    assert(necro_ast_node);
    assert(necro_ast_node->type == NECRO_AST_VARIABLE);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    NecroCoreAST_Expression* core_expr = necro_paged_arena_alloc(&core_transform->core_ast->arena, sizeof(NecroCoreAST_Expression));
    core_expr->expr_type = NECRO_CORE_EXPR_VAR;
    core_expr->var.id = necro_ast_node->variable.id;
    core_expr->var.symbol = necro_ast_node->variable.symbol;
    return core_expr;
}

NecroCoreAST_Expression* necro_transform_to_core_impl(NecroTransformToCore* core_transform, NecroAST_Node_Reified* necro_ast_node)
{
    assert(necro_ast_node);
    if (core_transform->transform_state != NECRO_CORE_TRANSFORMING)
        return NULL;

    switch (necro_ast_node->type)
    {
    case NECRO_AST_BIN_OP:
        return necro_transform_bin_op(core_transform, necro_ast_node);

    case NECRO_AST_TOP_DECL:
        return necro_transform_top_decl(core_transform, necro_ast_node);

    case NECRO_AST_SIMPLE_ASSIGNMENT:
        return necro_transform_simple_assignment(core_transform, necro_ast_node);

    case NECRO_AST_RIGHT_HAND_SIDE:
        return necro_transform_right_hand_side(core_transform, necro_ast_node);

    case NECRO_AST_LET_EXPRESSION:
        return necro_transform_let(core_transform, necro_ast_node);

    case NECRO_AST_CONSTANT:
        return necro_transform_constant(core_transform, necro_ast_node);

    case NECRO_AST_LAMBDA:
        return necro_transform_lambda(core_transform, necro_ast_node);

    case NECRO_AST_CASE:
        return necro_transform_case(core_transform, necro_ast_node);

    case NECRO_AST_VARIABLE:
        return necro_transform_variable(core_transform, necro_ast_node);

    default:
        printf("necro_transform_to_core transforming AST type unimplemented!: %d\n", necro_ast_node->type);
        assert(false);
        break;
    }

    assert(false);
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
