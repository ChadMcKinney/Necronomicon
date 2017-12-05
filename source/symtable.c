/* Copyright (C) Chad McKinney and Curtis McKinney - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */

#include <stdio.h>
#include <assert.h>
#include "symtable.h"

// Constants
#define NECRO_SYMTABLE_INITIAL_SIZE 512
#define NECRO_SYMTABLE_NULL_ID      ((NecroID) {0})

NecroSymTable necro_create_symtable(NecroIntern* intern)
{
    NecroSymbolInfo* data = calloc(NECRO_SYMTABLE_INITIAL_SIZE, sizeof(NecroSymbolInfo));
    if (data == NULL)
    {
        fprintf(stderr, "Malloc returned null while allocating data in necro_create_symtable()\n");
        exit(1);
    }
    return (NecroSymTable)
    {
        data,
        NECRO_SYMTABLE_INITIAL_SIZE,
        1,
        intern,
    };
}

void necro_destroy_symtable(NecroSymTable* table)
{
    if (table == NULL || table->data == NULL)
        return;
    free(table->data);
    table->data  = NULL;
    table->size  = 0;
    table->count = 0;
}

NecroSymbolInfo necro_create_initial_symbol_info(NecroSymbol symbol, NecroSourceLoc source_loc, NecroScope* scope)
{
    return (NecroSymbolInfo)
    {
        .name          = symbol,
        .id            = 0,
        .data_size     = 0,
        .type          = {0},
        .local_var_num = 0,
        .source_loc    = source_loc,
        .scope         = scope,
    };
}

inline void necro_symtable_grow(NecroSymTable* table)
{
    table->size *= 2;
    NecroSymbolInfo* new_data = realloc(table->data, table->size * sizeof(NecroSymbolInfo));
    if (new_data == NULL)
    {
        if (table->data != NULL)
            free(table->data);
        fprintf(stderr, "Malloc returned NULL in necro_symtable_grow!\n");
        exit(1);
    }
    table->data = new_data;
    assert(table->data != NULL);
}

NecroID necro_symtable_insert(NecroSymTable* table, NecroSymbolInfo info)
{
    if (table->count >= table->size)
        necro_symtable_grow(table);
    assert(table->count < table->size);
    info.id.id = table->count;
    table->data[table->count] = info;
    table->count++;
    return (NecroID) { table->count - 1 };
}

NecroSymbolInfo* necro_symtable_get(NecroSymTable* table, NecroID id)
{
    if (id.id < table->count)
    {
        return table->data + id.id;
    }
    else
    {
        return NULL;
    }
}

void necro_symtable_info_print(NecroSymbolInfo info, NecroIntern* intern, size_t whitespace)
{
    print_white_space(whitespace);
    printf("NecroSymbolInfo\n");

    print_white_space(whitespace);
    printf("{\n");

    print_white_space(whitespace + 4);
    printf("name:       %s\n",necro_intern_get_string(intern, info.name));

    print_white_space(whitespace + 4);
    printf("id:         %d\n", info.id.id);

    print_white_space(whitespace + 4);
    printf("size:       %d\n", info.data_size);

    print_white_space(whitespace + 4);
    printf("local var:  %d\n", info.local_var_num);

    print_white_space(whitespace + 4);
    printf("source loc: { line: %d, character: %d, pos: %d }\n", info.source_loc.line, info.source_loc.character, info.source_loc.pos);

    print_white_space(whitespace + 4);
    printf("scope:      %p\n", info.scope);

    print_white_space(whitespace);
    printf("}\n");
}

void necro_symtable_print(NecroSymTable* table)
{
    printf("NecroSymTable\n{\n");
    printf("    size:  %d\n", table->size);
    printf("    count: %d\n", table->count);
    printf("    data:\n");
    printf("    [\n");
    for (size_t i = 0; i < table->count; ++i)
    {
        necro_symtable_info_print(table->data[i], table->intern, 8);
    }
    printf("    ]\n");
    printf("}\n");
}

void necro_symtable_test()
{
    puts("---------------------------");
    puts("-- NecroSymTable");
    puts("---------------------------\n");

    NecroIntern     intern       = necro_create_intern();
    NecroSymTable   symtable     = necro_create_symtable(&intern);

    // Symbol 1 test
    NecroSymbol     test_symbol1 = necro_intern_string(&intern, "test1");
    NecroSymbolInfo info1        = { .type = { 0},.name = test_symbol1,.id = { 0 },.data_size = 4 };
    NecroID         id1          = necro_symtable_insert(&symtable, info1);

    // Symbol 2 test
    NecroSymbol     test_symbol2 = necro_intern_string(&intern, "fuck off!");
    NecroSymbolInfo info2        = { .type = { 0},.name = test_symbol2,.id = { 0 },.data_size = 8 };
    NecroID         id2          = necro_symtable_insert(&symtable, info2);

    // necro_symtable_print(&symtable);

    NecroSymbolInfo* info1_test = necro_symtable_get(&symtable, id1);
    if (info1_test != NULL && info1_test->name.id == info1.name.id)
    {
        printf("Symbol1 test: passed\n");
    }
    else
    {
        printf("Symbol1 test: failed\n");
    }

    NecroSymbolInfo* info2_test = necro_symtable_get(&symtable, id2);
    if (info2_test != NULL && info2_test->name.id == info2.name.id)
    {
        printf("Symbol2 test: passed\n");
    }
    else
    {
        printf("Symbol2 test: failed\n");
    }

    necro_destroy_symtable(&symtable);
    necro_destroy_intern(&intern);

    necro_scoped_symtable_test();
}

//=====================================================
// NecroScopedSymTable
//=====================================================
#define NECRO_SCOPE_INITIAL_SIZE 8

inline NecroScope* necro_create_scope(NecroPagedArena* arena, NecroScope* parent)
{
    NecroScope* scope    = necro_paged_arena_alloc(arena, sizeof(NecroScope));
    scope->parent        = parent;
    scope->buckets       = necro_paged_arena_alloc(arena, NECRO_SCOPE_INITIAL_SIZE * sizeof(NecroScopeNode));
    scope->size          = NECRO_SCOPE_INITIAL_SIZE;
    scope->count         = 0;
    memset(scope->buckets, 0, NECRO_SCOPE_INITIAL_SIZE * sizeof(NecroScopeNode));
    return scope;
}

NecroScopedSymTable necro_create_scoped_symtable(NecroSymTable* global_table)
{
    NecroPagedArena arena      = necro_create_paged_arena();
    NecroScope*     top_scope  = necro_create_scope(&arena, NULL);
    NecroScope*     type_scope = necro_create_scope(&arena, NULL);
    return (NecroScopedSymTable)
    {
        .arena         = arena,
        .global_table  = global_table,
        .current_scope = top_scope,
        .top_scope     = top_scope,
        .type_scope    = type_scope
    };
}

void necro_destroy_scoped_symtable(NecroScopedSymTable* table)
{
    necro_destroy_paged_arena(&table->arena);
}

void necro_scoped_symtable_new_scope(NecroScopedSymTable* table)
{
    table->current_scope = necro_create_scope(&table->arena, table->current_scope);
}

void necro_scoped_symtable_pop_scope(NecroScopedSymTable* table)
{
    table->current_scope = table->current_scope->parent;
}

void necro_scoped_symtable_new_type_scope(NecroScopedSymTable* table)
{
    table->current_scope = necro_create_scope(&table->arena, table->current_scope);
    if (table->current_scope->parent == table->top_scope)
        table->current_scope->parent = table->type_scope;
}

void necro_scope_insert(NecroScope* scope, NecroSymbol symbol, NecroID id, NecroPagedArena* arena);

inline void necro_scope_grow(NecroScope* scope, NecroPagedArena* arena)
{
    assert(scope != NULL);
    assert(scope->buckets != NULL);
    assert(scope->count < scope->size);
    NecroScopeNode* prev_buckets = scope->buckets;
    size_t          prev_size    = scope->size;
    size_t          prev_count   = scope->count;
    scope->size                 *= 2;
    scope->buckets               = necro_paged_arena_alloc(arena, scope->size * sizeof(NecroScopeNode));
    scope->count                 = 0;
    memset(scope->buckets, 0, scope->size * sizeof(NecroScopeNode));
    for (size_t bucket = 0; bucket < prev_size; ++bucket)
    {
        NecroID     id     = prev_buckets[bucket].id;
        NecroSymbol symbol = prev_buckets[bucket].symbol;
        if (id.id == NECRO_SYMTABLE_NULL_ID.id)
            continue;
        necro_scope_insert(scope, symbol, id, arena);
    }
    assert(scope->count == prev_count);
    // Leak prev_buckets since we're using an arena (which will free it later) and we care more about speed than memory conservation during this point
}

void necro_scope_insert(NecroScope* scope, NecroSymbol symbol, NecroID id, NecroPagedArena* arena)
{
    assert(scope != NULL);
    assert(scope->buckets != NULL);
    assert(scope->count < scope->size);
    if (scope->count >= scope->size / 2)
        necro_scope_grow(scope, arena);
    size_t bucket = symbol.hash & (scope->size - 1);
    while (scope->buckets[bucket].id.id != NECRO_SYMTABLE_NULL_ID.id)
    {
        if (scope->buckets[bucket].symbol.id == symbol.id)
            break;
        bucket = (bucket + 1) & (scope->size - 1);
    }
    scope->buckets[bucket].symbol = symbol;
    scope->buckets[bucket].id     = id;
    scope->count++;
}

NecroID necro_this_scope_find(NecroScope* scope, NecroSymbol symbol)
{
    assert(scope != NULL);
    assert(scope->buckets != NULL);
    assert(scope->count < scope->size);

    for (size_t bucket = symbol.hash & (scope->size - 1); scope->buckets[bucket].id.id != NECRO_SYMTABLE_NULL_ID.id; bucket = (bucket + 1) & (scope->size - 1))
    {
        if (scope->buckets[bucket].symbol.id == symbol.id)
            return scope->buckets[bucket].id;
    }

    return NECRO_SYMTABLE_NULL_ID;
}

NecroID necro_scope_find(NecroScope* scope, NecroSymbol symbol)
{
    NecroID     id            = NECRO_SYMTABLE_NULL_ID;
    NecroScope* current_scope = scope;
    while (current_scope != NULL)
    {
        id = necro_this_scope_find(current_scope, symbol);
        if (id.id != NECRO_SYMTABLE_NULL_ID.id)
        {
            return id;
        }
        current_scope = current_scope->parent;
    }
    return id;
}

NecroID necro_scoped_symtable_new_symbol_info(NecroScopedSymTable* table, NecroScope* scope, NecroSymbolInfo info)
{
    NecroID id = necro_symtable_insert(table->global_table, info);
    necro_scope_insert(scope, info.name, id, &table->arena);
    return id;
}

void necro_scope_set_last_introduced_id(NecroScope* scope, NecroID last_introduced_id)
{
    scope->last_introduced_id = last_introduced_id;
}

void necro_build_scopes_go(NecroScopedSymTable* scoped_symtable, NecroAST_Node_Reified* input_node)
{
    if (input_node == NULL || scoped_symtable->error.return_code == NECRO_ERROR)
        return;
    input_node->scope = scoped_symtable->current_scope;
    switch (input_node->type)
    {
    case NECRO_AST_UNDEFINED:
        break;
    case NECRO_AST_CONSTANT:
        break;
    case NECRO_AST_UN_OP:
        break;
    case NECRO_AST_BIN_OP:
        necro_build_scopes_go(scoped_symtable, input_node->bin_op.lhs);
        necro_build_scopes_go(scoped_symtable, input_node->bin_op.rhs);
        break;
    case NECRO_AST_IF_THEN_ELSE:
        necro_build_scopes_go(scoped_symtable, input_node->if_then_else.if_expr);
        necro_build_scopes_go(scoped_symtable, input_node->if_then_else.then_expr);
        necro_build_scopes_go(scoped_symtable, input_node->if_then_else.else_expr);
        break;
    case NECRO_AST_TOP_DECL:
        necro_build_scopes_go(scoped_symtable, input_node->top_declaration.declaration);
        necro_build_scopes_go(scoped_symtable, input_node->top_declaration.next_top_decl);
        break;
    case NECRO_AST_DECL:
        necro_build_scopes_go(scoped_symtable, input_node->declaration.declaration_impl);
        necro_build_scopes_go(scoped_symtable, input_node->declaration.next_declaration);
        break;
    case NECRO_AST_SIMPLE_ASSIGNMENT:
        necro_scoped_symtable_new_scope(scoped_symtable);
        necro_build_scopes_go(scoped_symtable, input_node->simple_assignment.rhs);
        necro_scoped_symtable_pop_scope(scoped_symtable);
        break;
    case NECRO_AST_APATS_ASSIGNMENT:
        necro_scoped_symtable_new_scope(scoped_symtable);
        necro_build_scopes_go(scoped_symtable, input_node->apats_assignment.apats);
        necro_build_scopes_go(scoped_symtable, input_node->apats_assignment.rhs);
        necro_scoped_symtable_pop_scope(scoped_symtable);
        break;
    case NECRO_AST_RIGHT_HAND_SIDE:
        necro_build_scopes_go(scoped_symtable, input_node->right_hand_side.declarations);
        necro_build_scopes_go(scoped_symtable, input_node->right_hand_side.expression);
        break;
    case NECRO_AST_LET_EXPRESSION:
        necro_scoped_symtable_new_scope(scoped_symtable);
        necro_build_scopes_go(scoped_symtable, input_node->let_expression.declarations);
        necro_build_scopes_go(scoped_symtable, input_node->let_expression.expression);
        necro_scoped_symtable_pop_scope(scoped_symtable);
        break;
    case NECRO_AST_FUNCTION_EXPRESSION:
        necro_build_scopes_go(scoped_symtable, input_node->fexpression.aexp);
        necro_build_scopes_go(scoped_symtable, input_node->fexpression.next_fexpression);
        break;
    case NECRO_AST_VARIABLE:
        break;
    case NECRO_AST_APATS:
        necro_build_scopes_go(scoped_symtable, input_node->apats.apat);
        necro_build_scopes_go(scoped_symtable, input_node->apats.next_apat);
        break;
    case NECRO_AST_WILDCARD:
        break;
    case NECRO_AST_LAMBDA:
        necro_scoped_symtable_new_scope(scoped_symtable);
        necro_build_scopes_go(scoped_symtable, input_node->lambda.apats);
        necro_build_scopes_go(scoped_symtable, input_node->lambda.expression);
        necro_scoped_symtable_pop_scope(scoped_symtable);
        break;
    case NECRO_AST_DO:
        // necro_scoped_symtable_new_scope(scoped_symtable);
        necro_build_scopes_go(scoped_symtable, input_node->do_statement.statement_list);
        necro_build_scopes_go(scoped_symtable, input_node->do_statement.statement_list);
        // necro_scoped_symtable_pop_scope(scoped_symtable);
        break;
    case NECRO_AST_LIST_NODE:
        necro_build_scopes_go(scoped_symtable, input_node->list.item);
        necro_build_scopes_go(scoped_symtable, input_node->list.next_item);
        break;
    case NECRO_AST_EXPRESSION_LIST:
        necro_build_scopes_go(scoped_symtable, input_node->expression_list.expressions);
        break;
    case NECRO_AST_TUPLE:
        necro_build_scopes_go(scoped_symtable, input_node->tuple.expressions);
        break;
    case NECRO_BIND_ASSIGNMENT:
        necro_build_scopes_go(scoped_symtable, input_node->bind_assignment.expression);
        break;
    case NECRO_AST_ARITHMETIC_SEQUENCE:
        necro_build_scopes_go(scoped_symtable, input_node->arithmetic_sequence.from);
        necro_build_scopes_go(scoped_symtable, input_node->arithmetic_sequence.then);
        necro_build_scopes_go(scoped_symtable, input_node->arithmetic_sequence.to);
        break;
    case NECRO_AST_CASE:
        necro_build_scopes_go(scoped_symtable, input_node->case_expression.expression);
        necro_build_scopes_go(scoped_symtable, input_node->case_expression.alternatives);
        break;
    case NECRO_AST_CASE_ALTERNATIVE:
        necro_scoped_symtable_new_scope(scoped_symtable);
        necro_build_scopes_go(scoped_symtable, input_node->case_alternative.pat);
        necro_build_scopes_go(scoped_symtable, input_node->case_alternative.body);
        necro_scoped_symtable_pop_scope(scoped_symtable);
        break;
    case NECRO_AST_CONID:
        break;
    case NECRO_AST_TYPE_APP:
        necro_build_scopes_go(scoped_symtable, input_node->type_app.ty);
        necro_build_scopes_go(scoped_symtable, input_node->type_app.next_ty);
        break;
    case NECRO_AST_BIN_OP_SYM:
        necro_build_scopes_go(scoped_symtable, input_node->bin_op_sym.left);
        necro_build_scopes_go(scoped_symtable, input_node->bin_op_sym.op);
        necro_build_scopes_go(scoped_symtable, input_node->bin_op_sym.right);
        break;
    case NECRO_AST_CONSTRUCTOR:
        necro_build_scopes_go(scoped_symtable, input_node->constructor.conid);
        necro_build_scopes_go(scoped_symtable, input_node->constructor.arg_list);
        break;
    case NECRO_AST_SIMPLE_TYPE:
        necro_build_scopes_go(scoped_symtable, input_node->simple_type.type_con);
        necro_build_scopes_go(scoped_symtable, input_node->simple_type.type_var_list);
        break;
    case NECRO_AST_DATA_DECLARATION:
        necro_scoped_symtable_new_type_scope(scoped_symtable);
        necro_build_scopes_go(scoped_symtable, input_node->data_declaration.simpletype);
        necro_build_scopes_go(scoped_symtable, input_node->data_declaration.constructor_list);
        necro_scoped_symtable_pop_scope(scoped_symtable);
        break;
    case NECRO_AST_TYPE_CLASS_CONTEXT:
        necro_build_scopes_go(scoped_symtable, input_node->type_class_context.conid);
        necro_build_scopes_go(scoped_symtable, input_node->type_class_context.varid);
        break;
    case NECRO_AST_TYPE_CLASS_DECLARATION:
        necro_scoped_symtable_new_type_scope(scoped_symtable);
        necro_build_scopes_go(scoped_symtable, input_node->type_class_declaration.context);
        necro_build_scopes_go(scoped_symtable, input_node->type_class_declaration.tycls);
        necro_build_scopes_go(scoped_symtable, input_node->type_class_declaration.tyvar);
        necro_build_scopes_go(scoped_symtable, input_node->type_class_declaration.declarations);
        necro_scoped_symtable_pop_scope(scoped_symtable);
        break;
    case NECRO_AST_TYPE_CLASS_INSTANCE:
        necro_scoped_symtable_new_type_scope(scoped_symtable);
        necro_build_scopes_go(scoped_symtable, input_node->type_class_instance.context);
        necro_build_scopes_go(scoped_symtable, input_node->type_class_instance.qtycls);
        necro_build_scopes_go(scoped_symtable, input_node->type_class_instance.inst);
        necro_build_scopes_go(scoped_symtable, input_node->type_class_instance.declarations);
        necro_scoped_symtable_pop_scope(scoped_symtable);
        break;
    case NECRO_AST_TYPE_SIGNATURE:
        necro_build_scopes_go(scoped_symtable, input_node->type_signature.var);
        necro_scoped_symtable_new_type_scope(scoped_symtable);
        necro_build_scopes_go(scoped_symtable, input_node->type_signature.context);
        necro_build_scopes_go(scoped_symtable, input_node->type_signature.type);
        necro_scoped_symtable_pop_scope(scoped_symtable);
        break;
    default:
        necro_error(&scoped_symtable->error, input_node->source_loc, "Unrecognized AST Node type found: %d", input_node->type);
        break;
    }
}

NECRO_RETURN_CODE necro_build_scopes(NecroScopedSymTable* table, NecroAST_Reified* ast)
{
    table->error.return_code = NECRO_SUCCESS;
    necro_build_scopes_go(table, ast->root);
    return table->error.return_code;
}

void necro_scope_print(NecroScope* scope, size_t whitespace, NecroIntern* intern, NecroSymTable* global_table)
{
    assert(scope != NULL);
    assert(intern != NULL);

    print_white_space(whitespace);
    printf("NecroScope\n");

    print_white_space(whitespace);
    printf("{\n");

    print_white_space(whitespace + 4);
    printf("size:  %d\n", scope->size);

    print_white_space(whitespace + 4);
    printf("count: %d\n", scope->count);

    print_white_space(whitespace + 4);
    printf("data:\n");

    print_white_space(whitespace + 4);
    printf("[\n");

    for (size_t bucket = 0; bucket < scope->size; ++bucket)
    {
        if (scope->buckets[bucket].id.id != NECRO_SYMTABLE_NULL_ID.id)
        {
            // print_white_space(whitespace + 8);
            NecroSymbolInfo info = *necro_symtable_get(global_table, scope->buckets[bucket].id);
            necro_symtable_info_print(info, intern, whitespace + 8);
            // printf("{ name: %s, id: %d }\n", necro_intern_get_string(intern, table->buckets[bucket].symbol), table->buckets[bucket].id.id);
        }
    }

    print_white_space(whitespace + 4);
    printf("]\n");

    print_white_space(whitespace);
    printf("}\n");
}

void necro_scoped_symtable_print(NecroScopedSymTable* table)
{
    assert(table != NULL);
    printf("NecroScopedSymtable\n{\n");
    printf("    scopes (from this leaf only):\n");
    NecroScope* current_scope = table->current_scope;
    while (current_scope != NULL)
    {
        necro_scope_print(current_scope, 8, table->global_table->intern, table->global_table);
        current_scope = current_scope->parent;
    }
    printf("}\n");
}

//=====================================================
// Testing
//=====================================================
void necro_scoped_symtable_test()
{
    necro_announce_phase("NecroScopedSymTable");

    NecroIntern         intern          = necro_create_intern();
    NecroSymTable       symtable        = necro_create_symtable(&intern);
    NecroScopedSymTable scoped_symtable = necro_create_scoped_symtable(&symtable);

    NecroScope*         top_scope       = scoped_symtable.current_scope;
    // necro_scoped_symtable_print(&scoped_symtable);
    // printf("\n");

    // Push Test
    necro_scoped_symtable_new_scope(&scoped_symtable);
    if (scoped_symtable.current_scope->parent != NULL)
        printf("Push test:      passed\n");
    else
        printf("Push test:      FAILED\n");

    // Pop Test
    necro_scoped_symtable_pop_scope(&scoped_symtable);
    if (scoped_symtable.current_scope->parent == NULL && scoped_symtable.current_scope == top_scope)
        printf("Pop test:       passed\n");
    else
        printf("Pop test:       FAILED\n");

    // New / Find Test
    {
        NecroSymbol     test_sym = necro_intern_string(&intern, "pulseDemon");
        NecroSymbolInfo info     = { .name = test_sym,.data_size = 4 };
        NecroID         id       = necro_scoped_symtable_new_symbol_info(&scoped_symtable, scoped_symtable.current_scope, info);
        NecroID         found_id = necro_scope_find(scoped_symtable.current_scope, test_sym);
        if (id.id == found_id.id)
            printf("New/Find test:  passed\n");
        else
            printf("New/Find test:  FAILED\n");
    }

    // Push / New / Find Test
    {
        necro_scoped_symtable_new_scope(&scoped_symtable);
        NecroSymbol     test_sym  = necro_intern_string(&intern, "dragonEngine");
        NecroSymbolInfo info      = { .name = test_sym,.data_size = 8 };
        NecroID         id        = necro_scoped_symtable_new_symbol_info(&scoped_symtable, scoped_symtable.current_scope, info);

        NecroSymbol     test_sym3 = necro_intern_string(&intern, "pulseDemon");
        NecroSymbolInfo info3     = { .name = test_sym3,.data_size = 32 };
        NecroID         id3       = necro_scoped_symtable_new_symbol_info(&scoped_symtable, scoped_symtable.current_scope, info3);

        NecroSymbol     test_sym2 = necro_intern_string(&intern, "AcidicSlime");
        NecroSymbolInfo info2     = { .name = test_sym2,.data_size = 16 };
        NecroID         id2       = necro_scoped_symtable_new_symbol_info(&scoped_symtable, scoped_symtable.current_scope, info2);

        NecroID         found_id  = necro_scope_find(scoped_symtable.current_scope, test_sym);
        NecroID         found_id2 = necro_scope_find(scoped_symtable.current_scope, test_sym2);
        NecroID         found_id3 = necro_scope_find(scoped_symtable.current_scope, test_sym3);

        // necro_scoped_symtable_print(&scoped_symtable);

        if (id.id == found_id.id && id.id != NECRO_SYMTABLE_NULL_ID.id && found_id.id != NECRO_SYMTABLE_NULL_ID.id)
            printf("Push/New test:  passed\n");
        else
            printf("Push/New test:  FAILED\n");
        if (id2.id == found_id2.id && found_id.id != found_id2.id && id.id != id2.id && id2.id != NECRO_SYMTABLE_NULL_ID.id && found_id2.id != NECRO_SYMTABLE_NULL_ID.id)
            printf("Push/New2 test: passed\n");
        else
            printf("Push/New2 test: FAILED\n");
        if (id3.id == found_id3.id && found_id.id != found_id3.id && id.id != id3.id && id3.id != NECRO_SYMTABLE_NULL_ID.id && found_id3.id != NECRO_SYMTABLE_NULL_ID.id)
            printf("Push/New3 test: passed\n");
        else
            printf("Push/New3 test: FAILED\n");

        necro_scoped_symtable_pop_scope(&scoped_symtable);
        NecroID         found_id4 = necro_scope_find(scoped_symtable.current_scope, test_sym);
        NecroID         found_id5 = necro_scope_find(scoped_symtable.current_scope, test_sym2);
        if (found_id4.id == NECRO_SYMTABLE_NULL_ID.id && found_id5.id == NECRO_SYMTABLE_NULL_ID.id)
            printf("Pop/Find test:  passed\n");
        else
            printf("Pop/Find test:  FAILED\n");
    }

    necro_destroy_scoped_symtable(&scoped_symtable);

    scoped_symtable = necro_create_scoped_symtable(&symtable);
    // Grow Test
    {
        bool test_passed = true;
        for (size_t i = 0; i < 64; ++i)
        {
            char buffer[20];
            snprintf(buffer, 20, "grow%d", i);
            NecroSymbol     symbol = necro_intern_string(&intern, buffer);
            NecroSymbolInfo info   = necro_create_initial_symbol_info(symbol, (NecroSourceLoc){ 0, 0, 0 }, NULL);
            NecroID         id     = necro_scoped_symtable_new_symbol_info(&scoped_symtable, scoped_symtable.current_scope, info);
            if (id.id == 0)
            {
                test_passed = false;
                break;
            }
        }
        if (test_passed)
            printf("Grow test:      passed\n");
        else
            printf("Grow test:      FAILED\n");
    }

    necro_destroy_symtable(&symtable);
    necro_destroy_intern(&intern);
}