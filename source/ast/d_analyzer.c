/* Copyright (C) Chad McKinney and Curtis McKinney - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */

#include <stdio.h>
#include <inttypes.h>
#include "intern.h"
#include "type_class.h"
#include "d_analyzer.h"

typedef struct NecroDeclarationsInfo
{
    int32_t                     index;
    NecroDeclarationGroupVector stack;
    NecroDeclarationGroupList*  group_lists;
    NecroDeclarationGroup*      current_group;
} NecroDeclarationsInfo;

typedef struct
{
    NecroDeclarationGroup* current_declaration_group;
    NecroPagedArena*       arena;
    NecroIntern*           intern;
} NecroDependencyAnalyzer;

NecroDeclarationGroup* necro_declaration_group_create(NecroPagedArena* arena, NecroAst* declaration_ast, NecroDeclarationGroup* prev)
{
    NecroDeclarationGroup* declaration_group = necro_paged_arena_alloc(arena, sizeof(NecroDeclarationGroup));
    declaration_group->declaration_ast       = declaration_ast;
    declaration_group->next                  = NULL;
    declaration_group->dependency_list       = NULL;
    declaration_group->info                  = NULL;
    declaration_group->type_checked          = false;
    declaration_group->index                 = -1;
    declaration_group->low_link              = 0;
    declaration_group->on_stack              = false;
    if (prev == NULL)
    {
        return declaration_group;
    }
    else
    {
        prev->next = declaration_group;
        return prev;
    }
}

NecroDeclarationGroup* necro_declaration_group_append(NecroPagedArena* arena, NecroAst* declaration_ast, NecroDeclarationGroup* head)
{
    NecroDeclarationGroup* declaration_group = necro_paged_arena_alloc(arena, sizeof(NecroDeclarationGroup));
    declaration_group->declaration_ast       = declaration_ast;
    declaration_group->next                  = NULL;
    declaration_group->dependency_list       = NULL;
    declaration_group->info                  = NULL;
    declaration_group->type_checked          = false;
    declaration_group->index                 = -1;
    declaration_group->low_link              = 0;
    declaration_group->on_stack              = false;
    if (head == NULL)
        return declaration_group;
    NecroDeclarationGroup* curr = head;
    while (curr->next != NULL)
        curr = curr->next;
    curr->next = declaration_group;
    return head;
}

void necro_declaration_group_append_to_group_in_group_list(NecroPagedArena* arena, NecroDeclarationGroupList* group_list, NecroDeclarationGroup* group_to_append)
{
    UNUSED(arena);
    NecroDeclarationGroup* existing_group = group_list->declaration_group;
    if (existing_group == NULL)
    {
        group_list->declaration_group = group_to_append;
        return;
    }
    NecroDeclarationGroup* curr = existing_group;
    while (curr->next != NULL)
        curr = curr->next;
    curr->next = group_to_append;
}

void necro_declaration_group_prepend_to_group_in_group_list(NecroPagedArena* arena, NecroDeclarationGroupList* group_list, NecroDeclarationGroup* group_to_prepend)
{
    UNUSED(arena);
    assert(group_to_prepend != NULL);
    NecroDeclarationGroup* curr = group_to_prepend;
    while (curr->next != NULL)
        curr = curr->next;
    curr->next = group_list->declaration_group;
    group_list->declaration_group = curr;
}

NecroDeclarationGroupList* necro_declaration_group_list_create(NecroPagedArena* arena, NecroDeclarationGroup* declaration_group, NecroDeclarationGroupList* prev)
{
    NecroDeclarationGroupList* declaration_group_list = necro_paged_arena_alloc(arena, sizeof(NecroDeclarationGroupList));
    declaration_group_list->declaration_group         = declaration_group;
    declaration_group_list->next                      = NULL;
    if (prev == NULL)
    {
        return declaration_group_list;
    }
    else
    {
        prev->next = declaration_group_list;
        return prev;
    }
}

NecroDeclarationGroupList* necro_declaration_group_list_prepend(NecroPagedArena* arena, NecroDeclarationGroup* declaration_group, NecroDeclarationGroupList* next)
{
    NecroDeclarationGroupList* declaration_group_list = necro_paged_arena_alloc(arena, sizeof(NecroDeclarationGroupList));
    declaration_group_list->declaration_group         = declaration_group;
    declaration_group_list->next                      = next;
    return declaration_group_list;
}

NecroDeclarationGroupList* necro_declaration_group_list_append(NecroPagedArena* arena, NecroDeclarationGroup* declaration_group, NecroDeclarationGroupList* head)
{
    NecroDeclarationGroupList* declaration_group_list = necro_paged_arena_alloc(arena, sizeof(NecroDeclarationGroupList));
    declaration_group_list->declaration_group         = declaration_group;
    declaration_group_list->next                      = NULL;
    if (head == NULL)
        return declaration_group_list;
    NecroDeclarationGroupList* curr = head;
    while (curr->next != NULL)
        curr = curr->next;
    curr->next = declaration_group_list;
    return head;
}

NecroDeclarationGroupList* necro_declaration_group_list_get_curr(NecroDeclarationGroupList* group_list)
{
    assert(group_list != NULL);
    while (group_list->next != NULL)
        group_list = group_list->next;
    return group_list;
}

NecroDeclarationsInfo* necro_declaration_info_create(NecroPagedArena* arena)
{
    NecroDeclarationsInfo* info = necro_paged_arena_alloc(arena, sizeof(NecroDeclarationsInfo));
    info->group_lists           = necro_declaration_group_list_create(arena, NULL, NULL);
    info->current_group         = NULL;
    info->stack                 = necro_create_declaration_group_vector();
    info->index                 = 0;
    return info;
}

NecroDependencyList*necro_dependency_list_create(NecroPagedArena* arena, NecroDeclarationGroup* dependency_group, NecroDependencyList* head)
{
    NecroDependencyList* dependency_list = necro_paged_arena_alloc(arena, sizeof(NecroDependencyList));
    dependency_list->dependency_group    = dependency_group;
    dependency_list->next                = NULL;
    if (head == NULL)
        return dependency_list;
    NecroDependencyList* curr = head;
    while (curr->next != NULL)
        curr = curr->next;
    curr->next = dependency_list;
    return curr;
}

///////////////////////////////////////////////////////
// Forward Declarations
///////////////////////////////////////////////////////
void d_analyze_go(NecroDependencyAnalyzer* d_analyzer, NecroAst* ast);

///////////////////////////////////////////////////////
// Dependency Analysis
///////////////////////////////////////////////////////
void necro_strong_connect1(NecroDeclarationGroup* group)
{
    group->index    = group->info->index;
    group->low_link = group->info->index;
    group->info->index++;
    necro_push_declaration_group_vector(&group->info->stack, &group);
    group->on_stack            = true;
    group->info->current_group = group;
}

void necro_strong_connect2(NecroPagedArena* arena, NecroDeclarationGroup* group)
{
    // If we're a root node
    if (group->low_link == group->index)
    {
        if (group->info->group_lists->declaration_group != NULL)
            necro_declaration_group_list_append(arena, NULL, group->info->group_lists);
        NecroDeclarationGroupList* strongly_connected_component = group->info->group_lists;
        while (strongly_connected_component->next != NULL) strongly_connected_component = strongly_connected_component->next;
        NecroDeclarationGroup* w = NULL;
        do
        {
            w = necro_pop_declaration_group_vector(&group->info->stack);
            assert(w != NULL);
            w->on_stack = false;
            necro_declaration_group_prepend_to_group_in_group_list(arena, strongly_connected_component, w);
        } while (w != group);
        assert(strongly_connected_component->declaration_group != NULL);
        assert(strongly_connected_component->declaration_group == group);
    }
}

void d_analyze_var(NecroDependencyAnalyzer* d_analyzer, NecroAstSymbol* ast_symbol)
{
    assert(ast_symbol != NULL);
    if (ast_symbol->declaration_group == NULL) return;
    NecroDeclarationGroup* w = ast_symbol->declaration_group;
    assert(w->info != NULL);
    if (w->info->current_group == NULL)
        w->info->current_group = w;
    NecroDeclarationGroup* v = w->info->current_group;
    assert(v != NULL);
    if (w->index == -1)
    {
        ast_symbol->declaration_group->info->current_group = w;
        d_analyze_go(d_analyzer, w->declaration_ast);
        v->low_link = min(w->low_link, v->low_link);
    }
    else if (w->on_stack)
    {
        v->low_link = min(w->low_link, v->low_link);
        if (w->info->current_group->declaration_ast->type == NECRO_AST_SIMPLE_ASSIGNMENT)
        {
            w->info->current_group->declaration_ast->simple_assignment.is_recursive = true;
            // necro_symtable_get(d_analyzer->symtable, w->info->current_group->declaration_ast->simple_assignment.id)->is_recursive = true;
        }
        else if (w->info->current_group->declaration_ast->type == NECRO_AST_DATA_DECLARATION)
        {
            w->info->current_group->declaration_ast->data_declaration.is_recursive = true;
            // necro_symtable_get(d_analyzer->symtable, w->info->current_group->declaration_ast->data_declaration.simpletype->simple_type.type_con->conid.id)->is_recursive = true;
        }
    }
    ast_symbol->declaration_group->info->current_group = v;
}

void d_analyze_go(NecroDependencyAnalyzer* d_analyzer, NecroAst* ast)
{
    if (ast == NULL)
        return;
    switch (ast->type)
    {

    //=====================================================
    // Declaration type things
    //=====================================================
    case NECRO_AST_TOP_DECL:
    {
        NecroAst*              curr = ast;
        NecroDeclarationsInfo* info = necro_declaration_info_create(d_analyzer->arena);
        NecroDeclarationGroup* temp = NULL;
        //-----------------------------------------
        // Pass 1, assign groups and info
        while (curr != NULL)
        {
            switch (curr->top_declaration.declaration->type)
            {
            case NECRO_AST_SIMPLE_ASSIGNMENT:
                curr->top_declaration.declaration->simple_assignment.declaration_group = curr->top_declaration.declaration->simple_assignment.ast_symbol->declaration_group;
                temp = curr->top_declaration.declaration->simple_assignment.declaration_group;
                while (temp != NULL)
                {
                    temp->info = info;
                    temp = temp->next;
                }
                break;
            case NECRO_AST_APATS_ASSIGNMENT:
                curr->top_declaration.declaration->apats_assignment.declaration_group = curr->top_declaration.declaration->apats_assignment.ast_symbol->declaration_group;
                temp = curr->top_declaration.declaration->apats_assignment.declaration_group;
                while (temp != NULL)
                {
                    temp->info = info;
                    temp = temp->next;
                }
                break;
            case NECRO_AST_PAT_ASSIGNMENT:
                curr->top_declaration.declaration->pat_assignment.declaration_group->info = info;
                break;
            case NECRO_AST_DATA_DECLARATION:
                curr->top_declaration.declaration->data_declaration.declaration_group->info = info;
                break;
            case NECRO_AST_TYPE_CLASS_DECLARATION:
                curr->top_declaration.declaration->type_class_declaration.declaration_group->info = info;
                break;
            case NECRO_AST_TYPE_CLASS_INSTANCE:
                curr->top_declaration.declaration->type_class_instance.declaration_group->info = info;
                break;
            case NECRO_AST_TYPE_SIGNATURE:
                curr->top_declaration.declaration->type_signature.declaration_group->info = info;
                break;
            default:
                assert(false);
                break;
            }
            curr = curr->top_declaration.next_top_decl;
        }

        //-----------------------------------------
        // Pass 2, analyze Data Declarations
        curr = ast;
        while (curr != NULL)
        {
            NECRO_AST_TYPE type = curr->top_declaration.declaration->type;
            if (type == NECRO_AST_DATA_DECLARATION)
                d_analyze_go(d_analyzer, curr->top_declaration.declaration);
            curr = curr->top_declaration.next_top_decl;
        }

        //-----------------------------------------
        // Pass 3, TypeClass Declarations, TypeClass Instances
        curr = ast;
        while (curr != NULL)
        {
            NECRO_AST_TYPE type = curr->top_declaration.declaration->type;
            if (type == NECRO_AST_TYPE_CLASS_DECLARATION ||
                type == NECRO_AST_TYPE_CLASS_INSTANCE)
                d_analyze_go(d_analyzer, curr->top_declaration.declaration);
            curr = curr->top_declaration.next_top_decl;
        }

        //-----------------------------------------
        // Pass 4, analyze Type Signatures
        curr = ast;
        while (curr != NULL)
        {
            NECRO_AST_TYPE type = curr->top_declaration.declaration->type;
            if (type == NECRO_AST_TYPE_SIGNATURE)
                d_analyze_go(d_analyzer, curr->top_declaration.declaration);
            curr = curr->top_declaration.next_top_decl;
        }

        //-----------------------------------------
        // Pass 5, analyze Terms
        curr = ast;
        while (curr != NULL)
        {
            NECRO_AST_TYPE type = curr->top_declaration.declaration->type;
            if (type == NECRO_AST_SIMPLE_ASSIGNMENT ||
                type == NECRO_AST_APATS_ASSIGNMENT  ||
                type == NECRO_AST_PAT_ASSIGNMENT)
                d_analyze_go(d_analyzer, curr->top_declaration.declaration);
            curr = curr->top_declaration.next_top_decl;
        }

        ast->top_declaration.group_list = info->group_lists;
        necro_destroy_declaration_group_vector(&info->stack);
        break;
    }

    case NECRO_AST_DECL:
    {
        NecroAst*          curr = ast;
        NecroDeclarationsInfo* info = necro_declaration_info_create(d_analyzer->arena);
        NecroDeclarationGroup* temp = NULL;
        //-----------------------------------------
        // Pass 1, assign groups and info
        while (curr != NULL)
        {
            switch (curr->declaration.declaration_impl->type)
            {
            case NECRO_AST_SIMPLE_ASSIGNMENT:
                curr->declaration.declaration_impl->simple_assignment.declaration_group = curr->declaration.declaration_impl->simple_assignment.ast_symbol->declaration_group;
                temp = curr->declaration.declaration_impl->simple_assignment.declaration_group;
                while (temp != NULL)
                {
                    temp->info = info;
                    temp = temp->next;
                }
                break;
            case NECRO_AST_APATS_ASSIGNMENT:
                curr->declaration.declaration_impl->apats_assignment.declaration_group = curr->declaration.declaration_impl->apats_assignment.ast_symbol->declaration_group;
                temp = curr->declaration.declaration_impl->apats_assignment.declaration_group;
                while (temp != NULL)
                {
                    temp->info = info;
                    temp = temp->next;
                }
                break;
            case NECRO_AST_PAT_ASSIGNMENT:
                curr->declaration.declaration_impl->pat_assignment.declaration_group->info = info;
                break;
            case NECRO_AST_TYPE_SIGNATURE:
                curr->top_declaration.declaration->type_signature.declaration_group->info = info;
                break;
            default: assert(false); break;
            }
            curr = curr->declaration.next_declaration;
        }
        //-----------------------------------------
        // Pass 2, analyze TypeSignatures
        curr = ast;
        while (curr != NULL)
        {
            NECRO_AST_TYPE type = curr->declaration.declaration_impl->type;
            if (type == NECRO_AST_TYPE_SIGNATURE)
                d_analyze_go(d_analyzer, curr->declaration.declaration_impl);
            curr = curr->declaration.next_declaration;
        }
        //-----------------------------------------
        // Pass 3, analyze everything else
        curr = ast;
        while (curr != NULL)
        {
            NECRO_AST_TYPE type = curr->declaration.declaration_impl->type;
            if (type == NECRO_AST_SIMPLE_ASSIGNMENT ||
                type == NECRO_AST_APATS_ASSIGNMENT  ||
                type == NECRO_AST_PAT_ASSIGNMENT)
                d_analyze_go(d_analyzer, curr->declaration.declaration_impl);
            curr = curr->declaration.next_declaration;
        }
        ast->declaration.group_list = info->group_lists;
        necro_destroy_declaration_group_vector(&info->stack);
        break;
    }

    //=====================================================
    // Assignment type things
    //=====================================================
    case NECRO_AST_SIMPLE_ASSIGNMENT:
    {
        if (ast->simple_assignment.declaration_group->index != -1) return;
        assert(ast->simple_assignment.declaration_group != NULL);
        if (ast->simple_assignment.ast_symbol->optional_type_signature != NULL)
            d_analyze_go(d_analyzer, ast->simple_assignment.ast_symbol->optional_type_signature);
        NecroDeclarationGroup* initial_group = ast->simple_assignment.declaration_group;
        NecroDeclarationGroup* current_group = initial_group;
        while (current_group != NULL)
        {
            // current_group->info = initial_group->info;
            necro_strong_connect1(current_group);
            assert(current_group->declaration_ast->type == NECRO_AST_SIMPLE_ASSIGNMENT);
            d_analyze_go(d_analyzer, current_group->declaration_ast->simple_assignment.initializer);
            d_analyze_go(d_analyzer, current_group->declaration_ast->simple_assignment.rhs);
            initial_group->low_link = min(initial_group->low_link, current_group->low_link);
            current_group = current_group->next;
        }
        current_group = initial_group;
        NecroDeclarationGroup* prev_group = NULL;
        while (current_group != NULL)
        {
            current_group->low_link = initial_group->low_link;
            prev_group              = current_group;
            current_group           = current_group->next;
            prev_group->next        = NULL;
        }
        ast->simple_assignment.declaration_group->info->current_group = ast->simple_assignment.declaration_group;
        necro_strong_connect2(d_analyzer->arena, ast->simple_assignment.declaration_group);
        // necro_symtable_get(d_analyzer->symtable, ast->simple_assignment.id)->ast = ast;
        break;
    }

    case NECRO_AST_APATS_ASSIGNMENT:
    {
        if (ast->apats_assignment.declaration_group->index != -1) return;
        assert(ast->apats_assignment.declaration_group != NULL);
        if (ast->apats_assignment.ast_symbol->optional_type_signature != NULL)
            d_analyze_go(d_analyzer, ast->apats_assignment.ast_symbol->optional_type_signature);
        NecroDeclarationGroup* initial_group = ast->apats_assignment.declaration_group;
        NecroDeclarationGroup* current_group = initial_group;
        while (current_group != NULL)
        {
            // current_group->info = initial_group->info;
            necro_strong_connect1(current_group);
            assert(current_group->declaration_ast->type == NECRO_AST_APATS_ASSIGNMENT);
            d_analyze_go(d_analyzer, current_group->declaration_ast->apats_assignment.apats);
            d_analyze_go(d_analyzer, current_group->declaration_ast->apats_assignment.rhs);
            initial_group->low_link = min(initial_group->low_link, current_group->low_link);
            current_group = current_group->next;
        }
        current_group = initial_group;
        NecroDeclarationGroup* prev_group = NULL;
        while (current_group != NULL)
        {
            current_group->low_link = initial_group->low_link;
            prev_group              = current_group;
            current_group           = current_group->next;
            prev_group->next        = NULL;
        }
        ast->apats_assignment.declaration_group->info->current_group = ast->apats_assignment.declaration_group;
        necro_strong_connect2(d_analyzer->arena, ast->apats_assignment.declaration_group);
        break;
    }

    case NECRO_AST_PAT_ASSIGNMENT:
        if (ast->pat_assignment.declaration_group->index != -1) return;
        assert(ast->pat_assignment.declaration_group != NULL);
        assert(ast->pat_assignment.declaration_group->next == NULL);
        necro_strong_connect1(ast->pat_assignment.declaration_group);
        d_analyze_go(d_analyzer, ast->pat_assignment.rhs);
        necro_strong_connect2(d_analyzer->arena, ast->pat_assignment.declaration_group);
        break;

    case NECRO_AST_DATA_DECLARATION:
        if (ast->data_declaration.declaration_group->index != -1) return;
        assert(ast->data_declaration.declaration_group != NULL);
        assert(ast->data_declaration.declaration_group->next == NULL);
        necro_strong_connect1(ast->data_declaration.declaration_group);
        d_analyze_go(d_analyzer, ast->data_declaration.simpletype);
        d_analyze_go(d_analyzer, ast->data_declaration.constructor_list);
        necro_strong_connect2(d_analyzer->arena, ast->data_declaration.declaration_group);
        break;

    case NECRO_AST_TYPE_CLASS_DECLARATION:
        if (ast->type_class_declaration.declaration_group->index != -1) return;
        assert(ast->type_class_declaration.declaration_group != NULL);
        assert(ast->type_class_declaration.declaration_group->next == NULL);
        d_analyze_go(d_analyzer, ast->type_class_declaration.context);
        necro_strong_connect1(ast->type_class_declaration.declaration_group);
        // d_analyze_go(d_analyzer, ast->type_class_declaration.tycls);
        // d_analyze_go(d_analyzer, ast->type_class_declaration.tyvar);
        d_analyze_go(d_analyzer, ast->type_class_declaration.declarations);
        necro_strong_connect2(d_analyzer->arena, ast->type_class_declaration.declaration_group);
        break;

    case NECRO_AST_TYPE_CLASS_INSTANCE:
        // TODO: Circular reference detection
        if (ast->type_class_instance.declaration_group->index != -1) return;
        assert(ast->type_class_instance.declaration_group != NULL);
        assert(ast->type_class_instance.declaration_group->next == NULL);
        d_analyze_go(d_analyzer, ast->type_class_instance.qtycls);
        necro_strong_connect1(ast->type_class_instance.declaration_group);
        d_analyze_go(d_analyzer, ast->type_class_instance.context);
        // d_analyze_go(d_analyzer, ast->type_class_instance.inst);
        d_analyze_go(d_analyzer, ast->type_class_instance.declarations);
        necro_strong_connect2(d_analyzer->arena, ast->type_class_instance.declaration_group);
        break;

    case NECRO_AST_TYPE_SIGNATURE:
        if (ast->type_signature.declaration_group->index != -1) return;
        assert(ast->type_signature.declaration_group != NULL);
        assert(ast->type_signature.declaration_group->next == NULL);
        necro_strong_connect1(ast->type_signature.declaration_group);
        // d_analyze_go(d_analyzer, ast->type_signature.var);
        d_analyze_go(d_analyzer, ast->type_signature.context);
        d_analyze_go(d_analyzer, ast->type_signature.type);
        necro_strong_connect2(d_analyzer->arena, ast->type_signature.declaration_group);
        break;

    //=====================================================
    // Variable type things
    //=====================================================
    case NECRO_AST_VARIABLE:
        switch (ast->variable.var_type)
        {
        case NECRO_VAR_VAR:
            d_analyze_var(d_analyzer, ast->variable.ast_symbol);
            break;
        case NECRO_VAR_SIG:                  break;
        case NECRO_VAR_DECLARATION:          break;
        case NECRO_VAR_TYPE_VAR_DECLARATION: break;
        case NECRO_VAR_TYPE_FREE_VAR:        break;
        case NECRO_VAR_CLASS_SIG:            break;
        default: assert(false);
        }
        if (ast->variable.initializer != NULL)
            d_analyze_go(d_analyzer, ast->variable.initializer);
        break;

    case NECRO_AST_CONID:
        if (ast->conid.con_type == NECRO_CON_TYPE_VAR)
            d_analyze_var(d_analyzer, ast->conid.ast_symbol);
        break;

    case NECRO_AST_TYPE_CLASS_CONTEXT:
        d_analyze_go(d_analyzer, ast->type_class_context.conid);
        // d_analyze_go(d_analyzer, ast->type_class_context.varid);
        break;

    //=====================================================
    // Other Stuff
    //=====================================================
    case NECRO_AST_UNDEFINED:
        break;
    case NECRO_AST_CONSTANT:
        break;
    case NECRO_AST_UN_OP:
        break;
    case NECRO_AST_BIN_OP:
        d_analyze_go(d_analyzer, ast->bin_op.lhs);
        d_analyze_go(d_analyzer, ast->bin_op.rhs);
        break;
    case NECRO_AST_IF_THEN_ELSE:
        d_analyze_go(d_analyzer, ast->if_then_else.if_expr);
        d_analyze_go(d_analyzer, ast->if_then_else.then_expr);
        d_analyze_go(d_analyzer, ast->if_then_else.else_expr);
        break;
    case NECRO_AST_OP_LEFT_SECTION:
        d_analyze_go(d_analyzer, ast->op_left_section.left);
        break;
    case NECRO_AST_OP_RIGHT_SECTION:
        d_analyze_go(d_analyzer, ast->op_right_section.right);
        break;
    case NECRO_AST_RIGHT_HAND_SIDE:
        d_analyze_go(d_analyzer, ast->right_hand_side.declarations);
        d_analyze_go(d_analyzer, ast->right_hand_side.expression);
        break;
    case NECRO_AST_LET_EXPRESSION:
        d_analyze_go(d_analyzer, ast->let_expression.declarations);
        d_analyze_go(d_analyzer, ast->let_expression.expression);
        break;
    case NECRO_AST_FUNCTION_EXPRESSION:
        d_analyze_go(d_analyzer, ast->fexpression.aexp);
        d_analyze_go(d_analyzer, ast->fexpression.next_fexpression);
        break;
    case NECRO_AST_APATS:
        d_analyze_go(d_analyzer, ast->apats.apat);
        d_analyze_go(d_analyzer, ast->apats.next_apat);
        break;
    case NECRO_AST_WILDCARD:
        break;
    case NECRO_AST_LAMBDA:
        d_analyze_go(d_analyzer, ast->lambda.apats);
        d_analyze_go(d_analyzer, ast->lambda.expression);
        break;
    case NECRO_AST_DO:
        d_analyze_go(d_analyzer, ast->do_statement.statement_list);
        break;
    case NECRO_AST_LIST_NODE:
        d_analyze_go(d_analyzer, ast->list.item);
        d_analyze_go(d_analyzer, ast->list.next_item);
        break;
    case NECRO_AST_EXPRESSION_LIST:
        d_analyze_go(d_analyzer, ast->expression_list.expressions);
        break;
    case NECRO_AST_EXPRESSION_ARRAY:
        d_analyze_go(d_analyzer, ast->expression_array.expressions);
        break;
    case NECRO_AST_PAT_EXPRESSION:
        d_analyze_go(d_analyzer, ast->pattern_expression.expressions);
        break;
    case NECRO_AST_TUPLE:
        d_analyze_go(d_analyzer, ast->tuple.expressions);
        break;
    case NECRO_BIND_ASSIGNMENT:
        d_analyze_go(d_analyzer, ast->bind_assignment.expression);
        break;
    case NECRO_PAT_BIND_ASSIGNMENT:
        d_analyze_go(d_analyzer, ast->pat_bind_assignment.pat);
        d_analyze_go(d_analyzer, ast->pat_bind_assignment.expression);
        break;
    case NECRO_AST_ARITHMETIC_SEQUENCE:
        d_analyze_go(d_analyzer, ast->arithmetic_sequence.from);
        d_analyze_go(d_analyzer, ast->arithmetic_sequence.then);
        d_analyze_go(d_analyzer, ast->arithmetic_sequence.to);
        break;
    case NECRO_AST_CASE:
        d_analyze_go(d_analyzer, ast->case_expression.expression);
        d_analyze_go(d_analyzer, ast->case_expression.alternatives);
        break;
    case NECRO_AST_CASE_ALTERNATIVE:
        d_analyze_go(d_analyzer, ast->case_alternative.pat);
        d_analyze_go(d_analyzer, ast->case_alternative.body);
        break;
    case NECRO_AST_TYPE_APP:
        d_analyze_go(d_analyzer, ast->type_app.ty);
        d_analyze_go(d_analyzer, ast->type_app.next_ty);
        break;
    case NECRO_AST_BIN_OP_SYM:
        d_analyze_go(d_analyzer, ast->bin_op_sym.left);
        d_analyze_go(d_analyzer, ast->bin_op_sym.op);
        d_analyze_go(d_analyzer, ast->bin_op_sym.right);
        break;
    case NECRO_AST_CONSTRUCTOR:
        d_analyze_go(d_analyzer, ast->constructor.conid);
        d_analyze_go(d_analyzer, ast->constructor.arg_list);
        break;
    case NECRO_AST_SIMPLE_TYPE:
        d_analyze_go(d_analyzer, ast->simple_type.type_con);
        d_analyze_go(d_analyzer, ast->simple_type.type_var_list);
        break;
    case NECRO_AST_FUNCTION_TYPE:
        d_analyze_go(d_analyzer, ast->function_type.type);
        d_analyze_go(d_analyzer, ast->function_type.next_on_arrow);
        break;

    default:
        assert(false);
        break;
    }
}

void necro_dependency_analyze(NecroCompileInfo info, NecroIntern* intern, NecroAstArena* ast_arena)
{
    NecroDependencyAnalyzer d_analyzer =
    {
        .intern = intern,
        .arena  = &ast_arena->arena,
    };
    d_analyze_go(&d_analyzer, ast_arena->root);
    if (info.compilation_phase == NECRO_PHASE_DEPENDENCY_ANALYSIS && info.verbosity > 0)
        necro_ast_arena_print(ast_arena);
}