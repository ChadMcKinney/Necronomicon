/* Copyright (C) Chad McKinney and Curtis McKinney - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */

#include <stdio.h>
#include <inttypes.h>
#include <stdarg.h>
#include "type_class.h"
#include "infer.h"
#include "symtable.h"
#include "kind.h"
#include "type/prim.h"
#include "type/derive.h"
#include "base.h"

/*
TODO:
    - Pattern literal cleanup: Require Eq for pattern literals, also make sure it's getting type class translated correctly
    - Require type signature for all top level bindings?!?!?
    - Numeric patterns seem messed up!?
    - Make sure nested super dictionaries are added and applied in correct order!!!
    - Make sure dictionaries get renamed correctly to get passed correct ID!!!!
    - Make sure selector argument ordering / forwarding IS CORRECT!!!!
    - Right now making sure the order of dictionaries and arguments lines up correctly!!!!
    - After translation some ids for added AST nodes are coming back as 0
    - Make sure we aren't duplicating names / scopes
    - Test deeply nested class dictionary passing!
BACKBURNER
    - Follow Haskell's lead: Make top declaration's default to Haskell style monomorphism restriction, and where bindings default to not generalizing (unless type signature is given!!!)
    - Better unification error messaging! This is especially true when it occurs during SuperClass constraint checking. Make necro_unify return struct with either Success or Error data
    - Can we collapse env into symtable!?!?!
    - Can we collide symtable and env IDs like this!
    - optimize necro_get_type_class_instance
    - Defaulting
    - Make context ALWAYS a list, even if it's just one item...
    - split NECRO_DECLARATION into:
        case NECRO_VAR_PATTERN?
        case NECRO_VAR_ARG?
    - Do statements
    - Perhaps drop Monads!?!?! Is that crazy? feels like a heavy handed solution to some specific use cases...
*/

#define NECRO_TYPE_CLASS_DEBUG 0
#if NECRO_TYPE_CLASS_DEBUG
#define TRACE_TYPE_CLASS(...) printf(__VA_ARGS__)
#else
#define TRACE_TYPE_CLASS(...)
#endif

#define SMALL_VEC_SIZE 8

//=====================================================
// Forward Declarations
//=====================================================
bool        necro_constrain_class_variable_check(NecroAstSymbol* type_var, NecroAstSymbol* type_sig_symbol, NecroTypeClassContext* context);
NecroType*  necro_instantiate_method_sig(NecroInfer* infer, NecroAstSymbol* type_class_var, NecroType* method_type, NecroType* data_type);
NecroSymbol necro_create_dictionary_name(NecroIntern* intern, NecroSymbol type_class_name);
NecroSymbol necro_create_selector_name(NecroIntern* intern, NecroSymbol member_name, NecroSymbol dictionary_name);
NecroSymbol necro_create_dictionary_instance_name(NecroIntern* intern, NecroSymbol type_class_name, NecroAst* type_class_instance_ast);
void        necro_rec_check_pat_assignment(NecroInfer* infer, NecroAst* ast);

//=====================================================
// Type Class Instances
//=====================================================
void necro_apply_constraints(NecroPagedArena* arena, NecroType* type, NecroTypeClassContext* context)
{
    if (type == NULL) return;
    if (context == NULL) return;
    switch (type->type)
    {
    case NECRO_TYPE_VAR:
    {
        while (context != NULL)
        {
            if (type->var.var_symbol->name == context->var_symbol->name)
            {
                NecroTypeClassContext* class_context = necro_create_type_class_context(arena, context->type_class, context->class_symbol, context->var_symbol, NULL);
                type->var.context = necro_union_contexts(arena, type->var.context, class_context);
            }
            context = context->next;
        }
        break;
    }
    case NECRO_TYPE_APP:  necro_apply_constraints(arena, type->app.type1, context); necro_apply_constraints(arena, type->app.type2, context); break;
    case NECRO_TYPE_FUN:  necro_apply_constraints(arena, type->fun.type1, context); necro_apply_constraints(arena, type->fun.type2, context); break;
    case NECRO_TYPE_CON:  necro_apply_constraints(arena, type->con.args,  context); break;
    case NECRO_TYPE_LIST: necro_apply_constraints(arena, type->list.item, context); necro_apply_constraints(arena, type->list.next, context); break;
    case NECRO_TYPE_FOR:  assert(false); break;
    default:              assert(false); break;
    }
}

NecroType* necro_sub_method_var(NecroMethodSub* subs, size_t sub_count, NecroAstSymbol* var)
{
    for (size_t i = 0; i < sub_count; ++i)
    {
        if (subs[i].old_type_var->name == var->name)
        {
            return subs[i].new_type_var;
        }
    }
    assert(false); // Should be impossible
    return NULL;
}

NecroMethodSub necro_create_method_sub(NecroInfer* infer, NecroAstSymbol* old_type_var, NecroScope* scope)
{
    NecroType* type_var             = necro_new_name(infer);
    // type_var->var.var_symbol->source_name = necro_intern_string(infer->intern, necro_id_as_character_string(infer->intern, type_var->var.var));
    type_var->var.is_rigid          = true;
    type_var->var.bound             = NULL;
    type_var->var.gen_bound         = NULL;
    type_var->var.arity             = -1;
    type_var->var.is_type_class_var = false;
    type_var->var.scope             = scope;
    // TODO: Replace with NecroAstSymbol system!!!
    type_var->kind                  = old_type_var->type->kind;
    return (NecroMethodSub)
    {
        .old_type_var = old_type_var,
        .new_type_var = type_var
    };
}

void necro_create_new_method_for_all(NecroInfer* infer, NecroMethodSub* sub, NecroType* data_type, NecroType** data_type_for_all, NecroType** data_type_for_all_curr, NecroScope* scope)
{
    *sub                                    = necro_create_method_sub(infer, data_type->for_all.var_symbol, scope);
    NecroTypeClassContext* new_context_head = NULL;
    NecroTypeClassContext* new_context_curr = NULL;
    NecroTypeClassContext* old_context      = data_type->for_all.context;
    while (old_context != NULL)
    {
        NecroTypeClassContext* new_context = necro_create_type_class_context(infer->arena, old_context->type_class, old_context->class_symbol, sub->new_type_var->var.var_symbol, NULL);
        if (new_context_head == NULL)
        {
            new_context_head = new_context;
            new_context_curr = new_context_head;
        }
        else
        {
            new_context_curr->next = new_context;
            new_context_curr       = new_context_curr->next;
        }
        old_context = old_context->next;
    }
    sub->new_type_var->var.context = new_context_head;
    NecroType* new_type = necro_type_for_all_create(infer->arena, sub->new_type_var->var.var_symbol, new_context_head, NULL);
    if ((*data_type_for_all) == NULL)
    {
        (*data_type_for_all)      = new_type;
        (*data_type_for_all_curr) = *data_type_for_all;
    }
    else
    {
        (*data_type_for_all_curr)->for_all.type = new_type;
        (*data_type_for_all_curr)               = (*data_type_for_all_curr)->for_all.type;
    }
}

NecroType* necro_instantiate_method_sig_go(NecroInfer* infer, NecroAstSymbol* type_class_var, NecroType* method_type, NecroType* data_type, NecroMethodSub* subs, size_t sub_count)
{
    assert(infer != NULL);
    assert(data_type != NULL);
    if (method_type == NULL) return NULL;
    switch (method_type->type)
    {
    case NECRO_TYPE_VAR:
        if (method_type->var.var_symbol->name == type_class_var->name) // TODO: Replace with direct symbol pointer comparison!
        {
            // inst data_type
            assert(method_type->var.is_type_class_var == true);
            return necro_instantiate_method_sig_go(infer, type_class_var, data_type, data_type, subs, sub_count);
        }
        else
        {
            // sub var
            return necro_sub_method_var(subs, sub_count, method_type->var.var_symbol);
        }
    case NECRO_TYPE_APP:  return necro_type_app_create(infer->arena,  necro_instantiate_method_sig_go(infer, type_class_var, method_type->app.type1, data_type, subs, sub_count), necro_instantiate_method_sig_go(infer, type_class_var, method_type->app.type2, data_type, subs, sub_count));
    case NECRO_TYPE_FUN:  return necro_type_fn_create(infer->arena,  necro_instantiate_method_sig_go(infer, type_class_var, method_type->fun.type1, data_type, subs, sub_count), necro_instantiate_method_sig_go(infer, type_class_var, method_type->fun.type2, data_type, subs, sub_count));
    case NECRO_TYPE_LIST: return necro_type_list_create(infer->arena, necro_instantiate_method_sig_go(infer, type_class_var, method_type->list.item, data_type, subs, sub_count), necro_instantiate_method_sig_go(infer, type_class_var, method_type->list.next, data_type, subs, sub_count));
    case NECRO_TYPE_CON:  return necro_type_con_create(infer->arena, method_type->con.con_symbol, necro_instantiate_method_sig_go(infer, type_class_var, method_type->con.args, data_type, subs, sub_count), method_type->con.arity);
    case NECRO_TYPE_FOR:  assert(false); return NULL;
    default:              assert(false); return NULL;
    }
}

NecroType* necro_instantiate_method_sig(NecroInfer* infer, NecroAstSymbol* type_class_var, NecroType* a_method_type, NecroType* a_data_type)
{
    assert(infer != NULL);
    assert(a_method_type != NULL);
    assert(a_data_type != NULL);

    // Local vars
    NecroType*  data_type_for_all_curr = NULL;
    NecroType*  data_type_for_all      = NULL;
    NecroType*  data_type              = a_data_type;
    NecroType*  method_type            = a_method_type;
    size_t      sub_count              = 0;
    // TODO: This seems very hacky....Replace with something better!
    infer->scoped_symtable->current_type_scope = infer->scoped_symtable->top_type_scope;
    necro_scoped_symtable_new_type_scope(infer->scoped_symtable);
    NecroScope* scope                  = infer->scoped_symtable->current_type_scope;
    necro_scoped_symtable_pop_type_scope(infer->scoped_symtable);

    // Count the number of subs
    while (data_type->type == NECRO_TYPE_FOR)
    {
        sub_count++;
        data_type = data_type->for_all.type;
    }
    while (method_type->type == NECRO_TYPE_FOR)
    {
        if (method_type->for_all.var_symbol->name != type_class_var->name)
            sub_count++;
        method_type = method_type->for_all.type;
    }

    // Alloc subs
    size_t          sub_i = 0;
    // TODO: Replace with NecroSnapshotArena solution!
    NecroMethodSub* subs  = malloc(sub_count * sizeof(NecroMethodSub));

    // reset
    data_type   = a_data_type;
    method_type = a_method_type;

    // Iterate through data_type for_alls
    while (data_type->type == NECRO_TYPE_FOR)
    {
        necro_create_new_method_for_all(infer, subs + sub_i, data_type, &data_type_for_all, &data_type_for_all_curr, scope);
        sub_i++;
        data_type = data_type->for_all.type;
    }

    // Iterate through method_type for_alls
    while (method_type->type == NECRO_TYPE_FOR)
    {
        if (method_type->for_all.var_symbol->name == type_class_var->name)
        {
            method_type = method_type->for_all.type;
            continue;
        }
        necro_create_new_method_for_all(infer, subs + sub_i, method_type, &data_type_for_all, &data_type_for_all_curr, scope);
        sub_i++;
        method_type = method_type->for_all.type;
    }
    assert(sub_i == sub_count);

    // Instanatiate method_sig
    NecroType* inst_method_type = necro_instantiate_method_sig_go(infer, type_class_var, method_type, data_type, subs, sub_count);
    if (data_type_for_all != NULL && data_type_for_all_curr != NULL)
    {
        data_type_for_all_curr->for_all.type = inst_method_type;
    }
    else
    {
        data_type_for_all = inst_method_type;
    }
    data_type_for_all->pre_supplied = true;

    // Cleanup
    free(subs);

    return data_type_for_all;
}

//=====================================================
// Contexts
//=====================================================
bool necro_context_contains_class(NecroTypeClassContext* context, NecroTypeClassContext* type_class)
{
    while (context != NULL)
    {
        if (context->class_symbol->name == type_class->class_symbol->name)
        {
            return true;
        }
        context = context->next;
    }
    return false;
}

bool necro_context_and_super_classes_contain_class(NecroTypeClassContext* context, NecroTypeClassContext* type_class)
{
    while (context != NULL)
    {
        if (context->class_symbol->name == type_class->class_symbol->name)
        {
            return true;
        }
        NecroTypeClassContext* super_classes = context->type_class->context;
        if (necro_context_and_super_classes_contain_class(super_classes, type_class))
        {
            return true;
        }
        context = context->next;
    }
    return false;
}

bool necro_is_subclass(NecroTypeClassContext* maybe_subclass_context, NecroTypeClassContext* maybe_super_class_context)
{
    if (maybe_subclass_context->var_symbol->name != maybe_super_class_context->var_symbol->name)
        return false;
    NecroTypeClassContext* super_classes = maybe_subclass_context->type_class->context;
    return necro_context_and_super_classes_contain_class(super_classes, maybe_super_class_context);
}

bool necro_context_contains_class_and_type_var(NecroTypeClassContext* context, NecroTypeClassContext* type_class, NecroAstSymbol* var_symbol)
{
    while (context != NULL)
    {
        if (context->class_symbol->name == type_class->class_symbol->name && context->var_symbol->name == var_symbol->name)
            return true;
        context = context->next;
    }
    return false;
}

NecroTypeClassContext* necro_remove_super_classes(NecroTypeClassContext* sub_class, NecroTypeClassContext* context)
{
    NecroTypeClassContext* head = context;
    NecroTypeClassContext* curr = head;
    NecroTypeClassContext* prev = NULL;
    while (curr != NULL)
    {
        if (necro_is_subclass(sub_class, curr))
        {
            if (curr == head)
            {
                head = head->next;
            }
            if (prev != NULL)
            {
                prev->next = curr->next;
            }
        }
        else
        {
            prev = curr;
        }
        curr = curr->next;
    }
    return head;
}

NecroTypeClassContext* necro_scrub_super_classes(NecroTypeClassContext* context)
{
    if (context == NULL || context->next == NULL)
        return context;
    NecroTypeClassContext* head = context;
    NecroTypeClassContext* curr = head;
    while (curr != NULL)
    {
        head = necro_remove_super_classes(curr, head);
        curr = curr->next;
    }
    return head;
}

NecroTypeClassContext* necro_union_contexts_to_same_var(NecroPagedArena* arena, NecroTypeClassContext* context1, NecroTypeClassContext* context2, NecroAstSymbol* var_symbol)
{
    NecroTypeClassContext* head = NULL;
    NecroTypeClassContext* curr = NULL;

    // Copy context1
    while (context1 != NULL)
    {
        if (!necro_context_contains_class(head, context1))
        {
            NecroTypeClassContext* new_context = necro_create_type_class_context(arena, context1->type_class, context1->class_symbol, var_symbol, NULL);
            if (curr == NULL)
            {
                head = new_context;
                curr = head;
            }
            else
            {
                curr->next = new_context;
                curr       = curr->next;
            }
        }
        context1 = context1->next;
    }

    // Insert context2 classes if they aren't already in context1
    while (context2 != NULL)
    {
        if (!necro_context_contains_class(head, context2))
        {
            NecroTypeClassContext* new_context = necro_create_type_class_context(arena, context2->type_class, context2->class_symbol, var_symbol, NULL);
            if (curr == NULL)
            {
                head = new_context;
                curr = head;
            }
            else
            {
                curr->next = new_context;
                curr       = curr->next;
            }
        }
        context2 = context2->next;
    }

    // Scrub super classes
    return necro_scrub_super_classes(head);
}

NecroTypeClassContext* necro_union_contexts(NecroPagedArena* arena, NecroTypeClassContext* context1, NecroTypeClassContext* context2)
{
    NecroTypeClassContext* head = NULL;
    NecroTypeClassContext* curr = NULL;

    // Copy context1
    while (context1 != NULL)
    {
        assert(context1->type_class != NULL);
        if (!necro_context_contains_class_and_type_var(head, context1, context1->var_symbol))
        {
            NecroTypeClassContext* new_context = necro_create_type_class_context(arena, context1->type_class, context1->class_symbol, context1->var_symbol, NULL);
            if (curr == NULL)
            {
                head = new_context;
                curr = head;
            }
            else
            {
                curr->next = new_context;
                curr       = curr->next;
            }
        }
        context1 = context1->next;
    }

    // Insert context2 classes if they aren't already in context1
    while (context2 != NULL)
    {
        assert(context2->type_class != NULL);
        if (!necro_context_contains_class_and_type_var(head, context2, context2->var_symbol))
        {
            NecroTypeClassContext* new_context = necro_create_type_class_context(arena, context2->type_class, context2->class_symbol, context2->var_symbol, NULL);
            if (curr == NULL)
            {
                head = new_context;
                curr = head;
            }
            else
            {
                curr->next = new_context;
                curr       = curr->next;
            }
        }
        context2 = context2->next;
    }

    // Scrub super classes
    return necro_scrub_super_classes(head);
}

NecroTypeClassContext* necro_create_type_class_context(NecroPagedArena* arena, NecroTypeClass* type_class, NecroAstSymbol* type_class_name, NecroAstSymbol* type_var, NecroTypeClassContext* next)
{
    NecroTypeClassContext* context = necro_paged_arena_alloc(arena, sizeof(NecroTypeClassContext));
    context->class_symbol          = type_class_name;
    context->var_symbol            = type_var;
    context->type_class            = type_class;
    context->next                  = next;
    return context;
}

bool necro_ambig_occurs(NecroAstSymbol* name, NecroType* type)
{
    if (type == NULL)
        return false;

    type = necro_find(type);
    switch (type->type)
    {
    case NECRO_TYPE_VAR:  return name->name == type->var.var_symbol->name;
    case NECRO_TYPE_APP:  return necro_ambig_occurs(name, type->app.type1) || necro_ambig_occurs(name, type->app.type2);
    case NECRO_TYPE_FUN:  return necro_ambig_occurs(name, type->fun.type1) || necro_ambig_occurs(name, type->fun.type2);
    case NECRO_TYPE_CON:  return necro_ambig_occurs(name, type->con.args);
    case NECRO_TYPE_LIST: return necro_ambig_occurs(name, type->list.item) || necro_ambig_occurs(name, type->list.next);
    case NECRO_TYPE_FOR:  assert(false); break;
    default:              assert(false); break;
    }
    return false;
}

bool necro_ambiguous_type_class_check(NecroAstSymbol* type_sig_name, NecroTypeClassContext* context, NecroType* type)
{
    UNUSED(type_sig_name);
    if (type == NULL)
        return false;
    while (context != NULL)
    {
        if (!necro_ambig_occurs(context->var_symbol, type))
        {
            // TODO: NecroResultError
            // necro_infer_ast_error(infer, type, NULL, "Could not deduce (%s %s) in ambiguity check for \'%s\'",
            //     context->type_class_name.symbol->str,
            //     necro_id_as_character_string(infer->intern, (NecroVar) { .id = context->type_var.id, .symbol = context->type_var.symbol }),
            //     type_sig_name->str);
            return true;
        }
        context = context->next;
    }
    return false;
}

bool necro_constrain_class_variable_check(NecroAstSymbol* type_var, NecroAstSymbol* type_sig_symbol, NecroTypeClassContext* context)
{
    UNUSED(type_sig_symbol);
    while (context != NULL)
    {
        if (context->var_symbol->name == type_var->name)
        {
            // TODO: NecroResultError
            // necro_infer_ast_error(infer, NULL, NULL, "Constraint \'%s %s\' in the type for method \'%s\' constrains only the type class variable",
            //     context->type_class_name.symbol->str,
            //     context->type_var.symbol->str,
            //     type_sig_symbol->str);
            return true;
        }
        context = context->next;
    }
    return false;
}

//=====================================================
// Printing
//=====================================================
void necro_print_type_class_member(NecroTypeClassMember* member, size_t white_count)
{
    if (member == NULL)
        return;
    NecroType* member_type = member->member_varid->type;
    print_white_space(white_count + 4);
    printf("%s :: ", member->member_varid->name->str);
    necro_type_sig_print(member_type);
    necro_print_type_class_member(member->next, white_count);
}

void necro_print_type_class_context(NecroTypeClassContext* context, size_t white_count)
{
    if (context == NULL)
        return;
    if (context->next != NULL)
        printf("%s %s,", context->class_symbol->name->str, context->var_symbol->name->str);
    else
        printf("%s %s", context->class_symbol->name->str, context->var_symbol->name->str);
    necro_print_type_class_context(context->next, white_count);
}

void necro_print_type_class(NecroTypeClass* type_class, size_t white_count)
{
    if (type_class == NULL)
        return;
    print_white_space(white_count);
    printf("class %s %s\n", type_class->type_class_name->source_name->str, type_class->type_var->source_name->str);
    print_white_space(white_count);
    printf("{\n");
    print_white_space(white_count + 4);
    printf("type:    ");
    necro_type_sig_print(type_class->type);
    print_white_space(white_count + 4);
    printf("context: (");
    necro_print_type_class_context(type_class->context, white_count + 4);
    printf(") =>\n");
    print_white_space(white_count + 4);
    printf("members:\n");
    print_white_space(white_count + 4);
    printf("[\n");
    necro_print_type_class_member(type_class->members, white_count + 4);
    print_white_space(white_count + 4);
    printf("]\n");

    // Print dictionary declaration ast
    // printf("dictionary declaration: \n\n");
    // necro_print_reified_ast_node(type_class->ast->type_class_declaration.dictionary_data_declaration, intern);
    // puts("");
    // print_white_space(white_count);

    printf("}\n");
}

void necro_print_dictionary_prototype(NecroDictionaryPrototype* prototype, size_t white_count)
{
    if (prototype == NULL)
        return;
    NecroType* member_type = prototype->prototype_varid->type;
    print_white_space(white_count + 4);
    printf("%s :: ", prototype->prototype_varid->name->str);
    necro_type_sig_print(member_type);
    necro_print_dictionary_prototype(prototype->next, white_count);
}

void necro_print_instance(NecroTypeClassInstance* instance, size_t white_count)
{
    if (instance == NULL)
        return;
    print_white_space(white_count);
    printf("instance %s %s\n", instance->type_class_name->source_name->str, instance->data_type_name->source_name->str);
    print_white_space(white_count);
    printf("{\n");
    print_white_space(white_count + 4);
    printf("context: (");
    necro_print_type_class_context(instance->context, white_count + 4);
    printf(") =>\n");
    print_white_space(white_count + 4);
    printf("data_type: ");
    if (instance->data_type != NULL)
        necro_type_sig_print(instance->data_type);
    else
        printf(" null_type");
    printf("\n");
    print_white_space(white_count + 4);
    printf("dictionary prototype:\n");
    print_white_space(white_count + 4);
    printf("[\n");
    necro_print_dictionary_prototype(instance->dictionary_prototype, white_count + 4);
    print_white_space(white_count + 4);
    printf("]\n");

    // Print dictionary ast
    print_white_space(white_count + 4);
    printf("dictionary: \n\n");
    necro_ast_print(instance->ast->type_class_instance.dictionary_instance);
    puts("");

    print_white_space(white_count);
    printf("}\n");
}

void necro_print_type_class_instance_go(NecroTypeClassInstance* instance)
{
    necro_print_instance(instance, 4);
}

void necro_print_type_classes(NecroInfer* infer)
{
    UNUSED(infer);
    printf("\nTypeClasses\n{\n");
    // TODO: Redo type class printing
    // for (size_t i = 0; i < infer->symtable->count; ++i)
    // {
    //     if (infer->symtable->data[i].type_class != NULL)
    //         necro_print_type_class(infer->symtable->data[i].type_class, 4);
    // }
    printf("}\n");
    printf("\nInstances\n{\n");
    // TODO: Redo type class printing
    // for (size_t i = 0; i < infer->symtable->count; ++i)
    // {
    //     if (infer->symtable->data[i].type_class_instance != NULL)
    //         necro_print_instance(infer->symtable->data[i].type_class_instance, 4);
    // }
    printf("}\n");
}

//=====================================================
// Dictionary Translation
//=====================================================
NecroSymbol necro_create_dictionary_name(NecroIntern* intern, NecroSymbol type_class_name)
{
    return necro_intern_concat_symbols(intern, type_class_name, necro_intern_string(intern, "@Dictionary"));
}

NecroSymbol necro_create_dictionary_arg_name(NecroIntern* intern, NecroSymbol type_class_name, NecroSymbol type_var_name)
{
    // TODO: Make unique names!
    NecroSymbol dictionary_arg_name = necro_intern_concat_symbols(intern, necro_intern_string(intern, "dictionaryArg@"), type_class_name);
    dictionary_arg_name             = necro_intern_concat_symbols(intern, dictionary_arg_name, necro_intern_string(intern, "@"));
    dictionary_arg_name             = necro_intern_concat_symbols(intern, dictionary_arg_name, type_var_name);
    return dictionary_arg_name;
}

NecroSymbol necro_create_selector_name(NecroIntern* intern, NecroSymbol member_name, NecroSymbol dictionary_name)
{
    NecroSymbol   select_symbol = necro_intern_string(intern, "select@");
    NecroSymbol   at_symbol     = necro_intern_string(intern, "@");
    NecroSymbol   selector_name = necro_intern_concat_symbols(intern, select_symbol, member_name);
    selector_name               = necro_intern_concat_symbols(intern, selector_name, at_symbol);
    selector_name               = necro_intern_concat_symbols(intern, selector_name, dictionary_name);
    return selector_name;
}

NecroSymbol necro_create_dictionary_instance_name(NecroIntern* intern, NecroSymbol type_class_name, NecroAst* type_class_instance_ast)
{
    NecroSymbol dictionary_name          = necro_create_dictionary_name(intern, type_class_name);
    NecroSymbol dictionary_symbol        = necro_intern_string(intern, "dictionaryInstance@");
    NecroSymbol dictionary_instance_name = NULL;
    if (type_class_instance_ast->type_class_instance.inst->type == NECRO_AST_CONID)
    {
        dictionary_instance_name = necro_intern_concat_symbols(intern, dictionary_symbol, type_class_instance_ast->type_class_instance.inst->conid.symbol);
    }
    else if (type_class_instance_ast->type_class_instance.inst->type == NECRO_AST_CONSTRUCTOR)
    {
        dictionary_instance_name = necro_intern_concat_symbols(intern, dictionary_symbol, type_class_instance_ast->type_class_instance.inst->constructor.conid->conid.symbol);
    }
    else
    {
        assert(false);
    }
    dictionary_instance_name = necro_intern_concat_symbols(intern, dictionary_instance_name, dictionary_name);
    return dictionary_instance_name;
}

// TODO: This stuff needs to be redone in light of the new renaming system!
NecroAst* necro_create_selector(NecroPagedArena* arena, NecroIntern* intern, NecroAst* type_class_ast, NecroSymbol dictionary_name, void* member_to_select, NecroSymbol selector_name)
{
    NecroAst* pat_var  = necro_ast_create_var(arena, intern, "selected_member@", NECRO_VAR_DECLARATION);
    NecroAst* rhs_var  = necro_ast_create_var(arena, intern, "selected_member@", NECRO_VAR_VAR);
    NecroAst* con_args = NULL;
    NecroAst* con_head = NULL;
    NecroAst* members  = type_class_ast->type_class_declaration.declarations;
    while (members != NULL)
    {
        if ((void*)members == member_to_select)
        {
            if (con_head == NULL)
            {
                con_args = necro_ast_create_apats(arena, pat_var, NULL);
                con_head = con_args;
            }
            else
            {
                con_args->apats.next_apat = necro_ast_create_apats(arena, pat_var, NULL);
                con_args                  = con_args->apats.next_apat;
            }
        }
        else
        {
            if (con_head == NULL)
            {
                con_args = necro_ast_create_apats(arena, necro_ast_create_wildcard(arena), NULL);
                con_head = con_args;
            }
            else
            {
                con_args->apats.next_apat = necro_ast_create_apats(arena, necro_ast_create_wildcard(arena), NULL);
                con_args                  = con_args->list.next_item;
            }
        }
        members = members->declaration.next_declaration;
    }
    NecroAst* context = type_class_ast->type_class_declaration.context;
    if (context != NULL && context->type == NECRO_AST_TYPE_CLASS_CONTEXT)
        context = necro_ast_create_list(arena, context, NULL);
    while (context != NULL)
    {
        if ((void*)(context->list.item) == member_to_select)
        {
            if (con_head == NULL)
            {
                con_args = necro_ast_create_apats(arena, pat_var, NULL);
                con_head = con_args;
            }
            else
            {
                con_args->apats.next_apat = necro_ast_create_apats(arena, pat_var, NULL);
                con_args                  = con_args->apats.next_apat;
            }
        }
        else
        {
            if (con_head == NULL)
            {
                con_args = necro_ast_create_apats(arena, necro_ast_create_wildcard(arena), NULL);
                con_head = con_args;
            }
            else
            {
                con_args->apats.next_apat = necro_ast_create_apats(arena, necro_ast_create_wildcard(arena), NULL);
                con_args                  = con_args->list.next_item;
            }
        }
        context = context->list.next_item;
    }
    NecroAst* constructor   = necro_ast_create_data_con(arena, intern, dictionary_name->str, con_head);
    constructor->constructor.conid->conid.con_type = NECRO_CON_VAR;
    NecroAst* selector_args = necro_ast_create_apats(arena, constructor, NULL);
    NecroAst* selector_rhs  = necro_ast_create_rhs(arena, rhs_var, NULL);
    NecroAst* selector_ast  = necro_ast_create_apats_assignment(arena, intern, selector_name->str, selector_args, selector_rhs);
    return selector_ast;
}

void necro_create_dictionary_data_declaration(NecroPagedArena* arena, NecroIntern* intern, NecroAst* type_class_ast)
{
    NecroSymbol dictionary_name        = necro_create_dictionary_name(intern, type_class_ast->type_class_declaration.tycls->conid.symbol);
    NecroAst*   dictionary_simple_type = necro_ast_create_simple_type(arena, intern, dictionary_name->str, NULL);
    NecroAst*   dictionary_args_head   = NULL;
    NecroAst*   dictionary_args        = NULL;
    NecroAst*   members                = type_class_ast->type_class_declaration.declarations;
    while (members != NULL)
    {
        if (dictionary_args_head == NULL)
        {
            dictionary_args_head = necro_ast_create_list(arena, necro_ast_create_conid(arena, intern, "_Poly", NECRO_CON_TYPE_VAR), NULL);
            dictionary_args      = dictionary_args_head;
        }
        else
        {
            dictionary_args->list.next_item = necro_ast_create_list(arena, necro_ast_create_conid(arena, intern, "_Poly", NECRO_CON_TYPE_VAR), NULL);
            dictionary_args                 = dictionary_args->list.next_item;
        }
        members = members->declaration.next_declaration;
    }
    NecroAst* context = type_class_ast->type_class_declaration.context;
    if (context != NULL && context->type == NECRO_AST_TYPE_CLASS_CONTEXT)
        context = necro_ast_create_list(arena, context, NULL);
    while (context != NULL)
    {
        NecroSymbol super_dictionary_name = necro_create_dictionary_name(intern, context->list.item->type_class_context.conid->conid.symbol);
        if (dictionary_args_head == NULL)
        {
            dictionary_args_head = necro_ast_create_list(arena, necro_ast_create_conid(arena, intern, super_dictionary_name->str, NECRO_CON_TYPE_VAR), NULL);
            dictionary_args      = dictionary_args_head;
        }
        else
        {
            dictionary_args->list.next_item = necro_ast_create_list(arena, necro_ast_create_conid(arena, intern, super_dictionary_name->str, NECRO_CON_TYPE_VAR), NULL);
            dictionary_args                 = dictionary_args->list.next_item;
        }
        context = context->list.next_item;
    }
    NecroAst* dictionary_constructor      = necro_ast_create_data_con(arena, intern, dictionary_name->str, dictionary_args_head);
    NecroAst* dictionary_constructor_list = necro_ast_create_list(arena, dictionary_constructor, NULL);
    NecroAst* dictionary_data_declaration = necro_ast_create_data_declaration(arena, intern, dictionary_simple_type, dictionary_constructor_list);
    NecroAst* dictionary_declaration_list = NULL;

    // Member selectors
    members = type_class_ast->type_class_declaration.declarations;
    while (members != NULL)
    {
        dictionary_declaration_list = necro_ast_create_top_decl(arena,
            necro_create_selector(arena, intern, type_class_ast, dictionary_name, (void*)members, necro_create_selector_name(intern, members->declaration.declaration_impl->type_signature.var->variable.symbol, dictionary_name)),
                dictionary_declaration_list);
        members = members->declaration.next_declaration;
    }

    // Super class selectors
    // Messy and inefficient, but oh well...
    context = type_class_ast->type_class_declaration.context;
    if (context != NULL && context->type == NECRO_AST_TYPE_CLASS_CONTEXT)
        context = necro_ast_create_list(arena, context, NULL);
    while (context != NULL)
    {
        dictionary_declaration_list = necro_ast_create_top_decl(arena,
            necro_create_selector(arena, intern, type_class_ast, dictionary_name, (void*)(context->list.item), necro_create_selector_name(intern, context->list.item->type_class_context.conid->conid.symbol, dictionary_name)),
                dictionary_declaration_list);
        context = context->list.next_item;
    }

    type_class_ast->type_class_declaration.dictionary_data_declaration = necro_ast_create_top_decl(arena, dictionary_data_declaration, dictionary_declaration_list);
}

NecroAst* retrieveNestedDictionaryFromContext(NecroInfer* infer, NecroTypeClassContext* context, NecroTypeClassContext* dictionary_to_retrieve, NecroAst* ast_so_far)
{
    if (context->class_symbol->name == dictionary_to_retrieve->class_symbol->name)
    {
        assert(ast_so_far != NULL);
        return ast_so_far;
    }
    else
    {
        // NecroTypeClassContext* super_classes = necro_type_class_table_get(&env->class_table, context->type_class_name.id.id)->context;
        NecroTypeClassContext* super_classes = context->type_class->context;
        while (super_classes != NULL)
        {
            // NOTE / HACK: Instead of allocating more memory every iterations to get isolated versions of each super class we just temporarily set the next pointer to NULL. Hackey, but it works.
            NecroTypeClassContext* super_next = super_classes->next;
            super_classes->next = NULL;
            if (necro_context_and_super_classes_contain_class(super_classes, dictionary_to_retrieve))
            {
                NecroSymbol dictionary_name = necro_create_dictionary_name(infer->intern, context->class_symbol->name);
                NecroSymbol selector_symbol = necro_create_selector_name(infer->intern, super_classes->class_symbol->name, dictionary_name);
                NecroAst*   selector_var    = necro_ast_create_var(infer->arena, infer->intern, selector_symbol->str, NECRO_VAR_VAR);
                selector_var->scope           = infer->scoped_symtable->top_scope;
                // TODO: NOTE THIS NEEDS TO BE REPLACED WITH NEW SYSTEM THAT DOESN'T USE THE RENAMER!
                // necro_rename_var_pass(infer->renamer, infer->arena, selector_var);
                NecroAst* super_ast       = retrieveNestedDictionaryFromContext(infer, super_classes, dictionary_to_retrieve, necro_ast_create_fexpr(infer->arena, selector_var, ast_so_far));
                assert(super_ast != NULL);
                super_classes->next = super_next;
                return super_ast;
            }
            super_classes->next = super_next;
            super_classes = super_classes->next;
        }
    }
    return NULL;
}

NecroAst* retrieveDictionaryFromContext(NecroInfer* infer, NecroTypeClassDictionaryContext* dictionary_context, NecroTypeClassContext* dictionary_to_retrieve, NecroAstSymbol* type_var)
{
    while (dictionary_context != NULL)
    {
        if (dictionary_context->type_class_name->name == dictionary_to_retrieve->var_symbol->name && dictionary_context->type_var_name->name == type_var->name)
        {
            // return necro_create_variable_ast(arena, intern, necro_intern_get_string(intern, necro_create_dictionary_name(intern, dictionary_to_retrieve->type_class_name.symbol)), NECRO_VAR_VAR);
            return dictionary_context->dictionary_arg_ast;
        }
        else if (dictionary_context->type_var_name->name == type_var->name)
        {
            NecroTypeClassContext context;
            context.class_symbol = dictionary_context->type_class_name;
            context.type_class   = dictionary_context->type_class_name->type_class;
            context.var_symbol   = type_var;
            context.next         = NULL;
            // NecroTypeClassContext* context_next = context->next;
            // context->next = NULL;
            if (necro_context_and_super_classes_contain_class(&context, dictionary_to_retrieve))
            {
                // NecroSymbol   d_arg_name = necro_create_dictionary_arg_name(intern, context->type_class_name.symbol, type_var_name);
                // NecroASTNode* d_arg_var  = necro_create_variable_ast(arena, intern, necro_intern_get_string(intern, d_arg_name), NECRO_VAR_VAR);
                NecroAst* super_ast = retrieveNestedDictionaryFromContext(infer, &context, dictionary_to_retrieve, dictionary_context->dictionary_arg_ast);
                assert(super_ast != NULL);
                // context->next = context_next;
                return super_ast;
            }
            // context->next = context_next;
        }
        dictionary_context = dictionary_context->next;
    }
    return NULL;
}

NecroAst* necro_resolve_context_to_dictionary(NecroInfer* infer, NecroTypeClassContext* context, NecroTypeClassDictionaryContext* dictionary_context)
{
    assert(infer != NULL);
    assert(context != NULL);
    NecroType* starting_type = context->var_symbol->type;
    NecroType* var_type      = necro_find(starting_type);
    assert(var_type != NULL);
    switch (var_type->type)
    {
    case NECRO_TYPE_VAR:
    {
        assert(var_type->var.context != NULL);
        // return retrieveDictionaryFromContext(infer->arena, infer->intern, env, dictionary_context, context, necro_con_to_var(var_type->var.context->type_var));
        if (var_type->var.is_rigid)
        {
            // NecroASTNode* d_var = retrieveDictionaryFromContext(infer->arena, infer->intern, env, dictionary_context, var_type->var.context, var_type->var.var);
            // NecroASTNode* d_var = retrieveDictionaryFromContext(infer->arena, infer->intern, env, dictionary_context, var_type->var.context, necro_con_to_var(var_type->var.context->type_var));
            NecroAst* d_var = retrieveDictionaryFromContext(infer, dictionary_context, context, var_type->var.context->var_symbol);
            assert(d_var != NULL);
            return d_var;
        }
        else
        {
            if (var_type->var.gen_bound == NULL)
            {
                // TODO: Defaulting
                // TODO: NecroResultError
                return NULL;
                // const char* var_string = necro_id_as_character_string(infer->intern, var_type->var.var);
                // return necro_infer_error(infer, "", starting_type, "Could not deduce (%s %s). The type variable %s is ambiguous",
                //     var_type->var.context->type_class_name.symbol->str,
                //     var_string,
                //     var_string);
            }
            else
            {
                assert(var_type->var.gen_bound->type == NECRO_TYPE_VAR);
                assert(var_type->var.gen_bound->var.is_rigid == true);
                // NecroASTNode* d_var = retrieveDictionaryFromContext(infer->arena, infer->intern, env, dictionary_context, var_type->var.context, var_type->var.gen_bound->var.var);
                // NecroASTNode* d_var = retrieveDictionaryFromContext(infer->arena, infer->intern, env, dictionary_context, var_type->var.context, necro_find(infer, var_type->var.gen_bound)->var.var);
                NecroAst* d_var = retrieveDictionaryFromContext(infer, dictionary_context, context, necro_find(var_type->var.gen_bound)->var.var_symbol);
                assert(d_var != NULL);
                return d_var;
            }
        }
    }
    case NECRO_TYPE_CON:
    {
        NecroTypeClassInstance* instance     = necro_get_type_class_instance(infer, var_type->con.con_symbol, context->class_symbol);
        NecroTypeClassContext*  inst_context = NULL;
        NecroType*              inst_type    = necro_inst_with_context(infer, instance->data_type, NULL, &inst_context);
        necro_unify(infer, var_type, inst_type, NULL, inst_type, "Compiler bug during necro_resolve_context_to_dictionary");
        NecroAst*               d_var        = necro_ast_create_var(infer->arena, infer->intern, instance->dictionary_instance_name->str, NECRO_VAR_VAR);
        d_var->scope = infer->scoped_symtable->top_type_scope;
        while (inst_context != NULL)
        {
            NecroAst* new_d_var = necro_resolve_context_to_dictionary(infer, inst_context, dictionary_context);
            new_d_var->scope = infer->scoped_symtable->top_type_scope;
            d_var = necro_ast_create_fexpr(infer->arena, d_var, new_d_var);
            inst_context = inst_context->next;
        }
        assert(d_var != NULL);
        return d_var;
    }
    case NECRO_TYPE_FUN:
    {
        // return necro_infer_error(infer, "", var_type, "Type classes on functions are not currently supported");
        assert(false && "Type classes on functions are not currently supported");
        return NULL;
    }
    default:
        assert(false);
    }
    return NULL;
}

NecroAst* necro_resolve_method(NecroInfer* infer, NecroTypeClass* method_type_class, NecroTypeClassContext* context, NecroAst* ast, NecroTypeClassDictionaryContext* dictionary_context)
{
    assert(infer != NULL);
    assert(method_type_class != NULL);
    assert(context != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_VARIABLE);
    NecroTypeClassContext* type_class_context = context;
    while (type_class_context != NULL && type_class_context->type_class != method_type_class)
        type_class_context = type_class_context->next;
    assert(type_class_context != NULL);
    assert(type_class_context->type_class == method_type_class);
    NecroType* var_type = necro_find(type_class_context->var_symbol->type);
    assert(var_type != NULL);
    switch (var_type->type)
    {

    case NECRO_TYPE_VAR:
    {
        assert(var_type->var.context != NULL);
        // NecroASTNode* d_var = retrieveDictionaryFromContext(infer->arena, infer->intern, env, dictionary_context, context, necro_con_to_var(var_type->var.context->type_var));
        NecroAst* d_var           = NULL;
        if (var_type->var.is_rigid)
        {
            d_var = retrieveDictionaryFromContext(infer, dictionary_context, type_class_context, var_type->var.context->var_symbol);
            d_var->scope = ast->scope;
            assert(d_var != NULL);
        }
        else
        {
            if (var_type->var.gen_bound == NULL)
            {
                // TODO: Defaulting
                // TODO: NecroResultError
                return NULL;
                // const char* var_string = necro_id_as_character_string(infer->intern, var_type->var.var);
                // return necro_infer_error(infer, "", starting_type, "Could not deduce (%s %s).\n  Arising from the use of %s.\n  The type variable %s is ambiguous",
                //     var_type->var.context->type_class_name.symbol->str,
                //     var_string,
                //     ast->variable.symbol->str,
                //     var_string);
            }
            else
            {
                assert(var_type->var.gen_bound != NULL);
                assert(var_type->var.gen_bound->type == NECRO_TYPE_VAR);
                assert(var_type->var.gen_bound->var.is_rigid == true);
                // d_var = retrieveDictionaryFromContext(infer->arena, infer->intern, env, dictionary_context, var_type->var.context, var_type->var.gen_bound->var.var);
                // d_var = retrieveDictionaryFromContext(infer->arena, infer->intern, env, dictionary_context, var_type->var.context, necro_find(infer, var_type->var.gen_bound)->var.var);
                d_var = retrieveDictionaryFromContext(infer, dictionary_context, type_class_context, necro_find(var_type->var.gen_bound)->var.var_symbol);
                assert(d_var != NULL);
                d_var->scope = ast->scope;
                assert(d_var != NULL);
            }
        }
        NecroSymbol dictionary_name  = necro_create_dictionary_name(infer->intern, type_class_context->class_symbol->name);
        NecroSymbol selector_name    = necro_create_selector_name(infer->intern, ast->variable.symbol, dictionary_name);
        NecroAst*   selector_var     = necro_ast_create_var(infer->arena, infer->intern, selector_name->str, NECRO_VAR_VAR);
        selector_var->scope          = infer->scoped_symtable->top_scope;
        // TODO: NOTE THIS NEEDS TO BE REPLACED WITH NEW SYSTEM THAT DOESN'T USE THE RENAMER!
        // necro_rename_var_pass(infer->renamer, infer->arena, selector_var);
        NecroAst* m_var              = necro_ast_create_fexpr(infer->arena, selector_var, d_var);
        m_var->scope                 = ast->scope;
        while (context != NULL)
        {
            if (context != type_class_context)
            {
                d_var        = necro_resolve_context_to_dictionary(infer, context, dictionary_context);
                d_var->scope = ast->scope;
                m_var        = necro_ast_create_fexpr(infer->arena, m_var, d_var);
                m_var->scope = ast->scope;
            }
            context = context->next;
        }
        return m_var;
    }

    case NECRO_TYPE_CON:
    {
        // Resolve directly to method instance
        NecroTypeClassInstance*   instance  = necro_get_type_class_instance(infer, var_type->con.con_symbol, type_class_context->class_symbol);
        NecroDictionaryPrototype* prototype = instance->dictionary_prototype;
        while (prototype != NULL)
        {
            if (prototype->type_class_member_varid->name == ast->variable.ast_symbol->name)
            {
                NecroAst*              m_var                = necro_ast_create_var(infer->arena, infer->intern, prototype->prototype_varid->name->str, NECRO_VAR_VAR);
                m_var->variable.ast_symbol                  = prototype->prototype_varid;
                m_var->scope                                = ast->scope;
                NecroType*             member_instance_type = prototype->prototype_varid->type;
                NecroTypeClassContext* inst_context         = NULL;
                NecroType*             inst_type            = necro_inst_with_context(infer, member_instance_type, NULL, &inst_context);
                necro_unify(infer, ast->necro_type, inst_type, NULL, inst_type, "Compiler bug during necro_resolve_context_to_dictionary");
                while (inst_context != NULL)
                {
                    NecroAst* d_var = necro_resolve_context_to_dictionary(infer, inst_context, dictionary_context);
                    d_var->scope    = ast->scope;
                    m_var           = necro_ast_create_fexpr(infer->arena, m_var, d_var);
                    inst_context    = inst_context->next;
                }
                assert(m_var != NULL);
                return m_var;
            }
            prototype = prototype->next;
        }
        assert(false);
        return NULL;
    }

    case NECRO_TYPE_FUN:
    {
        assert(false && "Type classes on functions are not currently supported");
        return NULL;
    }

    default:
        assert(false);
        return NULL;
    }
}

NecroTypeClassDictionaryContext* necro_create_type_class_dictionary_context(NecroPagedArena* arena, NecroAstSymbol* type_class_name, NecroAstSymbol* type_var_name, NecroAst* dictionary_arg_ast, NecroTypeClassDictionaryContext* next)
{
    NecroTypeClassDictionaryContext* dictionary_context = necro_paged_arena_alloc(arena, sizeof(NecroTypeClassDictionaryContext));
    dictionary_context->type_class_name                 = type_class_name;
    dictionary_context->type_var_name                   = type_var_name;
    dictionary_context->dictionary_arg_ast              = dictionary_arg_ast;
    dictionary_context->intentially_not_included        = false;
    dictionary_context->next                            = next;
    return dictionary_context;
}

// TODO: Knock this around a bit!
void necro_type_class_translate_constant(NecroInfer* infer, NecroAst* ast, NecroTypeClassDictionaryContext* dictionary_context)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_CONSTANT);

    // Removing overloaded numeric literal patterns for now.
#if 0
    if (ast->constant.type != NECRO_AST_CONSTANT_FLOAT_PATTERN && ast->constant.type != NECRO_AST_CONSTANT_INTEGER_PATTERN) return;

    const char*     from_name  = NULL;
    NecroTypeClass* type_class = NULL;
    NecroType*      num_type   = NULL;
    switch (ast->constant.type)
    {
    case NECRO_AST_CONSTANT_INTEGER_PATTERN:
        from_name  = "fromInt";
        type_class = necro_symtable_get(infer->symtable, infer->prim_types->num_type_class.id)->type_class;
        num_type   = necro_symtable_get(infer->symtable, infer->prim_types->int_type.id)->type;
        break;
    case NECRO_AST_CONSTANT_FLOAT_PATTERN:
        from_name  = "fromRational";
        type_class = necro_symtable_get(infer->symtable, infer->prim_types->fractional_type_class.id)->type_class;
        num_type   = necro_symtable_get(infer->symtable, infer->prim_types->rational_type.id)->type;
        break;
    default: assert(false);
    }

    //-----------------
    // from method
    {
        NecroASTNode* from_ast  = necro_create_variable_ast(infer->arena, infer->intern, from_name, NECRO_VAR_VAR);
        from_ast->scope         = ast->scope;
        NecroType* from_type    = necro_create_type_fun(infer, num_type, ast->necro_type);
        from_type->type_kind    = infer->star_type_kind;
        from_ast->necro_type    = from_type;
        necro_rename_var_pass(infer->renamer, infer->arena, from_ast);
        NecroTypeClassContext* inst_context = necro_create_type_class_context(infer->arena, type_class, type_class->type_class_name, necro_var_to_con(ast->necro_type->var.var), NULL);
        NecroASTNode* m_var = necro_resolve_method(infer, type_class, inst_context, from_ast, dictionary_context);
        if (necro_is_infer_error(infer)) return;
        assert(m_var != NULL);
        m_var->scope   = ast->scope;
        necro_rename_var_pass(infer->renamer, infer->arena, m_var);
        if (necro_is_infer_error(infer)) return;
        ast->constant.pat_from_ast = m_var;
    }

    //-----------------
    // eq method
    {
        type_class           = necro_symtable_get(infer->symtable, infer->prim_types->eq_type_class.id)->type_class;
        NecroASTNode* eq_ast = necro_create_variable_ast(infer->arena, infer->intern, "eq", NECRO_VAR_VAR);
        eq_ast->scope        = ast->scope;
        NecroType* eq_type   = necro_create_type_fun(infer, ast->necro_type, necro_create_type_fun(infer, ast->necro_type, necro_symtable_get(infer->symtable, infer->prim_types->bool_type.id)->type));
        eq_type->type_kind   = infer->star_type_kind;
        eq_ast->necro_type   = eq_type;
        necro_rename_var_pass(infer->renamer, infer->arena, eq_ast);
        NecroTypeClassContext* inst_context = necro_create_type_class_context(infer->arena, type_class, type_class->type_class_name, necro_var_to_con(ast->necro_type->var.var), NULL);
        NecroASTNode* m_var = necro_resolve_method(infer, type_class, inst_context, eq_ast, dictionary_context);
        if (necro_is_infer_error(infer)) return;
        assert(m_var != NULL);
        m_var->scope   = ast->scope;
        necro_rename_var_pass(infer->renamer, infer->arena, m_var);
        if (necro_is_infer_error(infer)) return;
        ast->constant.pat_from_ast = m_var;
    }
#else
    UNUSED(dictionary_context);
#endif
}

void necro_type_class_translate_go(NecroTypeClassDictionaryContext* dictionary_context, NecroInfer* infer, NecroAst* ast)
{
    if (ast == NULL)
        return;
    // assert(dictionary_context != NULL);
    switch (ast->type)
    {

        //=====================================================
        // Declaration type things
        //=====================================================
    case NECRO_AST_TOP_DECL:
    {
        NecroDeclarationGroupList* group_list = ast->top_declaration.group_list;
        while (group_list != NULL)
        {
            NecroDeclarationGroup* declaration_group = group_list->declaration_group;
            while (declaration_group != NULL)
            {
                necro_type_class_translate_go(dictionary_context, infer, declaration_group->declaration_ast);
                declaration_group = declaration_group->next;
            }
            group_list = group_list->next;
        }
        break;
    }

    case NECRO_AST_DECL:
    {
        NecroDeclarationGroupList* group_list = ast->declaration.group_list;
        while (group_list != NULL)
        {
            NecroDeclarationGroup* declaration_group = group_list->declaration_group;
            while (declaration_group != NULL)
            {
                necro_type_class_translate_go(dictionary_context, infer, declaration_group->declaration_ast);
                declaration_group = declaration_group->next;
            }
            group_list = group_list->next;
        }
        break;
    }

    case NECRO_AST_TYPE_CLASS_INSTANCE:
    {
        // break;
        // NecroCon type_class_name = (NecroCon) { .symbol = ast->type_class_instance.qtycls->conid.symbol, .id = ast->type_class_instance.qtycls->conid.id };
        // NecroCon data_type_name;
        // if (ast->type_class_instance.inst->type == NECRO_AST_CONID)
        //     data_type_name = (NecroCon) { .symbol = ast->type_class_instance.inst->conid.symbol, .id = ast->type_class_instance.inst->conid.id };
        // else if (ast->type_class_instance.inst->type == NECRO_AST_CONSTRUCTOR)
        //     data_type_name = (NecroCon) { .symbol = ast->type_class_instance.inst->constructor.conid->conid.symbol, .id = ast->type_class_instance.inst->constructor.conid->conid.id };
        // else
        //     assert(false);
        // NecroTypeClassInstance*          instance               = necro_get_type_class_instance(infer, data_type_name.symbol, type_class_name.symbol);
        // NecroSymbol                      dictionary_arg_name    = necro_create_dictionary_arg_name(infer->intern, type_class_name.symbol, data_type_name.symbol);
        // NecroASTNode*                    dictionary_arg_var     = necro_create_variable_ast(infer->arena, infer->intern, necro_intern_get_string(infer->intern, dictionary_arg_name), NECRO_VAR_DECLARATION);
        // dictionary_arg_var->scope                               = ast->scope;
        // necro_rename_declare_pass(infer->renamer, infer->arena, dictionary_arg_var);
        // if (necro_is_infer_error(infer)) return;
        // NecroTypeClassDictionaryContext* new_dictionary_context = necro_create_type_class_dictionary_context(infer->arena, type_class_name, data_type_name, dictionary_arg_var, dictionary_context);
        // necro_type_class_translate_go(new_dictionary_context, infer, env, ast->type_class_instance.context);
        // necro_type_class_translate_go(new_dictionary_context, infer, env, ast->type_class_instance.qtycls);
        // necro_type_class_translate_go(new_dictionary_context, infer, env, ast->type_class_instance.inst);
        necro_type_class_translate_go(dictionary_context, infer, ast->type_class_instance.declarations);
        break;
    }

    //=====================================================
    // Assignment type things
    //=====================================================
    case NECRO_AST_SIMPLE_ASSIGNMENT:
    {
        NecroType*                       type                   = necro_find(ast->necro_type);
        NecroTypeClassDictionaryContext* new_dictionary_context = dictionary_context;
        NecroAst*                        dictionary_args        = NULL;
        NecroAst*                        dictionary_args_head   = NULL;
        NecroAst*                        rhs                    = ast->simple_assignment.rhs;
        if (ast->simple_assignment.initializer != NULL && !necro_is_fully_concrete(type))
        {
            // TODO: NecroResultError
            // necro_infer_ast_error(infer, type, ast, "Only fully concrete (non-polymorphic) types may be used as initial values");
            return;
        }
        if (ast->simple_assignment.initializer != NULL && !ast->simple_assignment.is_recursive)
        {
            // TODO: NecroResultError
            // necro_infer_ast_error(infer, type, ast, "Cannot set an initial value for non-recursive types. (It would be immediately discarded)");
            return;
        }
        while (type->type == NECRO_TYPE_FOR)
        {
            NecroTypeClassContext* for_all_context = type->for_all.context;
            while (for_all_context != NULL)
            {
                NecroSymbol dictionary_arg_name = necro_create_dictionary_arg_name(infer->intern, for_all_context->class_symbol->name, for_all_context->var_symbol->name);
                NecroAst*   dictionary_arg_var  = necro_ast_create_var(infer->arena, infer->intern, dictionary_arg_name->str, NECRO_VAR_DECLARATION);
                dictionary_arg_var->scope       = ast->simple_assignment.rhs->scope;
                // TODO: NOTE THIS NEEDS TO BE REPLACED WITH NEW SYSTEM THAT DOESN'T USE THE RENAMER!
                // necro_rename_declare_pass(infer->renamer, infer->arena, dictionary_arg_var);
                if (necro_is_infer_error(infer)) return;
                new_dictionary_context = necro_create_type_class_dictionary_context(infer->arena, for_all_context->class_symbol, for_all_context->var_symbol, dictionary_arg_var, new_dictionary_context);
                if (dictionary_args_head == NULL)
                {
                    dictionary_args = necro_ast_create_apats(infer->arena, dictionary_arg_var, NULL);
                    dictionary_args_head = dictionary_args;
                }
                else
                {
                    dictionary_args->apats.next_apat = necro_ast_create_apats(infer->arena, dictionary_arg_var, NULL);
                    dictionary_args = dictionary_args->apats.next_apat;
                }
                for_all_context = for_all_context->next;
            }
            type = type->for_all.type;
        }
        if (dictionary_args_head != NULL)
        {
            NecroAst* new_rhs = necro_ast_create_wildcard(infer->arena);
            *new_rhs          = *rhs;
            *rhs              = *necro_ast_create_lambda(infer->arena, dictionary_args_head, new_rhs);
        }
        necro_type_class_translate_go(new_dictionary_context, infer, ast->simple_assignment.initializer);
        necro_type_class_translate_go(new_dictionary_context, infer, rhs);
        break;
    }

    case NECRO_AST_APATS_ASSIGNMENT:
    {
        NecroType*                       type                   = ast->necro_type;
        NecroTypeClassDictionaryContext* new_dictionary_context = dictionary_context;
        NecroAst*                        dictionary_args        = NULL;
        NecroAst*                        dictionary_args_head   = NULL;
        while (type->type == NECRO_TYPE_FOR)
        {
            NecroTypeClassContext* for_all_context = type->for_all.context;
            while (for_all_context != NULL)
            {
                NecroSymbol dictionary_arg_name = necro_create_dictionary_arg_name(infer->intern, for_all_context->class_symbol->name, for_all_context->var_symbol->name);
                NecroAst*   dictionary_arg_var  = necro_ast_create_var(infer->arena, infer->intern, dictionary_arg_name->str, NECRO_VAR_DECLARATION);
                dictionary_arg_var->scope = ast->apats_assignment.rhs->scope;
                // TODO: NOTE THIS NEEDS TO BE REPLACED WITH NEW SYSTEM THAT DOESN'T USE THE RENAMER!
                // necro_rename_declare_pass(infer->renamer, infer->arena, dictionary_arg_var);
                new_dictionary_context = necro_create_type_class_dictionary_context(infer->arena, for_all_context->class_symbol, for_all_context->var_symbol, dictionary_arg_var, new_dictionary_context);
                if (dictionary_args_head == NULL)
                {
                    dictionary_args = necro_ast_create_apats(infer->arena, dictionary_arg_var, NULL);
                    dictionary_args_head = dictionary_args;
                }
                else
                {
                    dictionary_args->apats.next_apat = necro_ast_create_apats(infer->arena, dictionary_arg_var, NULL);
                    dictionary_args = dictionary_args->apats.next_apat;
                }
                for_all_context = for_all_context->next;
            }
            type = type->for_all.type;
        }
        if (dictionary_args_head != NULL)
        {
            dictionary_args->apats.next_apat = ast->apats_assignment.apats;
            ast->apats_assignment.apats      = dictionary_args_head;
        }
        necro_type_class_translate_go(new_dictionary_context, infer, ast->apats_assignment.apats);
        necro_type_class_translate_go(new_dictionary_context, infer, ast->apats_assignment.rhs);
        break;
    }

    // NOTE: We are making pattern bindings fully monomorphic now (even if a type signature is given. Haskell was at least like this at one point)
    // If we REALLY want this (for some reason!?!?) we can look into putting more effort into getting polymorphic pattern bindings in.
    // For now they have a poor weight to power ration.
    case NECRO_AST_PAT_ASSIGNMENT:
        necro_rec_check_pat_assignment(infer, ast->pat_assignment.pat);
        necro_type_class_translate_go(dictionary_context, infer, ast->pat_assignment.rhs);
        break;

    case NECRO_AST_DATA_DECLARATION:
        necro_type_class_translate_go(dictionary_context, infer, ast->data_declaration.simpletype);
        necro_type_class_translate_go(dictionary_context, infer, ast->data_declaration.constructor_list);
        break;

    case NECRO_AST_VARIABLE:
        switch (ast->variable.var_type)
        {
        case NECRO_VAR_VAR:
        {
            NecroAstSymbol* data = ast->variable.ast_symbol;
            if (data->method_type_class != NULL)
            {
                // TODO: necro_ast_create_var_from_symbol!?
                NecroAst* var_ast = necro_ast_create_var(infer->arena, infer->intern, ast->variable.ast_symbol->name->str, NECRO_VAR_VAR);
                *var_ast = *ast;
                NecroTypeClassContext* inst_context = var_ast->variable.inst_context;
                assert(inst_context != NULL);
                NecroAst* m_var = necro_resolve_method(infer, data->method_type_class, inst_context, var_ast, dictionary_context);
                assert(m_var != NULL);
                m_var->scope   = ast->scope;
                // TODO: NOTE THIS NEEDS TO BE REPLACED WITH NEW SYSTEM THAT DOESN'T USE THE RENAMER!
                // necro_rename_var_pass(infer->renamer, infer->arena, m_var);
                *ast = *m_var;
            }
            else if (ast->variable.inst_context != NULL)
            {
                NecroAst* var_ast = necro_ast_create_var(infer->arena, infer->intern, ast->variable.ast_symbol->name->str, NECRO_VAR_VAR);
                *var_ast = *ast;
                var_ast->scope = ast->scope;
                NecroTypeClassContext* inst_context = ast->variable.inst_context;
                while (inst_context != NULL)
                {
                    NecroAst* d_var = necro_resolve_context_to_dictionary(infer, inst_context, dictionary_context);
                    assert(d_var != NULL);
                    var_ast = necro_ast_create_fexpr(infer->arena, var_ast, d_var);
                    d_var->scope   = ast->scope;
                    // TODO: NOTE THIS NEEDS TO BE REPLACED WITH NEW SYSTEM THAT DOESN'T USE THE RENAMER!
                    // necro_rename_var_pass(infer->renamer, infer->arena, d_var);
                    var_ast->scope = ast->scope;
                    inst_context = inst_context->next;
                }
                *ast = *var_ast;
            }
            break;
        }

        case NECRO_VAR_DECLARATION:          break;
        case NECRO_VAR_SIG:                  break;
        case NECRO_VAR_TYPE_VAR_DECLARATION: break;
        case NECRO_VAR_TYPE_FREE_VAR:        break;
        case NECRO_VAR_CLASS_SIG:            break;
        default: assert(false);
        }
        break;

    case NECRO_AST_CONID:
        break;

    case NECRO_AST_UNDEFINED:
        break;

    case NECRO_AST_CONSTANT:
        necro_type_class_translate_constant(infer, ast, dictionary_context);
        break;

    case NECRO_AST_UN_OP:
        break;

    case NECRO_AST_BIN_OP:
    {
        assert(ast->necro_type != NULL);
        // TODO: necro_ast_create_bin_op
        NecroAst* op_ast              = necro_ast_create_var(infer->arena, infer->intern, ast->bin_op.ast_symbol->name->str, NECRO_VAR_VAR);
        op_ast->variable.inst_context = ast->bin_op.inst_context;
        op_ast->variable.id           = ast->bin_op.id;
        op_ast->variable.symbol       = ast->bin_op.symbol;
        op_ast->scope                 = ast->scope;
        op_ast->necro_type            = ast->necro_type;
        op_ast                        = necro_ast_create_fexpr(infer->arena, op_ast, ast->bin_op.lhs);
        op_ast->scope                 = ast->scope;
        op_ast->necro_type            = ast->necro_type;
        op_ast                        = necro_ast_create_fexpr(infer->arena, op_ast, ast->bin_op.rhs);
        op_ast->scope                 = ast->scope;
        op_ast->necro_type            = ast->necro_type;
        *ast = *op_ast;
        if (ast->bin_op.inst_context != NULL)
            necro_type_class_translate_go(dictionary_context, infer, ast);
        break;
    }

    case NECRO_AST_OP_LEFT_SECTION:
    {
        assert(ast->necro_type != NULL);
        // TODO: necro_ast_create_left_section
        NecroAst* op_ast              = necro_ast_create_var(infer->arena, infer->intern, ast->op_left_section.ast_symbol->name->str, NECRO_VAR_VAR);
        op_ast->variable.inst_context = ast->op_left_section.inst_context;
        op_ast->variable.id           = ast->op_left_section.id;
        op_ast->variable.symbol       = ast->op_left_section.symbol;
        op_ast->scope                 = ast->scope;
        op_ast->necro_type            = ast->op_left_section.op_necro_type;
        op_ast                        = necro_ast_create_fexpr(infer->arena, op_ast, ast->op_left_section.left);
        op_ast->scope                 = ast->scope;
        op_ast->necro_type            = ast->necro_type;
        *ast = *op_ast;
        if (ast->op_left_section.inst_context != NULL)
            necro_type_class_translate_go(dictionary_context, infer, ast);
        break;
    }

    case NECRO_AST_OP_RIGHT_SECTION:
    {
        // TODO: necro_ast_create_right_section
        // TODO: necro_ast_create_var_from_ast_symbol
        assert(ast->necro_type != NULL);
        NecroAst* x_var_arg                   = necro_ast_create_var(infer->arena, infer->intern, "left@rightSection", NECRO_VAR_DECLARATION);
        NecroAst* x_var_var                   = necro_ast_create_var(infer->arena, infer->intern, "left@rightSection", NECRO_VAR_VAR);
        NecroAst* op_ast                      = necro_ast_create_var(infer->arena, infer->intern, ast->op_right_section.ast_symbol->name->str, NECRO_VAR_VAR);
        op_ast->variable.inst_context         = ast->op_right_section.inst_context;
        op_ast->variable.id                   = ast->op_right_section.id;
        op_ast->variable.symbol               = ast->op_right_section.symbol;
        op_ast->scope                         = NULL;
        op_ast->necro_type                    = ast->op_right_section.op_necro_type;
        op_ast                                = necro_ast_create_fexpr(infer->arena, op_ast, x_var_var);
        op_ast                                = necro_ast_create_fexpr(infer->arena, op_ast, ast->op_right_section.right);
        NecroAst* lambda_ast                  = necro_ast_create_lambda(infer->arena, necro_ast_create_apats(infer->arena, x_var_arg, NULL), op_ast);
        infer->scoped_symtable->current_scope = ast->scope;
        lambda_ast->necro_type                = ast->necro_type;
        necro_build_scopes_go(infer->scoped_symtable, lambda_ast);
        // TODO: NOTE THIS NEEDS TO BE REPLACED WITH NEW SYSTEM THAT DOESN'T USE THE RENAMER!
        // necro_rename_declare_pass(infer->renamer, infer->arena, lambda_ast);
        // necro_rename_var_pass(infer->renamer, infer->arena, lambda_ast);
        *ast = *lambda_ast;
        if (ast->op_right_section.inst_context != NULL)
            necro_type_class_translate_go(dictionary_context, infer, ast);
        break;
    }

    case NECRO_AST_IF_THEN_ELSE:
        necro_type_class_translate_go(dictionary_context, infer, ast->if_then_else.if_expr);
        necro_type_class_translate_go(dictionary_context, infer, ast->if_then_else.then_expr);
        necro_type_class_translate_go(dictionary_context, infer, ast->if_then_else.else_expr);
        break;

    case NECRO_AST_RIGHT_HAND_SIDE:
        necro_type_class_translate_go(dictionary_context, infer, ast->right_hand_side.declarations);
        necro_type_class_translate_go(dictionary_context, infer, ast->right_hand_side.expression);
        break;

    case NECRO_AST_LET_EXPRESSION:
        necro_type_class_translate_go(dictionary_context, infer, ast->let_expression.declarations);
        necro_type_class_translate_go(dictionary_context, infer, ast->let_expression.expression);
        break;

    case NECRO_AST_FUNCTION_EXPRESSION:
        necro_type_class_translate_go(dictionary_context, infer, ast->fexpression.aexp);
        necro_type_class_translate_go(dictionary_context, infer, ast->fexpression.next_fexpression);
        break;

    case NECRO_AST_APATS:
        // necro_type_class_translate_go(infer, ast->apats.apat, dictionary_context);
        necro_type_class_translate_go(dictionary_context, infer, ast->apats.apat);
        necro_type_class_translate_go(dictionary_context, infer, ast->apats.next_apat);
        break;

    case NECRO_AST_WILDCARD:
        break;

    case NECRO_AST_LAMBDA:
        necro_type_class_translate_go(dictionary_context, infer, ast->lambda.apats);
        necro_type_class_translate_go(dictionary_context, infer, ast->lambda.expression);
        break;

    case NECRO_AST_DO:
    {
        // DO statements NOT FULLY IMPLEMENTED currently
        assert(false);
        break;
        // TODO: REDO!
        // NecroAst* bind_node = necro_ast_create_var(infer->arena, infer->intern, "bind", NECRO_VAR_VAR);
        // NecroScope* scope = ast->scope;
        // while (scope->parent != NULL)
        //     scope = scope->parent;
        // bind_node->scope = bind_node->scope = scope;
        // // TODO: NOTE THIS NEEDS TO BE REPLACED WITH NEW SYSTEM THAT DOESN'T USE THE RENAMER!
        // // necro_rename_var_pass(infer->renamer, infer->arena, bind_node);
        // bind_node->necro_type = necro_symtable_get(infer->symtable, bind_node->variable.id)->type;
        // NecroTypeClassContext* bind_context     = necro_symtable_get(infer->symtable, bind_node->variable.id)->type->for_all.context;
        // NecroTypeClassContext* monad_context    = ast->do_statement.monad_var->var.context;
        // while (monad_context->type_class_name.id.id != bind_context->type_class_name.id.id)
        //     monad_context = monad_context->next;
        // // assert(monad_context->type_class_name.id.id != bind_context->type_class_name.id.id);
        // NecroAst*          bind_method_inst = necro_resolve_method(infer, monad_context->type_class, monad_context, bind_node, dictionary_context);
        // necro_ast_print(bind_method_inst);
        // necro_type_class_translate_go(dictionary_context, infer, ast->do_statement.statement_list);
        // break;
    }

    case NECRO_AST_LIST_NODE:
        necro_type_class_translate_go(dictionary_context, infer, ast->list.item);
        necro_type_class_translate_go(dictionary_context, infer, ast->list.next_item);
        break;

    case NECRO_AST_EXPRESSION_LIST:
        necro_type_class_translate_go(dictionary_context, infer, ast->expression_list.expressions);
        break;

    case NECRO_AST_EXPRESSION_ARRAY:
        necro_type_class_translate_go(dictionary_context, infer, ast->expression_array.expressions);
        break;

    case NECRO_AST_PAT_EXPRESSION:
        necro_type_class_translate_go(dictionary_context, infer, ast->pattern_expression.expressions);
        break;

    case NECRO_AST_TUPLE:
        necro_type_class_translate_go(dictionary_context, infer, ast->tuple.expressions);
        break;

    case NECRO_BIND_ASSIGNMENT:
        necro_type_class_translate_go(dictionary_context, infer, ast->bind_assignment.expression);
        break;

    case NECRO_PAT_BIND_ASSIGNMENT:
        necro_type_class_translate_go(dictionary_context, infer, ast->pat_bind_assignment.pat);
        necro_type_class_translate_go(dictionary_context, infer, ast->pat_bind_assignment.expression);
        break;

    case NECRO_AST_ARITHMETIC_SEQUENCE:
        necro_type_class_translate_go(dictionary_context, infer, ast->arithmetic_sequence.from);
        necro_type_class_translate_go(dictionary_context, infer, ast->arithmetic_sequence.then);
        necro_type_class_translate_go(dictionary_context, infer, ast->arithmetic_sequence.to);
        break;

    case NECRO_AST_CASE:
        necro_type_class_translate_go(dictionary_context, infer, ast->case_expression.expression);
        necro_type_class_translate_go(dictionary_context, infer, ast->case_expression.alternatives);
        break;

    case NECRO_AST_CASE_ALTERNATIVE:
        necro_type_class_translate_go(dictionary_context, infer, ast->case_alternative.pat);
        necro_type_class_translate_go(dictionary_context, infer, ast->case_alternative.body);
        break;

    case NECRO_AST_TYPE_APP:
        necro_type_class_translate_go(dictionary_context, infer, ast->type_app.ty);
        necro_type_class_translate_go(dictionary_context, infer, ast->type_app.next_ty);
        break;

    case NECRO_AST_BIN_OP_SYM:
        necro_type_class_translate_go(dictionary_context, infer, ast->bin_op_sym.left);
        necro_type_class_translate_go(dictionary_context, infer, ast->bin_op_sym.op);
        necro_type_class_translate_go(dictionary_context, infer, ast->bin_op_sym.right);
        break;

    case NECRO_AST_CONSTRUCTOR:
        necro_type_class_translate_go(dictionary_context, infer, ast->constructor.conid);
        necro_type_class_translate_go(dictionary_context, infer, ast->constructor.arg_list);
        break;

    case NECRO_AST_SIMPLE_TYPE:
        necro_type_class_translate_go(dictionary_context, infer, ast->simple_type.type_con);
        necro_type_class_translate_go(dictionary_context, infer, ast->simple_type.type_var_list);
        break;

    case NECRO_AST_TYPE_CLASS_DECLARATION:
#if 0
        necro_type_class_translate_go(dictionary_context, infer, ast->type_class_declaration.context);
        necro_type_class_translate_go(dictionary_context, infer, ast->type_class_declaration.tycls);
        necro_type_class_translate_go(dictionary_context, infer, ast->type_class_declaration.tyvar);
        necro_type_class_translate_go(dictionary_context, infer, ast->type_class_declaration.declarations);
#endif
        break;

    case NECRO_AST_TYPE_SIGNATURE:
#if 0
        necro_type_class_translate_go(dictionary_context, infer, ast->type_signature.var);
        necro_type_class_translate_go(dictionary_context, infer, ast->type_signature.context);
        necro_type_class_translate_go(dictionary_context, infer, ast->type_signature.type);
#endif
        break;

    case NECRO_AST_TYPE_CLASS_CONTEXT:
#if 0
        necro_type_class_translate_go(dictionary_context, infer, ast->type_class_context.conid);
        necro_type_class_translate_go(dictionary_context, infer, ast->type_class_context.varid);
#endif
        break;

    case NECRO_AST_FUNCTION_TYPE:
#if 0
        necro_type_class_translate_go(dictionary_context, infer, ast->function_type.type);
        necro_type_class_translate_go(dictionary_context, infer, ast->function_type.next_on_arrow);
#endif
        break;

    default:
        assert(false);
        break;
    }
}

NECRO_RETURN_CODE necro_type_class_translate(NecroInfer* infer, NecroAst* ast)
{
    necro_type_class_translate_go(NULL, infer, ast);
    return infer->error.return_code;
}

NecroSymbol necro_create_type_class_instance_name(NecroIntern* intern, NecroAst* ast)
{
    NecroAstSymbol* type_class_name = ast->type_class_instance.qtycls->conid.ast_symbol;
    NecroAstSymbol* data_type_name;
    if (ast->type_class_instance.inst->type == NECRO_AST_CONID)
    {
        data_type_name = ast->type_class_instance.inst->conid.ast_symbol;
    }
    else if (ast->type_class_instance.inst->type == NECRO_AST_CONSTRUCTOR)
    {
        data_type_name = ast->type_class_instance.inst->constructor.conid->conid.ast_symbol;
    }
    else
    {
        assert(false);
        data_type_name = NULL;
    }

    // TODO: Redo this to use NecroSymbol system...!
    return necro_intern_create_type_class_instance_symbol(intern, data_type_name->name, type_class_name->name);
}

NecroTypeClassContext* necro_ast_to_context(NecroInfer* infer, NecroAst* context_ast)
{
    NecroTypeClassContext* context      = NULL;
    NecroTypeClassContext* context_head = NULL;
    while (context_ast != NULL)
    {
        if (context_ast->type == NECRO_AST_LIST_NODE)
        {
            assert(context_ast->list.item->type == NECRO_AST_TYPE_CLASS_CONTEXT);
            NecroTypeClassContext* new_context = necro_create_type_class_context(
                infer->arena,
                context_ast->list.item->type_class_context.conid->conid.ast_symbol->type_class,
                context_ast->list.item->type_class_context.conid->conid.ast_symbol,
                context_ast->list.item->type_class_context.varid->variable.ast_symbol,
                NULL
            );
            if (context_head == NULL)
            {
                context      = new_context;
                context_head = context;
            }
            else
            {
                context->next = new_context;
                context       = context->next;
            }
            if (context->class_symbol->type_class == NULL)
            {
                // TODO: NecroResultError
                // necro_infer_ast_error(infer, NULL, NULL, "\'%s\' is not a type class", context->type_class_name.symbol->str);
                // return NULL;
                return NULL;
            }
            context_ast = context_ast->list.next_item;
        }
        else if (context_ast->type == NECRO_AST_TYPE_CLASS_CONTEXT)
        {
            context = necro_create_type_class_context(
                infer->arena,
                context_ast->type_class_context.conid->conid.ast_symbol->type_class,
                context_ast->type_class_context.conid->conid.ast_symbol,
                context_ast->type_class_context.varid->variable.ast_symbol,
                NULL
            );
            if (context->class_symbol->type_class == NULL)
            {
                // TODO: NecroResultError
                // necro_infer_ast_error(infer, NULL, NULL, "\'%s\' is not a type class", context->type_class_name.symbol->str);
                // return NULL;
                return NULL;
            }
            return context;
        }
        else
        {
            assert(false);
        }
    }
    return context_head;
}

void necro_create_type_class(NecroInfer* infer, NecroAst* type_class_ast)
{
    if (necro_is_infer_error(infer)) return;
    assert(infer != NULL);
    assert(type_class_ast != NULL);
    assert(type_class_ast->type == NECRO_AST_TYPE_CLASS_DECLARATION);

    //------------------------------------
    // Create type class
    NecroAstSymbol* data       = type_class_ast->type_class_declaration.tycls->conid.ast_symbol;
    NecroTypeClass*     type_class = necro_paged_arena_alloc(infer->arena, sizeof(NecroTypeClass));
    type_class->type_class_name    = type_class_ast->type_class_declaration.tycls->conid.ast_symbol;
    type_class->members            = NULL;
    type_class->type_var           = type_class_ast->type_class_declaration.tyvar->variable.ast_symbol;
    type_class->context            = NULL;
    type_class->dependency_flag    = 0;
    type_class->ast                = type_class_ast;
    type_class->dictionary_name    = necro_create_dictionary_name(infer->intern, type_class->type_class_name->name);
    data->type_class               = type_class;

    //------------------------------------
    // Create type_var for type_class
    NecroType* type_class_var             = necro_type_var_create(infer->arena, type_class_ast->type_class_declaration.tyvar->variable.ast_symbol);
    // ty_var->var.is_rigid                 = false;
    type_class_var->var.is_rigid          = true;
    type_class_var->var.is_type_class_var = true;
    type_class->type                      = necro_type_con1_create(infer->arena, type_class->type_class_name, necro_type_list_create(infer->arena, type_class_var, NULL));
    type_class->type->con.is_class        = true;
    type_class->context                   = necro_ast_to_context(infer, type_class_ast->type_class_declaration.context);
    data->type                            = type_class_var;

    type_class_var->var.context = necro_create_type_class_context(infer->arena, type_class, type_class->type_class_name, type_class->type_var, type_class->context);
    necro_kind_infer(infer, type_class_var, type_class_var, "While declaring a type class: ");
    NecroTypeClassContext* class_context = necro_create_type_class_context(infer->arena, type_class, type_class->type_class_name, type_class->type_var, NULL);

    //--------------------------------
    // Build Member Type Signatures
    if (type_class->ast->type_class_declaration.declarations != NULL)
    {
        NecroDeclarationGroupList* group_list = type_class->ast->type_class_declaration.declarations->declaration.group_list;
        while (group_list != NULL)
        {
            NecroDeclarationGroup* declaration_group = group_list->declaration_group;
            while (declaration_group != NULL)
            {
                NecroAst* method_ast = declaration_group->declaration_ast;
                if (method_ast->type != NECRO_AST_TYPE_SIGNATURE)
                    continue;

                NecroType* type_sig = necro_ast_to_type_sig_go(infer, method_ast->type_signature.type);

                //---------------------------------
                // Infer and check method type sig
                // NOTE: TypeClass's context should ALWAYS BE FIRST!
                NecroTypeClassContext* method_context = necro_ast_to_context(infer, method_ast->type_signature.context);
                if (necro_constrain_class_variable_check(type_class->type_var, method_ast->type_signature.var->variable.ast_symbol, method_context)) return;
                NecroTypeClassContext* context = necro_union_contexts(infer->arena, class_context, method_context);
                assert(context->type_class == class_context->type_class);
                if (necro_ambiguous_type_class_check(method_ast->type_signature.var->variable.ast_symbol, context, type_sig)) return;
                necro_apply_constraints(infer->arena, type_sig, context);
                type_sig->pre_supplied = true;
                type_sig->source_loc   = method_ast->source_loc;
                type_sig               = necro_gen(infer, type_sig, NULL);
                necro_kind_infer(infer, type_sig, type_sig, "While declaring a method of a type class: ");
                necro_kind_gen(infer, type_sig->kind);
                necro_kind_unify(infer, type_sig->kind, infer->base->star_kind->type, NULL, type_sig, "While declaring a method of a type class");
                method_ast->type_signature.var->variable.ast_symbol->type              = type_sig;
                method_ast->type_signature.var->variable.ast_symbol->method_type_class = type_class;

                //---------------------------------
                // kind check for type context!
                NecroTypeClassContext* curr_context = context;
                while (curr_context != NULL)
                {
                    // NecroTypeClass* type_class     = necro_type_class_table_get(&infer->type_class_env->class_table, curr_context->type_class_name.id.id);
                    NecroTypeClass* curr_type_class      = curr_context->type_class;
                    NecroType*      curr_type_class_var  = curr_type_class->type_var->type;
                    NecroType*      var_type             = curr_context->var_symbol->type;
                    necro_kind_unify(infer, var_type->kind, curr_type_class_var->kind, method_ast->scope, type_sig, "While inferring the type of a method type signature");
                    curr_context = curr_context->next;
                }

                //---------------------------------
                // Store type class members
                NecroTypeClassMember* new_member  = necro_paged_arena_alloc(infer->arena, sizeof(NecroTypeClassMember));
                new_member->member_varid = method_ast->type_signature.var->variable.ast_symbol;
                new_member->next         = NULL;
                if (type_class->members == NULL)
                {
                    type_class->members = new_member;
                }
                else
                {
                    NecroTypeClassMember* curr_member = type_class->members;
                    while (curr_member != NULL)
                    {
                        if (curr_member->next == NULL)
                        {
                            curr_member->next = new_member;
                            break;
                        }
                        curr_member = curr_member->next;
                    }
                }
                // next
                declaration_group = declaration_group->next;
            }
            group_list = group_list->next;
        }
    }

    //---------------------------
    // Build dictionary declarations
    // NecroASTNode* dictionary_declaration_ast = type_class->ast->type_class_declaration.dictionary_data_declaration;
    necro_create_dictionary_data_declaration(infer->arena, infer->intern, type_class->ast);

    //---------------------------
    // Infer dictionary data declaration types
    NecroDeclarationGroup* dictionary_declaration_group = NULL;
    NecroAst*              dictionary_declarations      = type_class->ast->type_class_declaration.dictionary_data_declaration;
    while (dictionary_declarations != NULL)
    {
        //---------------------------
        // Build scopes and rename
        necro_build_scopes_go(infer->scoped_symtable, dictionary_declarations->top_declaration.declaration);
        // infer->error = infer->scoped_symtable->error;
        // TODO: NOTE THIS NEEDS TO BE REPLACED WITH NEW SYSTEM THAT DOESN'T USE THE RENAMER!
        // necro_rename_declare_pass(infer->renamer, infer->arena, dictionary_declarations->top_declaration.declaration);
        // infer->error = infer->renamer->error;
        // TODO: NOTE THIS NEEDS TO BE REPLACED WITH NEW SYSTEM THAT DOESN'T USE THE RENAMER!
        // necro_rename_var_pass(infer->renamer, infer->arena, dictionary_declarations->top_declaration.declaration);
        // infer->error = infer->renamer->error;

        dictionary_declaration_group = necro_declaration_group_append(infer->arena, dictionary_declarations->top_declaration.declaration, dictionary_declaration_group);
        dictionary_declarations      = dictionary_declarations->top_declaration.next_top_decl;
    }
    necro_infer_declaration_group(infer, dictionary_declaration_group);

    // Generalize type class kind
    type_class_var->kind = necro_kind_gen(infer, type_class_var->kind);
}

void necro_create_type_class_instance(NecroInfer* infer, NecroAst* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    assert(ast->type == NECRO_AST_TYPE_CLASS_INSTANCE);

    //------------------
    // Info and Names
    NecroAstSymbol* data            = ast->type_class_instance.ast_symbol;
    NecroAstSymbol*      type_class_name = ast->type_class_instance.qtycls->conid.ast_symbol;
    NecroAstSymbol*      data_type_name;
    if (ast->type_class_instance.inst->type == NECRO_AST_CONID)
    {
        data_type_name = ast->type_class_instance.inst->conid.ast_symbol;
    }
    else if (ast->type_class_instance.inst->type == NECRO_AST_CONSTRUCTOR)
    {
        data_type_name = ast->type_class_instance.inst->constructor.conid->conid.ast_symbol;
    }
    else
    {
        assert(false);
        data_type_name = NULL;
    }

    //------------------
    // Data, Class, Duplicate checks
    if (data_type_name->type == NULL)
    {
        // TODO: NecroResultErro + is this required!?
        // necro_infer_ast_error(infer, NULL, ast, "No type named \'%s\', in instance declaration for class \'%s\'", data_type_name.symbol->str, type_class_name.symbol->str);
        // return;
        return;
    }
    NecroTypeClass* type_class = type_class_name->type_class;
    if (type_class == NULL)
    {
        // TODO: NecroResultError
        // necro_infer_ast_error(infer, NULL, ast, "%s cannot be an instance of %s, because %s is not a type class",
        //     data_type_name.symbol->str,
        //     type_class_name.symbol->str,
        //     type_class_name.symbol->str);
        return;
    }
    if (data->type_class_instance != NULL)
    {
        // TODO: NecroResultError
        // necro_infer_ast_error(infer, NULL, ast, "Duplicate Type Class Instance\n     class: %s\n     type: %s", type_class_name.symbol->str, data_type_name.symbol->str);
        return;
    }

    //------------------
    // Create instance
    NecroTypeClassInstance* instance   = necro_paged_arena_alloc(infer->arena, sizeof(NecroTypeClassInstance));
    // instance->instance_name            = ast->type_class_instance.ast_symbol;
    instance->type_class_name          = type_class_name;
    instance->data_type_name           = data_type_name;
    instance->context                  = necro_ast_to_context(infer, ast->type_class_instance.context);
    instance->dictionary_prototype     = NULL;
    instance->ast                      = ast;
    instance->dictionary_instance_name = necro_create_dictionary_instance_name(infer->intern, type_class_name->name, ast); // TODO: Change this to use NecroAstSymbol
    instance->data_type                = necro_ast_to_type_sig_go(infer, instance->ast->type_class_instance.inst);
    instance->super_instances          = NULL;
    data->type_class_instance          = instance;

    NecroType* class_var               = type_class->type_var->type;
    assert(class_var != NULL);

    necro_kind_infer(infer, instance->data_type, instance->data_type, "While creating an instance of a type class");
    necro_kind_unify(infer, instance->data_type->kind, class_var->kind, NULL, instance->data_type, "While creating an instance of a type class");

    // Apply constraints
    necro_apply_constraints(infer->arena, instance->data_type, instance->context);
    instance->data_type = necro_gen(infer, instance->data_type, NULL);

    NecroTypeClassContext* instance_context = NULL;
    // NecroType*             inst_data_type   = necro_inst_with_context(infer, instance->data_type, ast->scope, &instance_context);
    // UNUSED(inst_data_type);

    //--------------------------------
    // Dictionary Prototype
    if (instance->ast->type_class_instance.declarations != NULL)
    {
        NecroDeclarationGroupList* group_list = instance->ast->type_class_instance.declarations->declaration.group_list;
        while (group_list != NULL)
        {
            NecroDeclarationGroup* declaration_group = group_list->declaration_group;
            while (declaration_group != NULL)
            {
                NecroAst* method_ast = declaration_group->declaration_ast;
                assert(method_ast != NULL);
                assert(method_ast->type == NECRO_AST_SIMPLE_ASSIGNMENT || method_ast->type == NECRO_AST_APATS_ASSIGNMENT);
                if (necro_is_infer_error(infer)) return;

                //--------------------------------
                // Create dictionary prototype entry
                NecroDictionaryPrototype* prev_dictionary = instance->dictionary_prototype;
                instance->dictionary_prototype            = necro_paged_arena_alloc(infer->arena, sizeof(NecroDictionaryPrototype));
                if (method_ast->type == NECRO_AST_SIMPLE_ASSIGNMENT)
                    instance->dictionary_prototype->prototype_varid = method_ast->simple_assignment.ast_symbol;
                else if (method_ast->type == NECRO_AST_APATS_ASSIGNMENT)
                    instance->dictionary_prototype->prototype_varid = method_ast->apats_assignment.ast_symbol;
                else
                    assert(false);
                instance->dictionary_prototype->next = NULL;
                NecroSymbol member_symbol = necro_intern_get_type_class_member_symbol_from_instance_symbol(infer->intern, instance->dictionary_prototype->prototype_varid->name);

                //--------------------------------
                // Search for Type Class member
                bool                  found   = false;
                NecroTypeClassMember* members = type_class->members;
                while (members != NULL)
                {
                    if (members->member_varid->name == member_symbol)
                    {
                        instance->dictionary_prototype->type_class_member_varid = members->member_varid;
                        found = true;
                        break;
                    }
                    members = members->next;
                }

                //--------------------------------
                // Didn't find a matching method in class, bail out!
                if (found == false)
                {
                    instance->dictionary_prototype->next                          = NULL;
                    // TODO: NecroResultError
                    // necro_infer_ast_error(infer, NULL, instance->ast, "\'%s\' is not a (visible) method of class \'%s\'", member_symbol->str, instance->type_class_name.symbol->str);
                    return;
                }

                //--------------------------------
                // Assemble types for overloaded methods
                assert(members != NULL);
                assert(members->member_varid != NULL);

                NecroType* method_type      = members->member_varid->type;
                NecroType* inst_method_type = necro_instantiate_method_sig(infer, type_class->type_var, method_type, instance->data_type);
                instance->dictionary_prototype->prototype_varid->type               = inst_method_type;
                instance->dictionary_prototype->prototype_varid->type->pre_supplied = true;

                //--------------------------------
                // next
                instance->dictionary_prototype->next = prev_dictionary;
                declaration_group = declaration_group->next;
            }
            group_list = group_list->next;
        }
    }

    //--------------------------------
    // Infer declarations
    if (instance->ast->type_class_instance.declarations != NULL)
    {
        assert(instance->ast->type_class_instance.declarations->declaration.group_list != NULL);
        necro_infer_go(infer, instance->ast->type_class_instance.declarations);
    }

    //--------------------------------
    // Missing members check
    NecroTypeClassMember* type_class_members = type_class->members;
    while (type_class_members != NULL)
    {
        NecroDictionaryPrototype* dictionary_prototype = instance->dictionary_prototype;
        bool matched = false;
        while (dictionary_prototype != NULL)
        {
            if (dictionary_prototype->type_class_member_varid->name == type_class_members->member_varid->name)
            {
                matched = true;
                break;
            }
            dictionary_prototype = dictionary_prototype->next;
        }
        if (!matched)
        {
            // TODO: NecroResultError
            // necro_infer_ast_error(infer, NULL, instance->ast, "No explicit implementation for \'%s\' in the instance declaration for \'%s %s\'",
            //     type_class_members->member_varid.symbol->str,
            //     type_class->type_class_name.symbol->str,
            //     instance->data_type_name.symbol->str);
            return;
        }
        type_class_members = type_class_members->next;
    }

    //--------------------------------
    // Create Dictionary Instance
    {
        NecroSymbol           dictionary_name          = type_class->dictionary_name;
        NecroSymbol           dictionary_instance_name = necro_create_dictionary_instance_name(infer->intern, type_class->type_class_name->name, instance->ast);
        NecroAst*             dictionary_conid         = necro_ast_create_conid(infer->arena, infer->intern, dictionary_name->str, NECRO_CON_VAR);
        // TODO: Figure this next line out... what to do about this!?
        // dictionary_conid->conid.id                     = type_class->ast->type_class_declaration.dictionary_data_declaration->top_declaration.declaration->data_declaration.simpletype->simple_type.type_con->conid.id;
        NecroAst*             dictionary_fexpr         = dictionary_conid;
        NecroTypeClassMember* members                  = type_class->members;
        // NecroVar              type_var                 = necro_con_to_var(type_class->type_var);

        //---------------------------------
        // Dictionary context and args
        NecroAst*                        dictionary_instance           = NULL;
        NecroTypeClassDictionaryContext* dictionary_context            = NULL;
        NecroTypeClassDictionaryContext* dictionary_context_curr       = NULL;
        NecroAst*                        dictionary_instance_args_head = NULL;
        NecroAst*                        dictionary_instance_args      = NULL;
        if (instance->context != NULL)
        {
            NecroTypeClassContext* inst_context = instance_context;
            while (inst_context != NULL)
            {
                // TODO: Replace with NecroAstSymbol system
                NecroType*     inst_context_type_var_type = NULL;
                // NecroType* inst_context_type_var_type = necro_find(infer->env.data[inst_context->type_var.id.id]);
                NecroAstSymbol* inst_context_type_var      = inst_context_type_var_type->var.context->var_symbol;
                // NecroCon   inst_context_type_var      = inst_context->type_var;
                // d_var
                // NecroSymbol   arg_name = necro_create_dictionary_arg_name(infer->intern, inst_context->type_class_name.symbol, type_var.symbol);
                // NecroSymbol   arg_name = necro_create_dictionary_arg_name(infer->intern, inst_context->type_class_name.symbol, inst_context_type_var.id);
                NecroSymbol   arg_name = necro_create_dictionary_arg_name(infer->intern, inst_context->class_symbol->name, inst_context->var_symbol->name);
                NecroAst*     d_var    = necro_ast_create_var(infer->arena, infer->intern, arg_name->str, NECRO_VAR_DECLARATION);
                if (dictionary_instance_args_head == NULL)
                {
                    dictionary_instance_args_head = necro_ast_create_apats(infer->arena, d_var, NULL);
                    dictionary_instance_args      = dictionary_instance_args_head;
                }
                else
                {
                    dictionary_instance_args->apats.next_apat = necro_ast_create_apats(infer->arena, d_var, NULL);
                    dictionary_instance_args                  = dictionary_instance_args->apats.next_apat;
                }
                // dictionary_context
                if (dictionary_context == NULL)
                {
                    dictionary_context      = necro_create_type_class_dictionary_context(infer->arena, inst_context->class_symbol, inst_context_type_var, d_var, NULL);
                    dictionary_context_curr = dictionary_context;
                }
                else
                {
                    dictionary_context_curr->next = necro_create_type_class_dictionary_context(infer->arena, inst_context->class_symbol, inst_context_type_var, d_var, NULL);
                    dictionary_context_curr       = dictionary_context_curr->next;
                }
                inst_context = inst_context->next;
            }
        }

        //---------------------------------
        // Dictionary member app
        while (members != NULL)
        {
            NecroDictionaryPrototype* dictionary_prototype = instance->dictionary_prototype;
            while (dictionary_prototype != NULL)
            {
                if (dictionary_prototype->type_class_member_varid->name == members->member_varid->name)
                {
                    NecroAst* mvar    = necro_ast_create_var(infer->arena, infer->intern, dictionary_prototype->prototype_varid->name->str, NECRO_VAR_VAR);
                    // TODO: Proper way of doing AST creation during this whole mess... Currently very bad!!!
                    // mvar->variable.id = dictionary_prototype->prototype_varid.id;
                    NecroType* prototype_member_type = dictionary_prototype->prototype_varid->type;
                    assert(prototype_member_type != NULL);
                    NecroTypeClassContext* inst_context = instance_context;
                    while (inst_context != NULL)
                    {
                        // TODO: Replace with NecroAstSymbol system
                        NecroType*     inst_context_type_var_type = necro_find(inst_context->var_symbol->type);
                        NecroAstSymbol* inst_context_type_var      = inst_context_type_var_type->var.context->var_symbol;
                        NecroAst*      dvar                       = necro_ast_create_var(infer->arena, infer->intern, necro_create_dictionary_arg_name(infer->intern, inst_context->class_symbol->name, inst_context->var_symbol->name)->str, NECRO_VAR_VAR);
                        // TODO: Is this required!?!?
                        dictionary_context = necro_create_type_class_dictionary_context(infer->arena, inst_context->class_symbol, inst_context_type_var, dvar, dictionary_context);
                        mvar               = necro_ast_create_fexpr(infer->arena, mvar, dvar);
                        inst_context       = inst_context->next;
                    }
                    dictionary_fexpr = necro_ast_create_fexpr(infer->arena, dictionary_fexpr, mvar);
                    break;
                }
                dictionary_prototype = dictionary_prototype->next;
            }
            members = members->next;
        }

        //---------------------------------
        // Resolve Super instance symbols
        NecroTypeClassContext* super_classes = type_class->context;
        while (super_classes != NULL)
        {
            NecroTypeClassInstance* super_instance = necro_get_type_class_instance(infer, instance->data_type_name, super_classes->class_symbol);
            // Super class not implemented Error?
            if (super_instance == NULL)
            {
                // TODO: NecroResultError
                // necro_infer_ast_error(infer, instance->data_type, instance->ast, "Data type \'%s\' does not implement the super class \'%s\', required by instance for class \'%s\'",
                //     instance->data_type_name.symbol->str,
                //     super_classes->type_class_name.symbol->str,
                //     type_class->type_class_name.symbol->str);
                return;
            }
            assert(super_instance != NULL);
            instance->super_instances = necro_cons_instance_list(infer->arena, super_instance, instance->super_instances);
            super_classes             = super_classes->next;
        }

        //---------------------------------
        // Dictionary super class dictionary app
        NecroInstanceList* super_instances = instance->super_instances;
        while(super_instances != NULL)
        {
            NecroSymbol             super_class_dictionary_name = necro_create_dictionary_instance_name(infer->intern, super_instances->data->type_class_name->name, super_instances->data->ast);
            NecroAst*               dvar                        = necro_ast_create_var(infer->arena, infer->intern, super_class_dictionary_name->str, NECRO_VAR_VAR);
            // NecroTypeClassContext*  super_context               = super_instances->data->context;
            // Gen and inst!?
            NecroTypeClassContext* super_context = NULL;
            NecroType*  inst_super_inst = necro_inst_with_context(infer, super_instances->data->data_type, ast->scope, &super_context);
            necro_unify(infer, instance->data_type, inst_super_inst, ast->scope, super_instances->data->data_type, "Compiler bug: While resolving super instance dictionaries");
            // NecroTypeClassContext* super_context = context
            // Super class dictionary arguments
            while (super_context != NULL)
            {
                // TODO: Replace with NecroAstSymbol system
                NecroType*     super_inst_context_type = necro_find(super_context->var_symbol->type);
                NecroAstSymbol* super_inst_context_var  = super_inst_context_type->var.var_symbol;
                    // necro_con_to_var(necro_find(infer, super_context->type_var);
                // NecroASTNode* argument_dictionary = retrieveDictionaryFromContext(infer, dictionary_context, super_context, necro_con_to_var(type_class->type_var)); // is using the type class var correct here!?
                NecroAst* argument_dictionary = retrieveDictionaryFromContext(infer, dictionary_context, super_context, super_inst_context_var); // is using the type class var correct here!?
                assert(argument_dictionary != NULL);
                dvar          = necro_ast_create_fexpr(infer->arena, dvar, argument_dictionary);
                super_context = super_context->next;
            }
            dictionary_fexpr = necro_ast_create_fexpr(infer->arena, dictionary_fexpr, dvar);
            super_instances = super_instances->next;
        }
        if (instance->context == NULL)
            dictionary_instance = necro_ast_create_simple_assignment(infer->arena, infer->intern, dictionary_instance_name->str, necro_ast_create_rhs(infer->arena, dictionary_fexpr, NULL));
        else
            dictionary_instance = necro_ast_create_apats_assignment(infer->arena, infer->intern, dictionary_instance_name->str, dictionary_instance_args_head, necro_ast_create_rhs(infer->arena, dictionary_fexpr, NULL));
        instance->ast->type_class_instance.dictionary_instance = dictionary_instance;

        //---------------------------
        // Build scopes and rename
        // TODO: NOTE THIS NEEDS TO BE REPLACED WITH NEW SYSTEM THAT DOESN'T USE THE RENAMER!
        // necro_build_scopes_go(infer->scoped_symtable, dictionary_instance);
        // infer->error = infer->scoped_symtable->error;
        // necro_rename_declare_pass(infer->renamer, infer->arena, dictionary_instance);
        // infer->error = infer->renamer->error;
        // TODO: NOTE THIS NEEDS TO BE REPLACED WITH NEW SYSTEM THAT DOESN'T USE THE RENAMER!
        // necro_rename_var_pass(infer->renamer, infer->arena, dictionary_instance);
        // infer->error = infer->renamer->error;
    }
}

// TODO: FIX THIS! Broken after symtable changes
NecroTypeClassInstance* necro_get_type_class_instance(NecroInfer* infer, NecroAstSymbol* data_type_name, NecroAstSymbol* type_class_name)
{
    NecroSymbol super_symbol   = necro_intern_create_type_class_instance_symbol(infer->intern, data_type_name->name, type_class_name->name);
    const char* super_name     = super_symbol->str;
    UNUSED(super_name);
    UNUSED(infer);
    // TODO: FIX THIS! Broken after symtable changes
    // // NecroID                 super_id       = necro_scope_find(infer->scoped_symtable->top_type_scope, super_symbol);
    // NecroSymbolInfo*        super_info     = necro_symtable_get(infer->symtable, super_id);
    // NecroTypeClassInstance* super_instance = super_info->type_class_instance;
    // // assert(super_instance != NULL);
    // return super_instance;
    return NULL;
}

void necro_rec_check_pat_assignment(NecroInfer* infer, NecroAst* ast)
{
    assert(infer != NULL);
    assert(ast != NULL);
    switch (ast->type)
    {
    case NECRO_AST_VARIABLE:
    {
        if (ast->variable.initializer != NULL && !necro_is_fully_concrete(ast->variable.ast_symbol->type))
        {
            // TODO; NecroResultError
            // necro_infer_ast_error(infer, necro_symtable_get(infer->symtable, ast->variable.id)->type, ast, "Only fully concrete (non-polymorphic) types may be used as initial values");
            return;
        }
        if (ast->variable.initializer != NULL && !ast->variable.is_recursive)
        {
            // TODO; NecroResultError
            // necro_infer_ast_error(infer, necro_symtable_get(infer->symtable, ast->variable.id)->type, ast, "Cannot set an initial value for %s, which is non-recursive. (It would be immediately discarded)", ast->variable.symbol->str);
            return;
        }
        return;
    }
    case NECRO_AST_CONSTANT: return;
    case NECRO_AST_TUPLE:
    {
        NecroAst* args = ast->tuple.expressions;
        while (args != NULL)
        {
            necro_rec_check_pat_assignment(infer, args->list.item);
            args = args->list.next_item;
        }
        return;
    }
    case NECRO_AST_EXPRESSION_LIST:
    {
        NecroAst* args = ast->expression_list.expressions;
        while (args != NULL)
        {
            necro_rec_check_pat_assignment(infer, args->list.item);
            args = args->list.next_item;
        }
        return;
    }
    case NECRO_AST_WILDCARD: return;
    case NECRO_AST_BIN_OP_SYM:
        necro_rec_check_pat_assignment(infer, ast->bin_op_sym.left);
        necro_rec_check_pat_assignment(infer, ast->bin_op_sym.right);
        return;
    case NECRO_AST_CONID: return;
    case NECRO_AST_CONSTRUCTOR:
    {
        NecroAst* args = ast->constructor.arg_list;
        while (args != NULL)
        {
            necro_rec_check_pat_assignment(infer, args->list.item);
            args = args->list.next_item;
        }
        return;
    }
    default: assert(false && "Unimplemented pattern in necro_rec_check_pat_assignment"); return;
    }
}
