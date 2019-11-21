/* Copyright (C) Chad McKinney and Curtis McKinney - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */

#include <stdio.h>
#include <inttypes.h>
#include <stdarg.h>
#include "base.h"
#include "renamer.h"
#include "d_analyzer.h"
#include "kind.h"
#include "infer.h"
#include "monomorphize.h"
#include "alias_analysis.h"

///////////////////////////////////////////////////////
// Create / Destroy
///////////////////////////////////////////////////////
NecroBase necro_base_create()
{
    return (NecroBase)
    {
        // .ast                    = necro_ast_arena_create(necro_intern_string(intern, "Necro.Base")),
        .ast                    = necro_ast_arena_empty(),

        .higher_kind            = NULL,
        .kind_kind              = NULL,
        .star_kind              = NULL,
        .nat_kind               = NULL,
        .sym_kind               = NULL,

        .ownership_kind         = NULL,
        .ownership_share        = NULL,
        .ownership_steal        = NULL,

        .tuple2_con             = NULL,
        .tuple3_con             = NULL,
        .tuple4_con             = NULL,
        .tuple5_con             = NULL,
        .tuple6_con             = NULL,
        .tuple7_con             = NULL,
        .tuple8_con             = NULL,
        .tuple9_con             = NULL,
        .tuple10_con            = NULL,

        .tuple2_type            = NULL,
        .tuple3_type            = NULL,
        .tuple4_type            = NULL,
        .tuple5_type            = NULL,
        .tuple6_type            = NULL,
        .tuple7_type            = NULL,
        .tuple8_type            = NULL,
        .tuple9_type            = NULL,
        .tuple10_type           = NULL,

        .world_type             = NULL,
        .unit_type              = NULL,
        .unit_con               = NULL,
        // .list_type              = NULL,
        .int_type               = NULL,
        .float_type             = NULL,
        // .audio_type             = NULL,
        .rational_type          = NULL,
        .char_type              = NULL,
        .bool_type              = NULL,
        .true_con               = NULL,
        .false_con              = NULL,
        .num_type_class         = NULL,
        .fractional_type_class  = NULL,
        .eq_type_class          = NULL,
        .ord_type_class         = NULL,
        .functor_type_class     = NULL,
        .applicative_type_class = NULL,
        .monad_type_class       = NULL,
        .default_type_class     = NULL,
        .prev_fn                = NULL,
        // .event_type             = NULL,
        // .pattern_type           = NULL,
        // .closure_type           = NULL,
        // .ptr_type               = NULL,
        .array_type             = NULL,
        .range_type             = NULL,
        .index_type             = NULL,
        // .maybe_type             = NULL,

        .pipe_forward           = NULL,
        .pipe_back              = NULL,
        .compose_forward        = NULL,
        .compose_back           = NULL,

        .mouse_x_fn             = NULL,
        .mouse_y_fn             = NULL,
    };
}

void necro_base_destroy(NecroBase* base)
{
    necro_ast_arena_destroy(&base->ast);
}

void necro_append_top(NecroPagedArena* arena, NecroAst* top, NecroAst* item)
{
    assert(top != NULL);
    assert(top->type == NECRO_AST_TOP_DECL);
    while (top->top_declaration.next_top_decl != NULL)
    {
        assert(top->type == NECRO_AST_TOP_DECL);
        top = top->top_declaration.next_top_decl;
    }
    top->top_declaration.next_top_decl = necro_ast_create_top_decl(arena, item, NULL);
}

void necro_append_top_decl(NecroAst* top, NecroAst* next_top)
{
    assert(top != NULL);
    assert(top->type == NECRO_AST_TOP_DECL);
    while (top->top_declaration.next_top_decl != NULL)
    {
        assert(top->type == NECRO_AST_TOP_DECL);
        top = top->top_declaration.next_top_decl;
    }
    top->top_declaration.next_top_decl = next_top;
}

void necro_base_create_simple_data_decl(NecroPagedArena* arena, NecroAst* top, NecroIntern* intern, const char* data_type_name)
{
    NecroAst* s_type   = necro_ast_create_simple_type(arena, intern, data_type_name, NULL);
    NecroAst* n_con    = necro_ast_create_data_con(arena, intern, data_type_name, NULL);
    NecroAst* con_list = necro_ast_create_list(arena, n_con, NULL);
    necro_append_top(arena, top, necro_ast_create_data_declaration(arena, intern, s_type, con_list));
}

void necro_base_create_simple_poly_data_decl(NecroPagedArena* arena, NecroAst* top, NecroIntern* intern, const char* data_type_name)
{
    NecroAst* s_type   = necro_ast_create_simple_type(arena, intern, data_type_name, necro_ast_create_var_list(arena, intern, 1, NECRO_VAR_TYPE_VAR_DECLARATION));
    NecroAst* n_con    = necro_ast_create_data_con(arena, intern, data_type_name, necro_ast_create_var_list(arena, intern, 1, NECRO_VAR_TYPE_FREE_VAR));
    NecroAst* con_list = necro_ast_create_list(arena, n_con, NULL);
    necro_append_top(arena, top, necro_ast_create_data_declaration(arena, intern, s_type, con_list));
}

NecroAst* necro_base_create_class_comp_sig(NecroPagedArena* arena, NecroIntern* intern, const char* sig_name)
{
    return
        necro_ast_create_fn_type_sig(arena, intern, sig_name, NULL,
            necro_ast_create_type_fn(arena,
                necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR),
                necro_ast_create_type_fn(arena,
                    necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR),
                    necro_ast_create_conid(arena, intern, "Bool", NECRO_CON_TYPE_VAR))), NECRO_VAR_CLASS_SIG, NECRO_SIG_TYPE_CLASS);
}

NecroAst* necro_base_create_class_bin_op_sig(NecroPagedArena* arena, NecroIntern* intern, const char* sig_name)
{
    return
        necro_ast_create_fn_type_sig(arena, intern, sig_name, NULL,
            necro_ast_create_type_fn(arena,
                necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR),
                necro_ast_create_type_fn(arena,
                    necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR),
                    necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR))), NECRO_VAR_CLASS_SIG, NECRO_SIG_TYPE_CLASS);
}


NecroAst* necro_base_create_class_unary_op_sig(NecroPagedArena* arena, NecroIntern* intern, const char* sig_name)
{
    return
        necro_ast_create_fn_type_sig(arena, intern, sig_name, NULL,
            necro_ast_create_type_fn(arena,
                necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR),
                necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR)), NECRO_VAR_CLASS_SIG, NECRO_SIG_TYPE_CLASS);
}

NecroAst* necro_base_make_poly_con1(NecroPagedArena* arena, NecroIntern* intern, const char* data_type_name)
{
    NecroAst* type_var_ast = necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR);
    NecroAst* type_constr  = necro_ast_create_data_con(arena, intern, data_type_name, necro_ast_create_list(arena, type_var_ast, NULL));
    type_constr->constructor.conid->conid.con_type = NECRO_CON_TYPE_VAR;
    return type_constr;
}

NecroAst* necro_base_make_num_con(NecroPagedArena* arena, NecroIntern* intern, const char* data_type_name, bool is_polymorphic)
{
    if (is_polymorphic)
        return necro_base_make_poly_con1(arena, intern, data_type_name);
    else
        return necro_ast_create_conid(arena, intern, data_type_name, NECRO_CON_TYPE_VAR);
}

NecroAst* necro_base_make_bin_math_ast(NecroPagedArena* arena, NecroIntern* intern, const char* data_type_name, bool is_poly)
{
    return necro_ast_create_type_fn(arena,
        necro_base_make_num_con(arena, intern, data_type_name, is_poly),
        necro_ast_create_type_fn(arena,
            necro_base_make_num_con(arena, intern, data_type_name, is_poly),
            necro_base_make_num_con(arena, intern, data_type_name, is_poly)));
}

NecroAst* necro_base_make_unary_math_ast(NecroPagedArena* arena, NecroIntern* intern, const char* data_type_name, bool is_poly)
{
    return necro_ast_create_type_fn(arena,
        necro_base_make_num_con(arena, intern, data_type_name, is_poly),
        necro_base_make_num_con(arena, intern, data_type_name, is_poly));
}

NecroAst* necro_base_make_num_context(NecroPagedArena* arena, NecroIntern* intern, const char* class_name, bool is_polymorphic)
{
    if (is_polymorphic)
        return necro_ast_create_context(arena, intern, class_name, "a", NULL);
    else
        return NULL;
}

NecroAst* necro_base_create_num_bin_op_sig(NecroPagedArena* arena, NecroIntern* intern, const char* sig_name)
{
    return
        necro_ast_create_fn_type_sig(arena, intern, sig_name, necro_ast_create_context(arena, intern, "Num", "a", NULL),
            necro_ast_create_type_fn(arena,
                necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR),
                necro_ast_create_type_fn(arena,
                    necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR),
                    necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR))), NECRO_VAR_SIG, NECRO_SIG_DECLARATION);
}

NecroAst* necro_base_create_fractional_bin_op_sig(NecroPagedArena* arena, NecroIntern* intern, const char* sig_name)
{
    return
        necro_ast_create_fn_type_sig(arena, intern, sig_name, necro_ast_create_context(arena, intern, "Fractional", "a", NULL),
            necro_ast_create_type_fn(arena,
                necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR),
                necro_ast_create_type_fn(arena,
                    necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR),
                    necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR))), NECRO_VAR_SIG, NECRO_SIG_DECLARATION);
}

NecroAst* necro_base_create_eq_comp_sig(NecroPagedArena* arena, NecroIntern* intern, const char* sig_name)
{
    return
        necro_ast_create_fn_type_sig(arena, intern, sig_name, necro_ast_create_context(arena, intern, "Eq", "a", NULL),
            necro_ast_create_type_fn(arena,
                necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR),
                necro_ast_create_type_fn(arena,
                    necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR),
                    necro_ast_create_conid(arena, intern, "Bool", NECRO_CON_TYPE_VAR))), NECRO_VAR_SIG, NECRO_SIG_DECLARATION);
}

NecroAst* necro_base_create_ord_comp_sig(NecroPagedArena* arena, NecroIntern* intern, const char* sig_name)
{
    return
        necro_ast_create_fn_type_sig(arena, intern, sig_name, necro_ast_create_context(arena, intern, "Ord", "a", NULL),
            necro_ast_create_type_fn(arena,
                necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR),
                necro_ast_create_type_fn(arena,
                    necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR),
                    necro_ast_create_conid(arena, intern, "Bool", NECRO_CON_TYPE_VAR))), NECRO_VAR_SIG, NECRO_SIG_DECLARATION);
}

NecroAst* necro_ast_create_prim_bin_op_method_ast(NecroPagedArena* arena, NecroIntern* intern, const char* name)
{
    return necro_ast_create_apats_assignment(arena, intern, name,
        necro_ast_create_apats(arena,
            necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION),
            necro_ast_create_apats(arena,
                necro_ast_create_var(arena, intern, "y", NECRO_VAR_DECLARATION),
                NULL)),
        necro_ast_create_rhs(arena,
            necro_ast_create_var(arena, intern, "primUndefined", NECRO_VAR_VAR),
            NULL));
}

NecroAst* necro_ast_create_prim_unary_op_method_ast(NecroPagedArena* arena, NecroIntern* intern, const char* name)
{
    return necro_ast_create_apats_assignment(arena, intern, name,
        necro_ast_create_apats(arena,
            necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION),
            NULL),
        necro_ast_create_rhs(arena,
            necro_ast_create_var(arena, intern, "primUndefined", NECRO_VAR_VAR),
            NULL));
}

NecroAst* necro_ast_create_reciprocal(NecroPagedArena* arena, NecroIntern* intern, const char* name)
{
    return necro_ast_create_apats_assignment(arena, intern, name,
        necro_ast_create_apats(arena,
            necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION),
            NULL),
        necro_ast_create_rhs(arena,
            necro_ast_create_fexpr(arena,
                necro_ast_create_fexpr(arena,
                    necro_ast_create_var(arena, intern, "div", NECRO_VAR_VAR),
                    necro_ast_create_fexpr(arena,
                        necro_ast_create_var(arena, intern, "fromInt", NECRO_VAR_VAR),
                        necro_ast_create_constant(arena, (NecroParseAstConstant) { .type = NECRO_AST_CONSTANT_INTEGER, .int_literal = 1 }))),
                necro_ast_create_var(arena, intern, "x", NECRO_VAR_VAR)),
            NULL));
}

void necro_base_create_prim_num_instances(NecroPagedArena* arena, NecroAst* top, NecroIntern* intern, const char* data_type_name, bool is_poly, bool is_num, bool is_fractional)
{
    //--------------
    // Num
    if (is_num)
    {
        NecroAst* num_method_list  =
            necro_ast_create_decl(arena, necro_ast_create_prim_bin_op_method_ast(arena, intern, "add"),
                necro_ast_create_decl(arena, necro_ast_create_prim_bin_op_method_ast(arena, intern, "sub"),
                    necro_ast_create_decl(arena, necro_ast_create_prim_bin_op_method_ast(arena, intern, "mul"),
                        necro_ast_create_decl(arena, necro_ast_create_prim_unary_op_method_ast(arena, intern, "abs"),
                            necro_ast_create_decl(arena, necro_ast_create_prim_unary_op_method_ast(arena, intern, "signum"),
                                necro_ast_create_decl(arena, necro_ast_create_prim_unary_op_method_ast(arena, intern, "fromInt"), NULL))))));
        necro_append_top(arena, top, necro_ast_create_instance(arena, intern, "Num", necro_base_make_num_con(arena, intern, data_type_name, is_poly), necro_base_make_num_context(arena, intern, "Num", is_poly), num_method_list));
    }

    //--------------
    // Fractional
    if (is_fractional)
    {
        NecroAst* fractional_method_list =
            necro_ast_create_decl(arena, necro_ast_create_prim_bin_op_method_ast(arena, intern, "div"),
                necro_ast_create_decl(arena, necro_ast_create_reciprocal(arena, intern, "recip"),
                    necro_ast_create_decl(arena, necro_ast_create_prim_unary_op_method_ast(arena, intern, "fromRational"), NULL)));
        necro_append_top(arena, top, necro_ast_create_instance(arena, intern, "Fractional", necro_base_make_num_con(arena, intern, data_type_name, is_poly), necro_base_make_num_context(arena, intern, "Fractional", is_poly), fractional_method_list));
    }

    //--------------
    // Eq
    NecroAst* eq_method_list =
        necro_ast_create_decl(arena, necro_ast_create_prim_bin_op_method_ast(arena, intern, "eq"),
            necro_ast_create_decl(arena, necro_ast_create_prim_bin_op_method_ast(arena, intern, "neq"), NULL));

    necro_append_top(arena, top, necro_ast_create_instance(arena, intern, "Eq", necro_base_make_num_con(arena, intern, data_type_name, is_poly), necro_base_make_num_context(arena, intern, "Eq", is_poly), eq_method_list));

    //--------------
    // Ord
    NecroAst* ord_method_list =
        necro_ast_create_decl(arena, necro_ast_create_prim_bin_op_method_ast(arena, intern, "lt"),
            necro_ast_create_decl(arena, necro_ast_create_prim_bin_op_method_ast(arena, intern, "gt"),
                necro_ast_create_decl(arena, necro_ast_create_prim_bin_op_method_ast(arena, intern, "lte"),
                    necro_ast_create_decl(arena, necro_ast_create_prim_bin_op_method_ast(arena, intern, "gte"), NULL))));
    necro_append_top(arena, top, necro_ast_create_instance(arena, intern, "Ord", necro_base_make_num_con(arena, intern, data_type_name, is_poly), necro_base_make_num_context(arena, intern, "Ord", is_poly), ord_method_list));


    //--------------
    // Default
    if (is_num)
    {
        NecroAst* default_method_ast  =
            necro_ast_create_simple_assignment(arena, intern, "default",
                necro_ast_create_rhs(arena,
                    necro_ast_create_fexpr(arena,
                        necro_ast_create_var(arena, intern, "fromInt", NECRO_VAR_VAR),
                        necro_ast_create_constant(arena, (NecroParseAstConstant) { .type = NECRO_AST_CONSTANT_INTEGER, .int_literal = 0 })),
                    NULL
                ));
        NecroAst* default_method_list = necro_ast_create_decl(arena, default_method_ast, NULL);
        necro_append_top(arena, top, necro_ast_create_instance(arena, intern, "Default", necro_base_make_num_con(arena, intern, data_type_name, is_poly), necro_base_make_num_context(arena, intern, "Num", is_poly), default_method_list));
    }
}

void necro_base_setup_primitive(NecroScopedSymTable* scoped_symtable, NecroIntern* intern, const char* prim_name, NecroAstSymbol** symbol_to_cache, NECRO_PRIMOP_TYPE primop_type)
{
    NecroAstSymbol* symbol = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, prim_name));
    assert(symbol != NULL);
    symbol->is_primitive = true;
    symbol->primop_type  = primop_type;
    if (symbol_to_cache != NULL)
    {
        *symbol_to_cache = symbol;
    }
}

char* necro_base_open_lib_file(const char* file_name, size_t* file_length)
{
#ifdef WIN32
        FILE* file;
        fopen_s(&file, file_name, "r");
#else
        FILE* file = fopen(file_name, "r");
#endif
        if (!file)
        {
            return NULL;
        }

        char*  str    = NULL;
        size_t length = 0;

        // Find length of file
        fseek(file, 0, SEEK_END);
        length = ftell(file);
        fseek(file, 0, SEEK_SET);

        // Allocate buffer
        str = emalloc(length + 2);

        // read contents of buffer
        length = fread(str, 1, length, file);
        str[length]     = '\n';
        str[length + 1] = '\0';
        fclose(file);
        *file_length    = length;
        return str;
}

// NOTE: We load the contents of base lib file at start up so that we don't need to reload it every time we run a test, which slows things down a lot.
size_t necro_base_lib_string_length = 0;
char*  necro_base_lib_string        = NULL;
void necro_base_global_init()
{
    necro_base_lib_string                                    = necro_base_open_lib_file("./lib/base.necro", &necro_base_lib_string_length);
    if (necro_base_lib_string == NULL) necro_base_lib_string = necro_base_open_lib_file("../lib/base.necro", &necro_base_lib_string_length);
    if (necro_base_lib_string == NULL) necro_base_lib_string = necro_base_open_lib_file("../../lib/base.necro", &necro_base_lib_string_length);
    if (necro_base_lib_string == NULL) necro_base_lib_string = necro_base_open_lib_file("../../../lib/base.necro", &necro_base_lib_string_length);
    if (necro_base_lib_string == NULL) necro_base_lib_string = necro_base_open_lib_file("./base.necro", &necro_base_lib_string_length);
}

void necro_base_global_cleanup()
{
    free(necro_base_lib_string);
}

NecroBase necro_base_compile(NecroIntern* intern, NecroScopedSymTable* scoped_symtable)
{
    NecroCompileInfo info     = (NecroCompileInfo) { .verbosity = 0, .timer = NULL, .opt_level = NECRO_OPT_OFF, .compilation_phase = NECRO_PHASE_JIT };
    NecroBase        base     = necro_base_create();

    //--------------------
    // base.necro
    NecroLexTokenVector lex_tokens = necro_empty_lex_token_vector();
    NecroParseAstArena  parse_ast  = necro_parse_ast_arena_empty();
    unwrap_result(void, necro_lex(info, intern, necro_base_lib_string, necro_base_lib_string_length, &lex_tokens));
    unwrap_result(void, necro_parse(info, intern, &lex_tokens, necro_intern_string(intern, "Necro.Base"), &parse_ast));
    base.ast                       = necro_reify(info, intern, &parse_ast);
    NecroAst*           file_top   = base.ast.root;
    NecroPagedArena*    arena      = &base.ast.arena;
    NecroAst*           top        = NULL;
    necro_parse_ast_arena_destroy(&parse_ast);
    necro_destroy_lex_token_vector(&lex_tokens);


    necro_kind_init_kinds(&base, scoped_symtable, intern);

    /*
        HACK: Magical BlockSize :: Nat data type
        Magically appears as the value of the current block size, fixed at compile time.
        We need to actually funnel the real block size into this...
        Probably should create a better way of handling this...
    */
    NecroAstSymbol* block_size_symbol     = necro_ast_symbol_create(arena, necro_intern_string(intern, "Necro.Base.BlockSize"), necro_intern_string(intern, "BlockSize"), necro_intern_string(intern, "Necro.Base"), NULL);
    block_size_symbol->type               = necro_type_con_create(arena, block_size_symbol, NULL);
    block_size_symbol->type->kind         = base.nat_kind->type;
    block_size_symbol->type->pre_supplied = true;
    necro_scope_insert_ast_symbol(arena, scoped_symtable->top_type_scope, block_size_symbol);

    // primUndefined
    top = necro_ast_create_top_decl(arena, necro_ast_create_fn_type_sig(arena, intern, "primUndefined", NULL,
        necro_ast_create_type_attribute(arena, necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR), NECRO_TYPE_ATTRIBUTE_DOT),
        NECRO_VAR_SIG, NECRO_SIG_DECLARATION), NULL);
    base.ast.root                = top;
    NecroAst* prim_undefined_ast = necro_ast_create_simple_assignment(arena, intern, "primUndefined", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "primUndefined", NECRO_VAR_VAR), NULL));
    necro_append_top(arena, top, prim_undefined_ast);

    // Share Type
    // Primitive and Magical Share
    {
        NecroAstSymbol* share_type_symbol     = necro_ast_symbol_create(arena, necro_intern_string(intern, "Necro.Base.Share"), necro_intern_string(intern, "Share"), necro_intern_string(intern, "Necro.Base"), NULL);
        share_type_symbol->type               = necro_type_con_create(arena, share_type_symbol, NULL);
        share_type_symbol->type->kind         = necro_type_fn_create(arena, base.star_kind->type, base.star_kind->type);
        share_type_symbol->type->pre_supplied = true;
        share_type_symbol->is_primitive       = true;
        share_type_symbol->is_wrapper         = true;
        necro_scope_insert_ast_symbol(arena, scoped_symtable->top_type_scope, share_type_symbol);
        // Share con
        // Primitive and Magical Share
        NecroAstSymbol* share_con_symbol     = necro_ast_symbol_create(arena, necro_intern_string(intern, "Necro.Base.Share"), necro_intern_string(intern, "Share"), necro_intern_string(intern, "Necro.Base"), NULL);
        NecroAstSymbol* uvar_symbol          = necro_ast_symbol_create(arena, necro_intern_unique_string(intern, "Necro.Base.u"), necro_intern_string(intern, "u"), necro_intern_string(intern, "Necro.Base"), NULL);
        NecroAstSymbol* avar_symbol          = necro_ast_symbol_create(arena, necro_intern_unique_string(intern, "Necro.Base.a"), necro_intern_string(intern, "a"), necro_intern_string(intern, "Necro.Base"), NULL);
        NecroType*      uvar_type            = necro_type_var_create(arena, uvar_symbol, NULL);
        uvar_type->var.is_rigid              = true;
        uvar_type->kind                      = base.ownership_kind->type;
        uvar_type->ownership                 = NULL;
        uvar_symbol->type                    = uvar_type;
        NecroType*      avar_type            = necro_type_var_create(arena, avar_symbol, NULL);
        avar_type->var.is_rigid              = true;
        avar_type->kind                      = base.star_kind->type;
        avar_type->ownership                 = base.ownership_share->type;
        avar_symbol->type                    = avar_type;
        NecroType*      con_type             = necro_type_con_create(arena, share_type_symbol, necro_type_list_create(arena, avar_type, NULL));
        con_type->kind                       = base.star_kind->type;
        con_type->ownership                  = uvar_type;
        NecroType*      fn_type              = necro_type_fn_create(arena, avar_type, con_type);
        fn_type->kind                        = base.star_kind->type;
        fn_type->ownership                   = base.ownership_share->type;
        NecroType*      for_all_a_type       = necro_type_for_all_create(arena, avar_symbol, fn_type);
        for_all_a_type->kind                 = base.star_kind->type;
        for_all_a_type->ownership            = uvar_type;
        NecroType*      for_all_u_type       = necro_type_for_all_create(arena, uvar_symbol, for_all_a_type);
        for_all_u_type->kind                 = base.star_kind->type;
        for_all_u_type->ownership            = base.ownership_share->type;
        share_con_symbol->type               = for_all_u_type;
        share_con_symbol->type->pre_supplied = true;
        share_con_symbol->is_primitive       = true;
        share_con_symbol->is_wrapper         = true;
        share_con_symbol->is_constructor     = true;
        necro_scope_insert_ast_symbol(arena, scoped_symtable->top_scope, share_con_symbol);
    }

    // ()
    NecroAst* unit_s_type            = necro_ast_create_simple_type(arena, intern, "()", NULL);
    NecroAst* unit_constructor       = necro_ast_create_data_con(arena, intern, "()", NULL);
    NecroAst* unit_constructor_list  = necro_ast_create_list(arena, unit_constructor, NULL);
    necro_append_top(arena, top, necro_ast_create_data_declaration(arena, intern, unit_s_type, unit_constructor_list));

    {
        // TODO: Make proper instance methods for (), not relying on primUndefined
        //--------------
        // Eq ()
        NecroAst* eq_method_list =
            necro_ast_create_decl(arena, necro_ast_create_prim_bin_op_method_ast(arena, intern, "eq"),
                necro_ast_create_decl(arena, necro_ast_create_prim_bin_op_method_ast(arena, intern, "neq"), NULL));
        necro_append_top(arena, top, necro_ast_create_instance(arena, intern, "Eq", necro_base_make_num_con(arena, intern, "()", false), necro_base_make_num_context(arena, intern, "Eq", false), eq_method_list));

        //--------------
        // Ord ()
        NecroAst* ord_method_list =
            necro_ast_create_decl(arena, necro_ast_create_prim_bin_op_method_ast(arena, intern, "lt"),
                necro_ast_create_decl(arena, necro_ast_create_prim_bin_op_method_ast(arena, intern, "gt"),
                    necro_ast_create_decl(arena, necro_ast_create_prim_bin_op_method_ast(arena, intern, "lte"),
                        necro_ast_create_decl(arena, necro_ast_create_prim_bin_op_method_ast(arena, intern, "gte"), NULL))));
        necro_append_top(arena, top, necro_ast_create_instance(arena, intern, "Ord", necro_base_make_num_con(arena, intern, "()", false), necro_base_make_num_context(arena, intern, "Ord", false), ord_method_list));
    }

    // []
    // NecroAst* list_s_type            = necro_ast_create_simple_type(arena, intern, "[]", necro_ast_create_var_list(arena, intern, 1, NECRO_VAR_TYPE_VAR_DECLARATION));
    // NecroAst* list_app               = necro_ast_create_type_app(arena, necro_ast_create_conid(arena, intern, "[]", NECRO_CON_TYPE_VAR), necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR));
    // NecroAst* cons_args              = necro_ast_create_list(arena, necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR), necro_ast_create_list(arena, list_app, NULL));
    // NecroAst* cons_constructor       = necro_ast_create_data_con(arena, intern, ":", cons_args);
    // NecroAst* nil_constructor        = necro_ast_create_data_con(arena, intern, "[]", NULL);
    // NecroAst* list_constructor_list  = necro_ast_create_list(arena, cons_constructor, necro_ast_create_list(arena, nil_constructor, NULL));
    // necro_append_top(arena, top, necro_ast_create_data_declaration(arena, intern, list_s_type, list_constructor_list));

    // // Maybe
    // NecroAst* maybe_s_type           = necro_ast_create_simple_type(arena, intern, "Maybe", necro_ast_create_var_list(arena, intern, 1, NECRO_VAR_TYPE_VAR_DECLARATION));
    // NecroAst* just_constructor       = necro_ast_create_data_con(arena, intern, "Just", necro_ast_create_list(arena, necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR), NULL));
    // NecroAst* nothing_constructor    = necro_ast_create_data_con(arena, intern, "Nothing", NULL);
    // NecroAst* maybe_constructor_list = necro_ast_create_list(arena, just_constructor, necro_ast_create_list(arena, nothing_constructor, NULL));
    // necro_append_top(arena, top, necro_ast_create_data_declaration(arena, intern, maybe_s_type, maybe_constructor_list));

    // Bool
    NecroAst* bool_s_type            = necro_ast_create_simple_type(arena, intern, "Bool", NULL);
    NecroAst* false_constructor      = necro_ast_create_data_con(arena, intern, "False", NULL);
    NecroAst* true_constructor       = necro_ast_create_data_con(arena, intern, "True", NULL);
    NecroAst* bool_constructor_list  = necro_ast_create_list(arena, false_constructor, necro_ast_create_list(arena, true_constructor, NULL));
    necro_append_top(arena, top, necro_ast_create_data_declaration(arena, intern, bool_s_type, bool_constructor_list));

    // Simple Data Decls
    necro_base_create_simple_data_decl(arena, top, intern, "World");
    necro_base_create_simple_data_decl(arena, top, intern, "Int");
    necro_base_create_simple_data_decl(arena, top, intern, "UInt");
    necro_base_create_simple_data_decl(arena, top, intern, "Float");
    necro_base_create_simple_data_decl(arena, top, intern, "Char");
    necro_base_create_simple_data_decl(arena, top, intern, "F64");
    necro_base_create_simple_data_decl(arena, top, intern, "I64");

    // _project :: a -> UInt -> b, primitive data structure projection
    {
        necro_append_top(arena, top,
            necro_ast_create_fn_type_sig(arena, intern, "_project", NULL,
                necro_ast_create_type_fn(arena,
                    necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR),
                    necro_ast_create_type_fn(arena,
                        necro_ast_create_conid(arena, intern, "Int", NECRO_CON_TYPE_VAR),
                        necro_ast_create_var(arena, intern, "b", NECRO_VAR_TYPE_FREE_VAR))),
                NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top,
            necro_ast_create_apats_assignment(arena, intern, "_project",
                necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "con", NECRO_VAR_DECLARATION), necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "slot", NECRO_VAR_DECLARATION), NULL)),
                necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "primUndefined", NECRO_VAR_VAR), NULL)));
    }

    // TODO: Finish!
    // necro_base_create_simple_data_decl(arena, top, intern, "Rational");

    // Simple Poly Data Decls
    // necro_base_create_simple_poly_data_decl(arena, top, intern, "Pattern");
    necro_base_create_simple_poly_data_decl(arena, top, intern, "Ptr");
    // necro_base_create_simple_poly_data_decl(arena, top, intern, "Event");

    // Array
    NecroAst* array_n_type   = necro_ast_create_type_signature(arena, NECRO_SIG_TYPE_VAR, necro_ast_create_var(arena, intern, "n", NECRO_VAR_TYPE_VAR_DECLARATION), NULL, necro_ast_create_conid(arena, intern, "Nat", NECRO_CON_TYPE_VAR));
    NecroAst* array_s_type   = necro_ast_create_simple_type(arena, intern, "Array", necro_ast_create_list(arena, array_n_type, necro_ast_create_var_list(arena, intern, 1, NECRO_VAR_TYPE_VAR_DECLARATION)));
    NecroAst* array_args     = necro_ast_create_list(arena, necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR), NULL);
    NecroAst* array_con      = necro_ast_create_data_con(arena, intern, "Array", array_args);
    NecroAst* array_con_list = necro_ast_create_list(arena, array_con, NULL);
    necro_append_top(arena, top, necro_ast_create_data_declaration(arena, intern, array_s_type, array_con_list));

    // Range
    NecroAst* range_n_type   = necro_ast_create_type_signature(arena, NECRO_SIG_TYPE_VAR, necro_ast_create_var(arena, intern, "n", NECRO_VAR_TYPE_VAR_DECLARATION), NULL, necro_ast_create_conid(arena, intern, "Nat", NECRO_CON_TYPE_VAR));
    NecroAst* range_s_type   = necro_ast_create_simple_type(arena, intern, "Range", necro_ast_create_list(arena, range_n_type, NULL));
    NecroAst* range_args     =
        necro_ast_create_list(arena,
            necro_ast_create_conid(arena, intern, "UInt", NECRO_CON_TYPE_VAR),
            necro_ast_create_list(arena,
                necro_ast_create_conid(arena, intern, "UInt", NECRO_CON_TYPE_VAR),
                necro_ast_create_list(arena,
                    necro_ast_create_conid(arena, intern, "UInt", NECRO_CON_TYPE_VAR),
                    NULL)));
    NecroAst* range_con      = necro_ast_create_data_con(arena, intern, "Range", range_args);
    NecroAst* range_con_list = necro_ast_create_list(arena, range_con, NULL);
    necro_append_top(arena, top, necro_ast_create_data_declaration(arena, intern, range_s_type, range_con_list));

    // Index
    NecroAst* index_n_type   = necro_ast_create_type_signature(arena, NECRO_SIG_TYPE_VAR, necro_ast_create_var(arena, intern, "n", NECRO_VAR_TYPE_VAR_DECLARATION), NULL, necro_ast_create_conid(arena, intern, "Nat", NECRO_CON_TYPE_VAR));
    NecroAst* index_s_type   = necro_ast_create_simple_type(arena, intern, "Index", necro_ast_create_list(arena, index_n_type, NULL));
    NecroAst* index_args     = necro_ast_create_list(arena, necro_ast_create_conid(arena, intern, "Int", NECRO_CON_TYPE_VAR), NULL);
    NecroAst* index_con      = necro_ast_create_data_con(arena, intern, "Index", index_args);
    NecroAst* index_con_list = necro_ast_create_list(arena, index_con, NULL);
    necro_append_top(arena, top, necro_ast_create_data_declaration(arena, intern, index_s_type, index_con_list));

    // each value
    {
        necro_append_top(arena, top, necro_ast_create_fn_type_sig(arena, intern, "each", NULL,
            necro_ast_create_type_app(arena,
                necro_ast_create_conid(arena, intern, "Range", NECRO_CON_TYPE_VAR),
                necro_ast_create_var(arena, intern, "n", NECRO_VAR_TYPE_FREE_VAR)),
            NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, "each",
            necro_ast_create_rhs(arena,
                necro_ast_create_fexpr(arena,
                    necro_ast_create_fexpr(arena,
                        necro_ast_create_fexpr(arena,
                            necro_ast_create_conid(arena, intern, "Range", NECRO_CON_VAR),
                            necro_ast_create_fexpr(arena, necro_ast_create_var(arena, intern, "fromInt", NECRO_VAR_VAR), necro_ast_create_constant(arena, (NecroParseAstConstant) { .type = NECRO_AST_CONSTANT_INTEGER, .int_literal = 0 }))),
                        necro_ast_create_fexpr(arena, necro_ast_create_var(arena, intern, "fromInt", NECRO_VAR_VAR), necro_ast_create_constant(arena, (NecroParseAstConstant) { .type = NECRO_AST_CONSTANT_INTEGER, .int_literal = 1 }))),
                    necro_ast_create_fexpr(arena, necro_ast_create_var(arena, intern, "fromInt", NECRO_VAR_VAR), necro_ast_create_constant(arena, (NecroParseAstConstant) { .type = NECRO_AST_CONSTANT_INTEGER, .int_literal = 0 }))),
                NULL)));
    }

    //--------------------
    // Classes
    //--------------------

    // Eq
    NecroAst* eq_method_sig  = necro_base_create_class_comp_sig(arena, intern, "eq");
    NecroAst* neq_method_sig = necro_base_create_class_comp_sig(arena, intern, "neq");
    NecroAst* eq_method_list = necro_ast_create_decl(arena, eq_method_sig, necro_ast_create_decl(arena, neq_method_sig, NULL));
    NecroAst* eq_class_ast   = necro_ast_create_type_class(arena, intern, "Eq", "a", NULL, eq_method_list);
    necro_append_top(arena, top, eq_class_ast);

    // Ord
    NecroAst* gt_method_sig   = necro_base_create_class_comp_sig(arena, intern, "gt");
    NecroAst* lt_method_sig   = necro_base_create_class_comp_sig(arena, intern, "lt");
    NecroAst* gte_method_sig  = necro_base_create_class_comp_sig(arena, intern, "gte");
    NecroAst* lte_method_sig  = necro_base_create_class_comp_sig(arena, intern, "lte");
    NecroAst* ord_method_list =
        necro_ast_create_decl(arena, gt_method_sig,
            necro_ast_create_decl(arena, lt_method_sig,
                necro_ast_create_decl(arena, gte_method_sig,
                    necro_ast_create_decl(arena, lte_method_sig, NULL))));
    NecroAst* ord_class_ast   = necro_ast_create_type_class(arena, intern, "Ord", "a", necro_ast_create_context(arena, intern, "Eq", "a", NULL), ord_method_list);
    necro_append_top(arena, top, ord_class_ast);

    // Num
    NecroAst* add_method_sig    = necro_base_create_class_bin_op_sig(arena, intern, "add");
    NecroAst* sub_method_sig    = necro_base_create_class_bin_op_sig(arena, intern, "sub");
    NecroAst* mul_method_sig    = necro_base_create_class_bin_op_sig(arena, intern, "mul");
    NecroAst* abs_method_sig    = necro_base_create_class_unary_op_sig(arena, intern, "abs");
    NecroAst* signum_method_sig = necro_base_create_class_unary_op_sig(arena, intern, "signum");
    NecroAst* from_int_sig      =
        necro_ast_create_fn_type_sig(arena, intern, "fromInt", NULL, necro_ast_create_type_fn(arena,
            necro_ast_create_conid(arena, intern, "Int", NECRO_CON_TYPE_VAR),
            necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR)), NECRO_VAR_CLASS_SIG, NECRO_SIG_TYPE_CLASS);
    NecroAst* num_method_list =
        necro_ast_create_decl(arena, add_method_sig,
            necro_ast_create_decl(arena, sub_method_sig,
                necro_ast_create_decl(arena, mul_method_sig,
                    necro_ast_create_decl(arena, abs_method_sig,
                        necro_ast_create_decl(arena, signum_method_sig,
                            necro_ast_create_decl(arena, from_int_sig, NULL))))));
    NecroAst* num_class_ast = necro_ast_create_type_class(arena, intern, "Num", "a", NULL, num_method_list);
    necro_append_top(arena, top, num_class_ast);

    // Frac
    NecroAst* div_method_sig    = necro_base_create_class_bin_op_sig(arena, intern, "div");
    NecroAst* recip_method_sig  = necro_base_create_class_unary_op_sig(arena, intern, "recip");
    NecroAst* from_rational_sig =
        necro_ast_create_fn_type_sig(arena, intern, "fromRational", NULL, necro_ast_create_type_fn(arena,
            necro_ast_create_conid(arena, intern, "Float", NECRO_CON_TYPE_VAR),
            necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR)), NECRO_VAR_CLASS_SIG, NECRO_SIG_TYPE_CLASS);
    NecroAst* fractional_method_list =
        necro_ast_create_decl(arena, div_method_sig,
            necro_ast_create_decl(arena, recip_method_sig,
                necro_ast_create_decl(arena, from_rational_sig, NULL)));
    NecroAst* fractional_class_ast = necro_ast_create_type_class(arena, intern, "Fractional", "a", necro_ast_create_context(arena, intern, "Num", "a", NULL), fractional_method_list);
    necro_append_top(arena, top, fractional_class_ast);

    // TODO: Put Static constraints on this
    NecroAst* default_type_sig    = necro_ast_create_fn_type_sig(arena, intern, "default", NULL, necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR), NECRO_VAR_CLASS_SIG, NECRO_SIG_TYPE_CLASS);
    NecroAst* default_method_list = necro_ast_create_decl(arena, default_type_sig, NULL);
    NecroAst* default_class_ast   = necro_ast_create_type_class(arena, intern, "Default", "a", NULL, default_method_list);
    necro_append_top(arena, top, default_class_ast);

    // Num instances
    necro_base_create_prim_num_instances(arena, top, intern, "Int", false, true, false);
    necro_base_create_prim_num_instances(arena, top, intern, "UInt", false, true, false);
    necro_base_create_prim_num_instances(arena, top, intern, "Float", false, true, true);
    // necro_base_create_prim_num_instances(arena, top, intern, "Pattern", true, true, true);
    necro_base_create_prim_num_instances(arena, top, intern, "Bool", false, false, false);
    necro_base_create_prim_num_instances(arena, top, intern, "I64", false, true, false);
    necro_base_create_prim_num_instances(arena, top, intern, "F64", false, true, true);

    // TODO: Finish!
    // necro_base_create_prim_num_instances(arena, top, intern, "Rational", false, true, true);

    // (,)
    NecroAst* tuple_2_s_type      = necro_ast_create_simple_type(arena, intern, "(,)", necro_ast_create_var_list(arena, intern, 2, NECRO_VAR_TYPE_VAR_DECLARATION));
    NecroAst* tuple_2_constructor = necro_ast_create_data_con(arena, intern, "(,)", necro_ast_create_var_list(arena, intern, 2, NECRO_VAR_TYPE_FREE_VAR));
    necro_append_top(arena, top, necro_ast_create_data_declaration(arena, intern, tuple_2_s_type, necro_ast_create_list(arena, tuple_2_constructor, NULL)));

    // (,,)
    NecroAst* tuple_3_s_type      = necro_ast_create_simple_type(arena, intern, "(,,)", necro_ast_create_var_list(arena, intern, 3, NECRO_VAR_TYPE_VAR_DECLARATION));
    NecroAst* tuple_3_constructor = necro_ast_create_data_con(arena, intern, "(,,)", necro_ast_create_var_list(arena, intern, 3, NECRO_VAR_TYPE_FREE_VAR));
    necro_append_top(arena, top, necro_ast_create_data_declaration(arena, intern, tuple_3_s_type, necro_ast_create_list(arena, tuple_3_constructor, NULL)));

    // (,,,)
    NecroAst* tuple_4_s_type      = necro_ast_create_simple_type(arena, intern, "(,,,)", necro_ast_create_var_list(arena, intern, 4, NECRO_VAR_TYPE_VAR_DECLARATION));
    NecroAst* tuple_4_constructor = necro_ast_create_data_con(arena, intern, "(,,,)", necro_ast_create_var_list(arena, intern, 4, NECRO_VAR_TYPE_FREE_VAR));
    necro_append_top(arena, top, necro_ast_create_data_declaration(arena, intern, tuple_4_s_type, necro_ast_create_list(arena, tuple_4_constructor, NULL)));

    // (,,,,)
    NecroAst* tuple_5_s_type      = necro_ast_create_simple_type(arena, intern, "(,,,,)", necro_ast_create_var_list(arena, intern, 5, NECRO_VAR_TYPE_VAR_DECLARATION));
    NecroAst* tuple_5_constructor = necro_ast_create_data_con(arena, intern, "(,,,,)", necro_ast_create_var_list(arena, intern, 5, NECRO_VAR_TYPE_FREE_VAR));
    necro_append_top(arena, top, necro_ast_create_data_declaration(arena, intern, tuple_5_s_type, necro_ast_create_list(arena, tuple_5_constructor, NULL)));

    // (,,,,,)
    NecroAst* tuple_6_s_type      = necro_ast_create_simple_type(arena, intern, "(,,,,,)", necro_ast_create_var_list(arena, intern, 6, NECRO_VAR_TYPE_VAR_DECLARATION));
    NecroAst* tuple_6_constructor = necro_ast_create_data_con(arena, intern, "(,,,,,)", necro_ast_create_var_list(arena, intern, 6, NECRO_VAR_TYPE_FREE_VAR));
    necro_append_top(arena, top, necro_ast_create_data_declaration(arena, intern, tuple_6_s_type, necro_ast_create_list(arena, tuple_6_constructor, NULL)));

    // (,,,,,,)
    NecroAst* tuple_7_s_type      = necro_ast_create_simple_type(arena, intern, "(,,,,,,)", necro_ast_create_var_list(arena, intern, 7, NECRO_VAR_TYPE_VAR_DECLARATION));
    NecroAst* tuple_7_constructor = necro_ast_create_data_con(arena, intern, "(,,,,,,)", necro_ast_create_var_list(arena, intern, 7, NECRO_VAR_TYPE_FREE_VAR));
    necro_append_top(arena, top, necro_ast_create_data_declaration(arena, intern, tuple_7_s_type, necro_ast_create_list(arena, tuple_7_constructor, NULL)));

    // (,,,,,,,)
    NecroAst* tuple_8_s_type      = necro_ast_create_simple_type(arena, intern, "(,,,,,,,)", necro_ast_create_var_list(arena, intern, 8, NECRO_VAR_TYPE_VAR_DECLARATION));
    NecroAst* tuple_8_constructor = necro_ast_create_data_con(arena, intern, "(,,,,,,,)", necro_ast_create_var_list(arena, intern, 8, NECRO_VAR_TYPE_FREE_VAR));
    necro_append_top(arena, top, necro_ast_create_data_declaration(arena, intern, tuple_8_s_type, necro_ast_create_list(arena, tuple_8_constructor, NULL)));

    // (,,,,,,,,)
    NecroAst* tuple_9_s_type      = necro_ast_create_simple_type(arena, intern, "(,,,,,,,,)", necro_ast_create_var_list(arena, intern, 9, NECRO_VAR_TYPE_VAR_DECLARATION));
    NecroAst* tuple_9_constructor = necro_ast_create_data_con(arena, intern, "(,,,,,,,,)", necro_ast_create_var_list(arena, intern, 9, NECRO_VAR_TYPE_FREE_VAR));
    necro_append_top(arena, top, necro_ast_create_data_declaration(arena, intern, tuple_9_s_type, necro_ast_create_list(arena, tuple_9_constructor, NULL)));

    // (,,,,,,,,,)
    NecroAst* tuple_10_s_type      = necro_ast_create_simple_type(arena, intern, "(,,,,,,,,,)", necro_ast_create_var_list(arena, intern, 10, NECRO_VAR_TYPE_VAR_DECLARATION));
    NecroAst* tuple_10_constructor = necro_ast_create_data_con(arena, intern, "(,,,,,,,,,)", necro_ast_create_var_list(arena, intern, 10, NECRO_VAR_TYPE_FREE_VAR));
    necro_append_top(arena, top, necro_ast_create_data_declaration(arena, intern, tuple_10_s_type, necro_ast_create_list(arena, tuple_10_constructor, NULL)));

    for (size_t i = 2; i <= NECRO_MAX_UNBOXED_TUPLE_TYPES; ++i)
    {
        char name[16] = "";
        snprintf(name, 16, "(#,#)%zu", i);
        NecroAst* unboxed_tuple_s_type      = necro_ast_create_simple_type(arena, intern, name, necro_ast_create_var_list(arena, intern, i, NECRO_VAR_TYPE_VAR_DECLARATION));
        NecroAst* unboxed_tuple_constructor = necro_ast_create_data_con(arena, intern, name, necro_ast_create_var_list(arena, intern, i, NECRO_VAR_TYPE_FREE_VAR));
        necro_append_top(arena, top, necro_ast_create_data_declaration(arena, intern, unboxed_tuple_s_type, necro_ast_create_list(arena, unboxed_tuple_constructor, NULL)));
    }

    for (size_t i = 0; i < NECRO_MAX_ENV_TYPES; ++i)
    {
        char env_name[16] = "";
        snprintf(env_name, 16, "Env%zu", i);
        NecroAst* env_s_type      = necro_ast_create_simple_type(arena, intern, env_name, necro_ast_create_var_list(arena, intern, i, NECRO_VAR_TYPE_VAR_DECLARATION));
        NecroAst* env_constructor = necro_ast_create_data_con(arena, intern, env_name, necro_ast_create_var_list(arena, intern, i, NECRO_VAR_TYPE_FREE_VAR));
        necro_append_top(arena, top, necro_ast_create_data_declaration(arena, intern, env_s_type, necro_ast_create_list(arena, env_constructor, NULL)));
    }

    // for (size_t i = 2; i < NECRO_MAX_BRANCH_TYPES; ++i)
    // {
    //     char branch_fn_name[24] = "";
    //     snprintf(branch_fn_name, 24, "BranchFn%zu", i);
    //     NecroAst* branch_fn_s_type = necro_ast_create_simple_type(arena, intern, branch_fn_name, necro_ast_create_var_list(arena, intern, i, NECRO_VAR_TYPE_VAR_DECLARATION));
    //     NecroAst* branch_fn_cons   = NULL;
    //     for (size_t i2 = 0; i2 < i; ++i2)
    //     {
    //         snprintf(branch_fn_name, 24, "BranchFn%zuAlt%zu", i, i2);
    //         NecroAst* branch_fn_con_var     = necro_ast_create_var(arena, intern, var_names[i2], NECRO_VAR_TYPE_FREE_VAR);
    //         NecroAst* branch_fn_constructor = necro_ast_create_data_con(arena, intern, branch_fn_name, necro_ast_create_list(arena, branch_fn_con_var, NULL));
    //         branch_fn_cons                  = necro_ast_create_list(arena, branch_fn_constructor, branch_fn_cons);
    //     }
    //     necro_append_top(arena, top, necro_ast_create_data_declaration(arena, intern, branch_fn_s_type, branch_fn_cons));
    // }

    // Functor
    {
        NecroAst* a_var          = necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR);
        NecroAst* b_var          = necro_ast_create_var(arena, intern, "b", NECRO_VAR_TYPE_FREE_VAR);
        NecroAst* f_var          = necro_ast_create_var(arena, intern, "f", NECRO_VAR_TYPE_FREE_VAR);
        NecroAst* map_method_sig =
            necro_ast_create_fn_type_sig(arena, intern, "map", NULL,
                necro_ast_create_type_fn(arena,
                    necro_ast_create_type_fn(arena, a_var, b_var),
                    necro_ast_create_type_fn(arena,
                        necro_ast_create_type_app(arena, f_var, a_var),
                        necro_ast_create_type_app(arena, f_var, b_var))),
                    NECRO_VAR_CLASS_SIG, NECRO_SIG_TYPE_CLASS);
        NecroAst* functor_method_list = necro_ast_create_decl(arena, map_method_sig, NULL);
        necro_append_top(arena, top, necro_ast_create_type_class(arena, intern, "Functor", "f", NULL, functor_method_list));
    }

    // Applicative
    {
        NecroAst* a_var           = necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR);
        NecroAst* b_var           = necro_ast_create_var(arena, intern, "b", NECRO_VAR_TYPE_FREE_VAR);
        NecroAst* f_var           = necro_ast_create_var(arena, intern, "f", NECRO_VAR_TYPE_FREE_VAR);
        NecroAst* pure_method_sig =
            necro_ast_create_fn_type_sig(arena, intern, "pure", NULL, necro_ast_create_type_fn(arena, a_var, necro_ast_create_type_app(arena, f_var, a_var)), NECRO_VAR_CLASS_SIG, NECRO_SIG_TYPE_CLASS);
        NecroAst* ap_method_sig  =
            necro_ast_create_fn_type_sig(arena, intern, "ap", NULL,
                necro_ast_create_type_fn(arena,
                    necro_ast_create_type_app(arena, f_var, necro_ast_create_type_fn(arena, a_var, b_var)),
                    necro_ast_create_type_fn(arena,
                        necro_ast_create_type_app(arena, f_var, a_var),
                        necro_ast_create_type_app(arena, f_var, b_var))),
                    NECRO_VAR_CLASS_SIG, NECRO_SIG_TYPE_CLASS);
        NecroAst* applicative_method_list = necro_ast_create_decl(arena, pure_method_sig, necro_ast_create_decl(arena, ap_method_sig, NULL));
        necro_append_top(arena, top, necro_ast_create_type_class(arena, intern, "Applicative", "f", necro_ast_create_context(arena, intern, "Functor", "f", NULL), applicative_method_list));
    }

    // Monad
    {
        NecroAst* a_var           = necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR);
        NecroAst* b_var           = necro_ast_create_var(arena, intern, "b", NECRO_VAR_TYPE_FREE_VAR);
        NecroAst* m_var           = necro_ast_create_var(arena, intern, "m", NECRO_VAR_TYPE_FREE_VAR);
        NecroAst* bind_method_sig  =
            necro_ast_create_fn_type_sig(arena, intern, "bind", NULL,
                necro_ast_create_type_fn(arena,
                    necro_ast_create_type_app(arena, m_var, a_var),
                    necro_ast_create_type_fn(arena,
                        necro_ast_create_type_fn(arena, a_var, necro_ast_create_type_app(arena, m_var, b_var)),
                        necro_ast_create_type_app(arena, m_var, b_var))),
                    NECRO_VAR_CLASS_SIG, NECRO_SIG_TYPE_CLASS);
        NecroAst* monad_method_list = necro_ast_create_decl(arena, bind_method_sig, NULL);
        necro_append_top(arena, top, necro_ast_create_type_class(arena, intern, "Monad", "m", necro_ast_create_context(arena, intern, "Applicative", "m", NULL), monad_method_list));
    }

    // Semigroup
    {
        NecroAst* a_var             = necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR);
        NecroAst* append_method_sig =
            necro_ast_create_fn_type_sig(arena, intern, "append", NULL,
                necro_ast_create_type_fn(arena, a_var, necro_ast_create_type_fn(arena, a_var, a_var)),
                    NECRO_VAR_CLASS_SIG, NECRO_SIG_TYPE_CLASS);
        NecroAst* semigroup_method_list = necro_ast_create_decl(arena, append_method_sig, NULL);
        necro_append_top(arena, top, necro_ast_create_type_class(arena, intern, "Semigroup", "a", NULL, semigroup_method_list));
    }

    // Monoid
    {
        NecroAst* a_var             = necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR);
        NecroAst* mempty_method_sig =
            necro_ast_create_fn_type_sig(arena, intern, "mempty", NULL,
                a_var,
                    NECRO_VAR_CLASS_SIG, NECRO_SIG_TYPE_CLASS);
        NecroAst* monoid_method_list = necro_ast_create_decl(arena, mempty_method_sig, NULL);
        necro_append_top(arena, top, necro_ast_create_type_class(arena, intern, "Monoid", "a", necro_ast_create_context(arena, intern, "Semigroup", "a", NULL), monoid_method_list));
    }

    //--------------------
    // BinOps
    //--------------------

    // +
    necro_append_top(arena, top, necro_base_create_num_bin_op_sig(arena, intern, "+"));
    necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, "+", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "add", NECRO_VAR_VAR), NULL)));

    // -
    necro_append_top(arena, top, necro_base_create_num_bin_op_sig(arena, intern, "-"));
    necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, "-", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "sub", NECRO_VAR_VAR), NULL)));

    // *
    necro_append_top(arena, top, necro_base_create_num_bin_op_sig(arena, intern, "*"));
    necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, "*", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "mul", NECRO_VAR_VAR), NULL)));

    // /
    necro_append_top(arena, top, necro_base_create_fractional_bin_op_sig(arena, intern, "/"));
    necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, "/", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "div", NECRO_VAR_VAR), NULL)));

    // ==
    necro_append_top(arena, top, necro_base_create_eq_comp_sig(arena, intern, "=="));
    necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, "==", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "eq", NECRO_VAR_VAR), NULL)));

    // /=
    necro_append_top(arena, top,  necro_base_create_eq_comp_sig(arena, intern, "/="));
    necro_append_top(arena, top,  necro_ast_create_simple_assignment(arena, intern, "/=", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "neq", NECRO_VAR_VAR), NULL)));

    // <
    necro_append_top(arena, top, necro_base_create_ord_comp_sig(arena, intern, "<"));
    necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, "<", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "lt", NECRO_VAR_VAR), NULL)));

    // >
    necro_append_top(arena, top, necro_base_create_ord_comp_sig(arena, intern, ">"));
    necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, ">", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "gt", NECRO_VAR_VAR), NULL)));

    // <=
    necro_append_top(arena, top, necro_base_create_ord_comp_sig(arena, intern, "<="));
    necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, "<=", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "lte", NECRO_VAR_VAR), NULL)));

    // >=
    necro_append_top(arena, top, necro_base_create_ord_comp_sig(arena, intern, ">="));
    necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, ">=", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "gte", NECRO_VAR_VAR), NULL)));

    // <>
    {
        NecroAst* a_var  = necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR);
        NecroAst* op_sig =
            necro_ast_create_fn_type_sig(arena, intern, "<>", necro_ast_create_context(arena, intern, "Semigroup", "a", NULL),
                necro_ast_create_type_fn(arena, a_var, necro_ast_create_type_fn(arena, a_var, a_var)), NECRO_VAR_SIG, NECRO_SIG_DECLARATION);
        necro_append_top(arena, top, op_sig);
        necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, "<>", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "append", NECRO_VAR_VAR), NULL)));
    }

    // @+
    {
        NecroAst* seq_type = necro_ast_create_type_app(arena, necro_ast_create_conid(arena, intern, "Seq", NECRO_CON_TYPE_VAR), necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR));
        necro_append_top(arena, top, necro_ast_create_fn_type_sig(arena, intern, "@+", necro_ast_create_context(arena, intern, "Num", "a", NULL), necro_ast_create_type_fn(arena, seq_type, necro_ast_create_type_fn(arena, seq_type, seq_type)), NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, "@+", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "addSeqOnLeft", NECRO_VAR_VAR), NULL)));
    }

    // +@
    {
        NecroAst* seq_type = necro_ast_create_type_app(arena, necro_ast_create_conid(arena, intern, "Seq", NECRO_CON_TYPE_VAR), necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR));
        necro_append_top(arena, top, necro_ast_create_fn_type_sig(arena, intern, "+@", necro_ast_create_context(arena, intern, "Num", "a", NULL), necro_ast_create_type_fn(arena, seq_type, necro_ast_create_type_fn(arena, seq_type, seq_type)), NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, "+@", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "addSeqOnRight", NECRO_VAR_VAR), NULL)));
    }

    // @-
    {
        NecroAst* seq_type = necro_ast_create_type_app(arena, necro_ast_create_conid(arena, intern, "Seq", NECRO_CON_TYPE_VAR), necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR));
        necro_append_top(arena, top, necro_ast_create_fn_type_sig(arena, intern, "@-", necro_ast_create_context(arena, intern, "Num", "a", NULL), necro_ast_create_type_fn(arena, seq_type, necro_ast_create_type_fn(arena, seq_type, seq_type)), NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, "@-", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "subSeqOnLeft", NECRO_VAR_VAR), NULL)));
    }

    // -@
    {
        NecroAst* seq_type = necro_ast_create_type_app(arena, necro_ast_create_conid(arena, intern, "Seq", NECRO_CON_TYPE_VAR), necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR));
        necro_append_top(arena, top, necro_ast_create_fn_type_sig(arena, intern, "-@", necro_ast_create_context(arena, intern, "Num", "a", NULL), necro_ast_create_type_fn(arena, seq_type, necro_ast_create_type_fn(arena, seq_type, seq_type)), NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, "-@", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "subSeqOnRight", NECRO_VAR_VAR), NULL)));
    }

    // @*
    {
        NecroAst* seq_type = necro_ast_create_type_app(arena, necro_ast_create_conid(arena, intern, "Seq", NECRO_CON_TYPE_VAR), necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR));
        necro_append_top(arena, top, necro_ast_create_fn_type_sig(arena, intern, "@*", necro_ast_create_context(arena, intern, "Num", "a", NULL), necro_ast_create_type_fn(arena, seq_type, necro_ast_create_type_fn(arena, seq_type, seq_type)), NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, "@*", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "mulSeqOnLeft", NECRO_VAR_VAR), NULL)));
    }

    // *@
    {
        NecroAst* seq_type = necro_ast_create_type_app(arena, necro_ast_create_conid(arena, intern, "Seq", NECRO_CON_TYPE_VAR), necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR));
        necro_append_top(arena, top, necro_ast_create_fn_type_sig(arena, intern, "*@", necro_ast_create_context(arena, intern, "Num", "a", NULL), necro_ast_create_type_fn(arena, seq_type, necro_ast_create_type_fn(arena, seq_type, seq_type)), NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, "*@", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "mulSeqOnRight", NECRO_VAR_VAR), NULL)));
    }

    // @/
    {
        NecroAst* seq_type = necro_ast_create_type_app(arena, necro_ast_create_conid(arena, intern, "Seq", NECRO_CON_TYPE_VAR), necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR));
        necro_append_top(arena, top, necro_ast_create_fn_type_sig(arena, intern, "@/", necro_ast_create_context(arena, intern, "Fractional", "a", NULL), necro_ast_create_type_fn(arena, seq_type, necro_ast_create_type_fn(arena, seq_type, seq_type)), NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, "@/", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "divSeqOnLeft", NECRO_VAR_VAR), NULL)));
    }

    // /@
    {
        NecroAst* seq_type = necro_ast_create_type_app(arena, necro_ast_create_conid(arena, intern, "Seq", NECRO_CON_TYPE_VAR), necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR));
        necro_append_top(arena, top, necro_ast_create_fn_type_sig(arena, intern, "/@", necro_ast_create_context(arena, intern, "Fractional", "a", NULL), necro_ast_create_type_fn(arena, seq_type, necro_ast_create_type_fn(arena, seq_type, seq_type)), NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, "/@", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "divSeqOnRight", NECRO_VAR_VAR), NULL)));
    }

    // @<
    {
        NecroAst* seq_type_a = necro_ast_create_type_app(arena, necro_ast_create_conid(arena, intern, "Seq", NECRO_CON_TYPE_VAR), necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR));
        NecroAst* seq_type_b = necro_ast_create_type_app(arena, necro_ast_create_conid(arena, intern, "Seq", NECRO_CON_TYPE_VAR), necro_ast_create_var(arena, intern, "b", NECRO_VAR_TYPE_FREE_VAR));
        necro_append_top(arena, top, necro_ast_create_fn_type_sig(arena, intern, "@<", NULL, necro_ast_create_type_fn(arena, seq_type_a, necro_ast_create_type_fn(arena, seq_type_b, seq_type_a)), NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, "@<", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "leftConstSeqOnLeft", NECRO_VAR_VAR), NULL)));
    }

    // <@
    {
        NecroAst* seq_type_a = necro_ast_create_type_app(arena, necro_ast_create_conid(arena, intern, "Seq", NECRO_CON_TYPE_VAR), necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR));
        NecroAst* seq_type_b = necro_ast_create_type_app(arena, necro_ast_create_conid(arena, intern, "Seq", NECRO_CON_TYPE_VAR), necro_ast_create_var(arena, intern, "b", NECRO_VAR_TYPE_FREE_VAR));
        necro_append_top(arena, top, necro_ast_create_fn_type_sig(arena, intern, "<@", NULL, necro_ast_create_type_fn(arena, seq_type_a, necro_ast_create_type_fn(arena, seq_type_b, seq_type_a)), NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, "<@", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "leftConstSeqOnRight", NECRO_VAR_VAR), NULL)));
    }

    // @>
    {
        NecroAst* seq_type_a = necro_ast_create_type_app(arena, necro_ast_create_conid(arena, intern, "Seq", NECRO_CON_TYPE_VAR), necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR));
        NecroAst* seq_type_b = necro_ast_create_type_app(arena, necro_ast_create_conid(arena, intern, "Seq", NECRO_CON_TYPE_VAR), necro_ast_create_var(arena, intern, "b", NECRO_VAR_TYPE_FREE_VAR));
        necro_append_top(arena, top, necro_ast_create_fn_type_sig(arena, intern, "@>", NULL, necro_ast_create_type_fn(arena, seq_type_a, necro_ast_create_type_fn(arena, seq_type_b, seq_type_b)), NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, "@>", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "rightConstSeqOnLeft", NECRO_VAR_VAR), NULL)));
    }

    // >@
    {
        NecroAst* seq_type_a = necro_ast_create_type_app(arena, necro_ast_create_conid(arena, intern, "Seq", NECRO_CON_TYPE_VAR), necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR));
        NecroAst* seq_type_b = necro_ast_create_type_app(arena, necro_ast_create_conid(arena, intern, "Seq", NECRO_CON_TYPE_VAR), necro_ast_create_var(arena, intern, "b", NECRO_VAR_TYPE_FREE_VAR));
        necro_append_top(arena, top, necro_ast_create_fn_type_sig(arena, intern, ">@", NULL, necro_ast_create_type_fn(arena, seq_type_a, necro_ast_create_type_fn(arena, seq_type_b, seq_type_b)), NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, ">@", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "rightConstSeqOnRight", NECRO_VAR_VAR), NULL)));
    }

    // @<@
    {
        NecroAst* seq_type_a = necro_ast_create_type_app(arena, necro_ast_create_conid(arena, intern, "Seq", NECRO_CON_TYPE_VAR), necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR));
        NecroAst* seq_type_b = necro_ast_create_type_app(arena, necro_ast_create_conid(arena, intern, "Seq", NECRO_CON_TYPE_VAR), necro_ast_create_var(arena, intern, "b", NECRO_VAR_TYPE_FREE_VAR));
        necro_append_top(arena, top, necro_ast_create_fn_type_sig(arena, intern, "@<@", NULL, necro_ast_create_type_fn(arena, seq_type_a, necro_ast_create_type_fn(arena, seq_type_b, seq_type_a)), NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, "@<@", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "leftConstSeqOnBoth", NECRO_VAR_VAR), NULL)));
    }

    // @>@
    {
        NecroAst* seq_type_a = necro_ast_create_type_app(arena, necro_ast_create_conid(arena, intern, "Seq", NECRO_CON_TYPE_VAR), necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR));
        NecroAst* seq_type_b = necro_ast_create_type_app(arena, necro_ast_create_conid(arena, intern, "Seq", NECRO_CON_TYPE_VAR), necro_ast_create_var(arena, intern, "b", NECRO_VAR_TYPE_FREE_VAR));
        necro_append_top(arena, top, necro_ast_create_fn_type_sig(arena, intern, "@>@", NULL, necro_ast_create_type_fn(arena, seq_type_a, necro_ast_create_type_fn(arena, seq_type_b, seq_type_b)), NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, "@>@", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "rightConstSeqOnBoth", NECRO_VAR_VAR), NULL)));
    }

    // // >>=
    // {
    //     NecroAst* a_var       = necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR);
    //     NecroAst* b_var       = necro_ast_create_var(arena, intern, "b", NECRO_VAR_TYPE_FREE_VAR);
    //     NecroAst* m_var       = necro_ast_create_var(arena, intern, "m", NECRO_VAR_TYPE_FREE_VAR);
    //     NecroAst* bind_op_sig =
    //         necro_ast_create_fn_type_sig(arena, intern, ">>=", necro_ast_create_context(arena, intern, "Monad", "m", NULL),
    //             necro_ast_create_type_fn(arena,
    //                 necro_ast_create_type_app(arena, m_var, a_var),
    //                 necro_ast_create_type_fn(arena,
    //                     necro_ast_create_type_fn(arena, a_var, necro_ast_create_type_app(arena, m_var, b_var)),
    //                     necro_ast_create_type_app(arena, m_var, b_var))),
    //             NECRO_VAR_SIG, NECRO_SIG_DECLARATION);
    //     necro_append_top(arena, top, bind_op_sig);
    //     necro_append_top(arena, top, necro_ast_create_simple_assignment(arena, intern, ">>=", necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "bind", NECRO_VAR_VAR), NULL)));
    // }

    // >> , no longer monadic bind operator, co-opted by forwards compose operator
    // {
    //     NecroAst* a_var       = necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR);
    //     NecroAst* b_var       = necro_ast_create_var(arena, intern, "b", NECRO_VAR_TYPE_FREE_VAR);
    //     NecroAst* m_var       = necro_ast_create_var(arena, intern, "m", NECRO_VAR_TYPE_FREE_VAR);
    //     NecroAst* op_sig =
    //         necro_ast_create_fn_type_sig(arena, intern, ">>", necro_ast_create_context(arena, intern, "Monad", "m", NULL),
    //             necro_ast_create_type_fn(arena,
    //                 necro_ast_create_type_app(arena, m_var, a_var),
    //                 necro_ast_create_type_fn(arena,
    //                     necro_ast_create_type_app(arena, m_var, b_var),
    //                     necro_ast_create_type_app(arena, m_var, b_var))),
    //             NECRO_VAR_SIG, NECRO_SIG_DECLARATION);
    //     NecroAst* mm_var      = necro_ast_create_var(arena, intern, "m", NECRO_VAR_DECLARATION);
    //     NecroAst* mk_var      = necro_ast_create_var(arena, intern, "k", NECRO_VAR_DECLARATION);
    //     NecroAst* op_args     = necro_ast_create_apats(arena, mm_var, necro_ast_create_apats(arena, mk_var, NULL));
    //     NecroAst* bind        = necro_ast_create_fexpr(arena,
    //         necro_ast_create_fexpr(arena, necro_ast_create_var(arena, intern, "bind", NECRO_VAR_VAR), necro_ast_create_var(arena, intern, "m", NECRO_VAR_VAR)),
    //         necro_ast_create_lambda(arena, necro_ast_create_apats(arena, necro_ast_create_wildcard(arena), NULL), necro_ast_create_var(arena, intern, "k", NECRO_VAR_VAR)));
    //     NecroAst* op_rhs      = necro_ast_create_rhs(arena, bind, NULL);
    //     NecroAst* op_def_ast  = necro_ast_create_apats_assignment(arena, intern, ">>", op_args, op_rhs);
    //     necro_append_top(arena, top, op_sig);
    //     necro_append_top(arena, top, op_def_ast);
    // }

    // <| :: .(.a -> .b) -> .(.a -> .b)
    {
        NecroAst* a_var       = necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR);
        NecroAst* b_var       = necro_ast_create_var(arena, intern, "b", NECRO_VAR_TYPE_FREE_VAR);
        a_var->variable.order = NECRO_TYPE_HIGHER_ORDER;
        b_var->variable.order = NECRO_TYPE_HIGHER_ORDER;
        NecroAst* op_sig      =
            necro_ast_create_fn_type_sig(arena, intern, "<|", NULL,
                necro_ast_create_type_fn(arena,
                    necro_ast_create_type_attribute(arena, necro_ast_create_type_fn(arena, necro_ast_create_type_attribute(arena, a_var, NECRO_TYPE_ATTRIBUTE_DOT), necro_ast_create_type_attribute(arena, b_var, NECRO_TYPE_ATTRIBUTE_DOT)), NECRO_TYPE_ATTRIBUTE_DOT),
                    necro_ast_create_type_attribute(arena, necro_ast_create_type_fn(arena, necro_ast_create_type_attribute(arena, a_var, NECRO_TYPE_ATTRIBUTE_DOT), necro_ast_create_type_attribute(arena, b_var, NECRO_TYPE_ATTRIBUTE_DOT)), NECRO_TYPE_ATTRIBUTE_DOT)),
                NECRO_VAR_SIG, NECRO_SIG_DECLARATION);
        NecroAst* f_var       = necro_ast_create_var(arena, intern, "f", NECRO_VAR_DECLARATION);
        NecroAst* x_var       = necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION);
        NecroAst* op_args     = necro_ast_create_apats(arena, f_var, necro_ast_create_apats(arena, x_var, NULL));
        NecroAst* op_rhs_ast  = necro_ast_create_rhs(arena, necro_ast_create_fexpr(arena, necro_ast_create_var(arena, intern, "f", NECRO_VAR_VAR), necro_ast_create_var(arena, intern, "x", NECRO_VAR_VAR)), NULL);
        NecroAst* op_def_ast  = necro_ast_create_apats_assignment(arena, intern, "<|", op_args, op_rhs_ast);
        necro_append_top(arena, top, op_sig);
        necro_append_top(arena, top, op_def_ast);
    }

    // |>
    {
        NecroAst* a_var       = necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR);
        NecroAst* b_var       = necro_ast_create_var(arena, intern, "b", NECRO_VAR_TYPE_FREE_VAR);
        a_var->variable.order = NECRO_TYPE_HIGHER_ORDER;
        b_var->variable.order = NECRO_TYPE_HIGHER_ORDER;
        NecroAst* op_sig      =
            necro_ast_create_fn_type_sig(arena, intern, "|>", NULL,
                necro_ast_create_type_fn(arena,
                    necro_ast_create_type_attribute(arena, a_var, NECRO_TYPE_ATTRIBUTE_DOT),
                        necro_ast_create_type_fn(arena,
                            necro_ast_create_type_attribute(arena, necro_ast_create_type_fn(arena,
                                necro_ast_create_type_attribute(arena, a_var, NECRO_TYPE_ATTRIBUTE_DOT),
                                necro_ast_create_type_attribute(arena, b_var, NECRO_TYPE_ATTRIBUTE_DOT)),
                                NECRO_TYPE_ATTRIBUTE_DOT),
                            necro_ast_create_type_attribute(arena, b_var, NECRO_TYPE_ATTRIBUTE_DOT))),
                NECRO_VAR_SIG, NECRO_SIG_DECLARATION);
        NecroAst* x_var       = necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION);
        NecroAst* f_var       = necro_ast_create_var(arena, intern, "f", NECRO_VAR_DECLARATION);
        NecroAst* op_args     = necro_ast_create_apats(arena, x_var, necro_ast_create_apats(arena, f_var, NULL));
        NecroAst* op_rhs_ast  = necro_ast_create_rhs(arena, necro_ast_create_fexpr(arena, necro_ast_create_var(arena, intern, "f", NECRO_VAR_VAR), necro_ast_create_var(arena, intern, "x", NECRO_VAR_VAR)), NULL);
        NecroAst* op_def_ast  = necro_ast_create_apats_assignment(arena, intern, "|>", op_args, op_rhs_ast);
        necro_append_top(arena, top, op_sig);
        necro_append_top(arena, top, op_def_ast);
    }

    // <. :: (b -> c) -> (a -> b) -> a -> c
    {
        NecroAst* a_var       = necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR);
        NecroAst* b_var       = necro_ast_create_var(arena, intern, "b", NECRO_VAR_TYPE_FREE_VAR);
        NecroAst* c_var       = necro_ast_create_var(arena, intern, "c", NECRO_VAR_TYPE_FREE_VAR);
        a_var->variable.order = NECRO_TYPE_HIGHER_ORDER;
        b_var->variable.order = NECRO_TYPE_HIGHER_ORDER;
        c_var->variable.order = NECRO_TYPE_HIGHER_ORDER;
        NecroAst* op_sig      =
            necro_ast_create_fn_type_sig(arena, intern, "<.", NULL,
                necro_ast_create_type_fn(arena, necro_ast_create_type_fn(arena, b_var, c_var),
                    necro_ast_create_type_fn(arena, necro_ast_create_type_fn(arena, a_var, b_var),
                        necro_ast_create_type_fn(arena, a_var, c_var))),
                NECRO_VAR_SIG, NECRO_SIG_DECLARATION);
        NecroAst* f_var       = necro_ast_create_var(arena, intern, "f", NECRO_VAR_DECLARATION);
        NecroAst* g_var       = necro_ast_create_var(arena, intern, "g", NECRO_VAR_DECLARATION);
        NecroAst* x_var       = necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION);
        NecroAst* op_args     = necro_ast_create_apats(arena, f_var, necro_ast_create_apats(arena, g_var, NULL));
        NecroAst* op_lambda   = necro_ast_create_lambda(arena, necro_ast_create_apats(arena, x_var, NULL),
            necro_ast_create_fexpr(arena, necro_ast_create_var(arena, intern, "f", NECRO_VAR_VAR),
                necro_ast_create_fexpr(arena, necro_ast_create_var(arena, intern, "g", NECRO_VAR_VAR),
                    necro_ast_create_var(arena, intern, "x", NECRO_VAR_VAR))));
        NecroAst* op_rhs_ast  = necro_ast_create_rhs(arena, op_lambda, NULL);
        NecroAst* op_def_ast  = necro_ast_create_apats_assignment(arena, intern, "<.", op_args, op_rhs_ast);
        necro_append_top(arena, top, op_sig);
        necro_append_top(arena, top, op_def_ast);
    }

    // .> :: (a -> b) -> (b -> c) -> a -> c
    {
        NecroAst* a_var       = necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR);
        NecroAst* b_var       = necro_ast_create_var(arena, intern, "b", NECRO_VAR_TYPE_FREE_VAR);
        NecroAst* c_var       = necro_ast_create_var(arena, intern, "c", NECRO_VAR_TYPE_FREE_VAR);
        a_var->variable.order = NECRO_TYPE_HIGHER_ORDER;
        b_var->variable.order = NECRO_TYPE_HIGHER_ORDER;
        c_var->variable.order = NECRO_TYPE_HIGHER_ORDER;
        NecroAst* op_sig      =
            necro_ast_create_fn_type_sig(arena, intern, ".>", NULL,
                necro_ast_create_type_fn(arena, necro_ast_create_type_fn(arena, a_var, b_var),
                    necro_ast_create_type_fn(arena, necro_ast_create_type_fn(arena, b_var, c_var),
                        necro_ast_create_type_fn(arena, a_var, c_var))),
                NECRO_VAR_SIG, NECRO_SIG_DECLARATION);
        NecroAst* f_var       = necro_ast_create_var(arena, intern, "f", NECRO_VAR_DECLARATION);
        NecroAst* g_var       = necro_ast_create_var(arena, intern, "g", NECRO_VAR_DECLARATION);
        NecroAst* x_var       = necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION);
        NecroAst* op_args     = necro_ast_create_apats(arena, f_var, necro_ast_create_apats(arena, g_var, NULL));
        NecroAst* op_lambda   = necro_ast_create_lambda(arena, necro_ast_create_apats(arena, x_var, NULL),
            necro_ast_create_fexpr(arena, necro_ast_create_var(arena, intern, "g", NECRO_VAR_VAR),
                necro_ast_create_fexpr(arena, necro_ast_create_var(arena, intern, "f", NECRO_VAR_VAR),
                    necro_ast_create_var(arena, intern, "x", NECRO_VAR_VAR))));
        NecroAst* op_rhs_ast  = necro_ast_create_rhs(arena, op_lambda, NULL);
        NecroAst* op_def_ast  = necro_ast_create_apats_assignment(arena, intern, ".>", op_args, op_rhs_ast);
        necro_append_top(arena, top, op_sig);
        necro_append_top(arena, top, op_def_ast);
    }

    // TODO: Look into this. Looks like somewhere along the way we broke polymorphic pat assignments.
    // // prev
    // {
    //     NecroAst* a_var       = necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR);
    //     a_var->variable.order = NECRO_TYPE_ZERO_ORDER;
    //     NecroAst* context     = necro_ast_create_context(arena, intern, "Default", "a", NULL);
    //     NecroAst* fn_sig      =
    //         necro_ast_create_fn_type_sig(arena, intern, "prev", context,
    //             necro_ast_create_type_fn(arena, a_var, a_var),
    //             NECRO_VAR_SIG, NECRO_SIG_DECLARATION);
    //     NecroAst* x_var       = necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION);
    //     NecroAst* fn_args     = necro_ast_create_apats(arena, x_var, NULL);
    //     NecroAst* px_var      = necro_ast_create_var(arena, intern, "px", NECRO_VAR_DECLARATION);
    //     NecroAst* cx_var      = necro_ast_create_var(arena, intern, "cx", NECRO_VAR_DECLARATION);
    //     px_var->variable.initializer = necro_ast_create_var(arena, intern, "default", NECRO_VAR_VAR);
    //     NecroAst* pat_assign  = necro_ast_create_pat_assignment(arena,
    //         necro_ast_create_tuple(arena, necro_ast_create_list(arena, px_var, necro_ast_create_list(arena, cx_var, NULL))),
    //         necro_ast_create_tuple(arena, necro_ast_create_list(arena, necro_ast_create_var(arena, intern, "x", NECRO_VAR_VAR), necro_ast_create_list(arena, necro_ast_create_var(arena, intern, "px", NECRO_VAR_VAR), NULL))));
    //     NecroAst* fn_decl     = necro_ast_create_decl(arena, pat_assign, NULL);
    //     NecroAst* fn_rhs_ast  = necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "cx", NECRO_VAR_VAR), fn_decl);
    //     NecroAst* fn_def_ast  = necro_ast_create_apats_assignment(arena, intern, "prev", fn_args, fn_rhs_ast);
    //     necro_append_top(arena, top, fn_sig);
    //     necro_append_top(arena, top, fn_def_ast);
    // }

    // getMouseX
    {
        necro_append_top(arena, top, necro_ast_create_fn_type_sig(arena, intern, "getMouseX", NULL,
            necro_ast_create_type_fn(arena,
                necro_ast_create_conid(arena, intern, "()", NECRO_CON_TYPE_VAR),
                necro_ast_create_conid(arena, intern, "Int", NECRO_CON_TYPE_VAR)),
            NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top,
            necro_ast_create_apats_assignment(arena, intern, "getMouseX",
                necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "_dummy", NECRO_VAR_DECLARATION), NULL),
                necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "primUndefined", NECRO_VAR_VAR), NULL)));
    }

    // getMouseY
    {
        necro_append_top(arena, top, necro_ast_create_fn_type_sig(arena, intern, "getMouseY", NULL,
            necro_ast_create_type_fn(arena,
                necro_ast_create_conid(arena, intern, "()", NECRO_CON_TYPE_VAR),
                necro_ast_create_conid(arena, intern, "Int", NECRO_CON_TYPE_VAR)),
            NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top,
            necro_ast_create_apats_assignment(arena, intern, "getMouseY",
                necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "_dummy", NECRO_VAR_DECLARATION), NULL),
                necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "primUndefined", NECRO_VAR_VAR), NULL)));
    }

    // mouseX
    {
        necro_append_top(arena, top, necro_ast_create_fn_type_sig(arena, intern, "mouseX", NULL,
            necro_ast_create_conid(arena, intern, "Int", NECRO_CON_TYPE_VAR),
            NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top,
            necro_ast_create_simple_assignment(arena, intern, "mouseX",
                necro_ast_create_rhs(arena,
                    necro_ast_create_fexpr(arena,
                        necro_ast_create_var(arena, intern, "getMouseX", NECRO_VAR_VAR),
                        necro_ast_create_conid(arena, intern, "()", NECRO_CON_VAR)),
                    NULL)));
    }

    // mouseY
    {
        necro_append_top(arena, top, necro_ast_create_fn_type_sig(arena, intern, "mouseY", NULL,
            necro_ast_create_conid(arena, intern, "Int", NECRO_CON_TYPE_VAR),
            NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top,
            necro_ast_create_simple_assignment(arena, intern, "mouseY",
                necro_ast_create_rhs(arena,
                    necro_ast_create_fexpr(arena,
                        necro_ast_create_var(arena, intern, "getMouseY", NECRO_VAR_VAR),
                        necro_ast_create_conid(arena, intern, "()", NECRO_CON_VAR)),
                    NULL)));
    }

    // sampleRate
    {
        necro_append_top(arena, top, necro_ast_create_fn_type_sig(arena, intern, "sampleRate", NULL,
            necro_ast_create_conid(arena, intern, "UInt", NECRO_CON_TYPE_VAR),
            NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top,
            necro_ast_create_simple_assignment(arena, intern, "sampleRate",
                necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "primUndefined", NECRO_VAR_VAR), NULL)));
    }

    // recipSampleRate
    {
        necro_append_top(arena, top, necro_ast_create_fn_type_sig(arena, intern, "recipSampleRate", NULL,
            necro_ast_create_conid(arena, intern, "F64", NECRO_CON_TYPE_VAR),
            NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top,
            necro_ast_create_simple_assignment(arena, intern, "recipSampleRate",
                necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "primUndefined", NECRO_VAR_VAR), NULL)));
    }

    // unsafeEmptyArray :: () -> *Array n a
    {
        necro_append_top(arena, top,
            necro_ast_create_fn_type_sig(arena, intern, "unsafeEmptyArray", NULL,
                necro_ast_create_type_fn(arena,
                    necro_ast_create_conid(arena, intern, "()", NECRO_CON_TYPE_VAR),
                    necro_ast_create_type_attribute(arena,
                        necro_ast_create_type_app(arena,
                            necro_ast_create_type_app(arena,
                                necro_ast_create_conid(arena, intern, "Array", NECRO_CON_TYPE_VAR),
                                necro_ast_create_var(arena, intern, "n", NECRO_VAR_TYPE_FREE_VAR)),
                            necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR)),
                        NECRO_TYPE_ATTRIBUTE_STAR)),
                NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top,
            necro_ast_create_apats_assignment(arena, intern, "unsafeEmptyArray",
                necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION), NULL),
                necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "primUndefined", NECRO_VAR_VAR), NULL)));
    }

    // freezeArray :: *Array n (Share a) -> Array n a
    {
        necro_append_top(arena, top,
            necro_ast_create_fn_type_sig(arena, intern, "freezeArray", NULL,
                necro_ast_create_type_fn(arena,
                    necro_ast_create_type_attribute(arena,
                        necro_ast_create_type_app(arena,
                            necro_ast_create_type_app(arena,
                                necro_ast_create_conid(arena, intern, "Array", NECRO_CON_TYPE_VAR),
                                necro_ast_create_var(arena, intern, "n", NECRO_VAR_TYPE_FREE_VAR)),
                            necro_ast_create_type_app(arena,
                                necro_ast_create_conid(arena, intern, "Share", NECRO_CON_TYPE_VAR),
                                necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR))),
                        NECRO_TYPE_ATTRIBUTE_STAR),
                    necro_ast_create_type_app(arena,
                        necro_ast_create_type_app(arena,
                            necro_ast_create_conid(arena, intern, "Array", NECRO_CON_TYPE_VAR),
                            necro_ast_create_var(arena, intern, "n", NECRO_VAR_TYPE_FREE_VAR)),
                        necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR))
                    ),
                NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top,
            necro_ast_create_apats_assignment(arena, intern, "freezeArray",
                necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION), NULL),
                necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "primUndefined", NECRO_VAR_VAR), NULL)));
    }

    // readArray :: Index n -> Array n a -> a
    {
        necro_append_top(arena, top,
            necro_ast_create_fn_type_sig(arena, intern, "readArray", NULL,
                necro_ast_create_type_fn(arena,
                    necro_ast_create_type_app(arena,
                        necro_ast_create_conid(arena, intern, "Index", NECRO_CON_TYPE_VAR),
                        necro_ast_create_var(arena, intern, "n", NECRO_VAR_TYPE_FREE_VAR)
                        ),
                    necro_ast_create_type_fn(arena,
                        necro_ast_create_type_app(arena,
                            necro_ast_create_type_app(arena,
                                necro_ast_create_conid(arena, intern, "Array", NECRO_CON_TYPE_VAR),
                                necro_ast_create_var(arena, intern, "n", NECRO_VAR_TYPE_FREE_VAR)),
                            necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR)),
                        necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR))
                    ),
                NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top,
            necro_ast_create_apats_assignment(arena, intern, "readArray",
                necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "i", NECRO_VAR_DECLARATION), necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "a", NECRO_VAR_DECLARATION), NULL)),
                necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "primUndefined", NECRO_VAR_VAR), NULL)));
    }

    // writeArray :: Index n -> *a -> *Array n a -> *Array n a
    {
        necro_append_top(arena, top,
            necro_ast_create_fn_type_sig(arena, intern, "writeArray", NULL,
                necro_ast_create_type_fn(arena,
                    necro_ast_create_type_app(arena,
                        necro_ast_create_conid(arena, intern, "Index", NECRO_CON_TYPE_VAR),
                        necro_ast_create_var(arena, intern, "n", NECRO_VAR_TYPE_FREE_VAR)
                        ),

                    necro_ast_create_type_fn(arena,
                        necro_ast_create_type_attribute(arena, necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR), NECRO_TYPE_ATTRIBUTE_STAR),

                        necro_ast_create_type_fn(arena,
                            necro_ast_create_type_attribute(arena, necro_ast_create_type_app(arena,
                                necro_ast_create_type_app(arena,
                                    necro_ast_create_conid(arena, intern, "Array", NECRO_CON_TYPE_VAR),
                                    necro_ast_create_var(arena, intern, "n", NECRO_VAR_TYPE_FREE_VAR)),
                                necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR)), NECRO_TYPE_ATTRIBUTE_STAR),

                            necro_ast_create_type_attribute(arena, necro_ast_create_type_app(arena,
                                necro_ast_create_type_app(arena,
                                    necro_ast_create_conid(arena, intern, "Array", NECRO_CON_TYPE_VAR),
                                    necro_ast_create_var(arena, intern, "n", NECRO_VAR_TYPE_FREE_VAR)),
                                necro_ast_create_var(arena, intern, "a", NECRO_VAR_TYPE_FREE_VAR)), NECRO_TYPE_ATTRIBUTE_STAR))
                    )
                ),
                NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top,
            necro_ast_create_apats_assignment(arena, intern, "writeArray",
                necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "i", NECRO_VAR_DECLARATION), necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION), necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "a", NECRO_VAR_DECLARATION), NULL))),
                necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "primUndefined", NECRO_VAR_VAR), NULL)));
    }

    // outAudioBlock :: UInt -> Array BlockSize F64 -> *World -> *World
    {
        necro_append_top(arena, top,
            necro_ast_create_fn_type_sig(arena, intern, "outAudioBlock", NULL,
                necro_ast_create_type_fn(arena,
                    necro_ast_create_conid(arena, intern, "UInt", NECRO_CON_TYPE_VAR),
                    necro_ast_create_type_fn(arena,
                        necro_ast_create_type_app(arena,
                            necro_ast_create_type_app(arena,
                                necro_ast_create_conid(arena, intern, "Array", NECRO_CON_TYPE_VAR),
                                necro_ast_create_conid(arena, intern, "BlockSize", NECRO_CON_TYPE_VAR)),
                            necro_ast_create_conid(arena, intern, "F64", NECRO_CON_TYPE_VAR)),
                        necro_ast_create_type_fn(arena,
                            necro_ast_create_type_attribute(arena, necro_ast_create_conid(arena, intern, "World", NECRO_CON_TYPE_VAR), NECRO_TYPE_ATTRIBUTE_STAR),
                            necro_ast_create_type_attribute(arena, necro_ast_create_conid(arena, intern, "World", NECRO_CON_TYPE_VAR), NECRO_TYPE_ATTRIBUTE_STAR)))),
                NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top,
            necro_ast_create_apats_assignment(arena, intern, "outAudioBlock",
                necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "c", NECRO_VAR_DECLARATION), necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION), necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "w", NECRO_VAR_DECLARATION), NULL))),
                necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "w", NECRO_VAR_VAR), NULL)));
    }

    // printInt
    {
        necro_append_top(arena, top,
            necro_ast_create_fn_type_sig(arena, intern, "printInt", NULL,
                necro_ast_create_type_fn(arena,
                    necro_ast_create_conid(arena, intern, "Int", NECRO_CON_TYPE_VAR),
                    necro_ast_create_type_fn(arena,
                        necro_ast_create_type_attribute(arena, necro_ast_create_conid(arena, intern, "World", NECRO_CON_TYPE_VAR), NECRO_TYPE_ATTRIBUTE_STAR),
                        necro_ast_create_type_attribute(arena, necro_ast_create_conid(arena, intern, "World", NECRO_CON_TYPE_VAR), NECRO_TYPE_ATTRIBUTE_STAR))),
                NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top,
            necro_ast_create_apats_assignment(arena, intern, "printInt",
                necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION), necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "w", NECRO_VAR_DECLARATION), NULL)),
                necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "w", NECRO_VAR_VAR), NULL)));
    }

    // printI64
    {
        necro_append_top(arena, top,
            necro_ast_create_fn_type_sig(arena, intern, "printI64", NULL,
                necro_ast_create_type_fn(arena,
                    necro_ast_create_conid(arena, intern, "I64", NECRO_CON_TYPE_VAR),
                    necro_ast_create_type_fn(arena,
                        necro_ast_create_type_attribute(arena, necro_ast_create_conid(arena, intern, "World", NECRO_CON_TYPE_VAR), NECRO_TYPE_ATTRIBUTE_STAR),
                        necro_ast_create_type_attribute(arena, necro_ast_create_conid(arena, intern, "World", NECRO_CON_TYPE_VAR), NECRO_TYPE_ATTRIBUTE_STAR))),
                NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top,
            necro_ast_create_apats_assignment(arena, intern, "printI64",
                necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION), necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "w", NECRO_VAR_DECLARATION), NULL)),
                necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "w", NECRO_VAR_VAR), NULL)));
    }

    // printF64
    {
        necro_append_top(arena, top,
            necro_ast_create_fn_type_sig(arena, intern, "printF64", NULL,
                necro_ast_create_type_fn(arena,
                    necro_ast_create_conid(arena, intern, "F64", NECRO_CON_TYPE_VAR),
                    necro_ast_create_type_fn(arena,
                        necro_ast_create_type_attribute(arena, necro_ast_create_conid(arena, intern, "World", NECRO_CON_TYPE_VAR), NECRO_TYPE_ATTRIBUTE_STAR),
                        necro_ast_create_type_attribute(arena, necro_ast_create_conid(arena, intern, "World", NECRO_CON_TYPE_VAR), NECRO_TYPE_ATTRIBUTE_STAR))),
                NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top,
            necro_ast_create_apats_assignment(arena, intern, "printF64",
                necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION), necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "w", NECRO_VAR_DECLARATION), NULL)),
                necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "w", NECRO_VAR_VAR), NULL)));
    }

    // printChar
    {
        necro_append_top(arena, top,
            necro_ast_create_fn_type_sig(arena, intern, "printChar", NULL,
                necro_ast_create_type_fn(arena,
                    necro_ast_create_conid(arena, intern, "Char", NECRO_CON_TYPE_VAR),
                    necro_ast_create_type_fn(arena,
                        necro_ast_create_type_attribute(arena, necro_ast_create_conid(arena, intern, "World", NECRO_CON_TYPE_VAR), NECRO_TYPE_ATTRIBUTE_STAR),
                        necro_ast_create_type_attribute(arena, necro_ast_create_conid(arena, intern, "World", NECRO_CON_TYPE_VAR), NECRO_TYPE_ATTRIBUTE_STAR))),
                NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top,
            necro_ast_create_apats_assignment(arena, intern, "printChar",
                necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION), necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "w", NECRO_VAR_DECLARATION), NULL)),
                necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "w", NECRO_VAR_VAR), NULL)));
    }

    // fastFloor
    {
        necro_append_top(arena, top,
            necro_ast_create_fn_type_sig(arena, intern, "fastFloor", NULL,
                necro_ast_create_type_fn(arena,
                    necro_ast_create_conid(arena, intern, "F64", NECRO_CON_TYPE_VAR),
                    necro_ast_create_conid(arena, intern, "F64", NECRO_CON_TYPE_VAR)),
                NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top,
            necro_ast_create_apats_assignment(arena, intern, "fastFloor",
                necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION), NULL),
                necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "primUndefined", NECRO_VAR_VAR), NULL)));
    }

    // floor
    {
        necro_append_top(arena, top,
            necro_ast_create_fn_type_sig(arena, intern, "floor", NULL,
                necro_ast_create_type_fn(arena,
                    necro_ast_create_conid(arena, intern, "F64", NECRO_CON_TYPE_VAR),
                    necro_ast_create_conid(arena, intern, "F64", NECRO_CON_TYPE_VAR)),
                NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top,
            necro_ast_create_apats_assignment(arena, intern, "floor",
                necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION), NULL),
                necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "primUndefined", NECRO_VAR_VAR), NULL)));
    }

    // fma
    {
        necro_append_top(arena, top,
            necro_ast_create_fn_type_sig(arena, intern, "fma", NULL,
                necro_ast_create_type_fn(arena,
                    necro_ast_create_conid(arena, intern, "F64", NECRO_CON_TYPE_VAR),
                    necro_ast_create_type_fn(arena,
                        necro_ast_create_conid(arena, intern, "F64", NECRO_CON_TYPE_VAR),
                        necro_ast_create_type_fn(arena,
                            necro_ast_create_conid(arena, intern, "F64", NECRO_CON_TYPE_VAR),
                            necro_ast_create_conid(arena, intern, "F64", NECRO_CON_TYPE_VAR)))),
                NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top,
            necro_ast_create_apats_assignment(arena, intern, "fma",
                necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION),
                    necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "y", NECRO_VAR_DECLARATION),
                        necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "z", NECRO_VAR_DECLARATION), NULL))),
                necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "primUndefined", NECRO_VAR_VAR), NULL)));
    }

    // && / ||
    // NOTE: && and || turn into NecroMach binops 'and' and 'or', thus use primUndefined.
    {
        NecroAst* bool_conid = necro_ast_create_conid(arena, intern, "Bool", NECRO_CON_TYPE_VAR);
        necro_append_top(arena, top,
            necro_ast_create_fn_type_sig(arena, intern, "&&", NULL,
                necro_ast_create_type_fn(arena,
                    bool_conid,
                    necro_ast_create_type_fn(arena,
                        bool_conid,
                        bool_conid)), NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        NecroAst* and_apats = necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION), necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "y", NECRO_VAR_DECLARATION), NULL));
        necro_append_top(arena, top, necro_ast_create_apats_assignment(arena, intern, "&&", and_apats, necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "primUndefined", NECRO_VAR_VAR), NULL)));
        necro_append_top(arena, top,
            necro_ast_create_fn_type_sig(arena, intern, "||", NULL,
                necro_ast_create_type_fn(arena,
                    bool_conid,
                    necro_ast_create_type_fn(arena,
                        bool_conid,
                        bool_conid)), NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        NecroAst* or_apats = necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION), necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "y", NECRO_VAR_DECLARATION), NULL));
        necro_append_top(arena, top, necro_ast_create_apats_assignment(arena, intern, "||", or_apats, necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "primUndefined", NECRO_VAR_VAR), NULL)));
    }

    // %
    {
        necro_append_top(arena, top,
            necro_ast_create_fn_type_sig(arena, intern, "%", NULL,
                necro_ast_create_type_fn(arena,
                    necro_ast_create_conid(arena, intern, "Int", NECRO_CON_TYPE_VAR),
                    necro_ast_create_type_fn(arena,
                        necro_ast_create_conid(arena, intern, "Int", NECRO_CON_TYPE_VAR),
                        necro_ast_create_conid(arena, intern, "Rational", NECRO_CON_TYPE_VAR))),
                NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        NecroAst* ratio_apats = necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION), necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "y", NECRO_VAR_DECLARATION), NULL));
        NecroAst* ratio_rhs   =
            necro_ast_create_fexpr(arena,
                necro_ast_create_fexpr(arena,
                    necro_ast_create_var(arena, intern, "rational", NECRO_VAR_VAR),
                    necro_ast_create_var(arena, intern, "x", NECRO_VAR_VAR)),
                necro_ast_create_var(arena, intern, "y", NECRO_VAR_VAR));
        necro_append_top(arena, top, necro_ast_create_apats_assignment(arena, intern, "%", ratio_apats, necro_ast_create_rhs(arena, ratio_rhs, NULL)));
    }

    // quotInt
    {
        necro_append_top(arena, top,
            necro_ast_create_fn_type_sig(arena, intern, "quotInt", NULL,
                necro_ast_create_type_fn(arena,
                    necro_ast_create_conid(arena, intern, "Int", NECRO_CON_TYPE_VAR),
                    necro_ast_create_type_fn(arena,
                        necro_ast_create_conid(arena, intern, "Int", NECRO_CON_TYPE_VAR),
                        necro_ast_create_conid(arena, intern, "Int", NECRO_CON_TYPE_VAR))),
                NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top,
            necro_ast_create_apats_assignment(arena, intern, "quotInt",
                necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION), necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "y", NECRO_VAR_DECLARATION), NULL)),
                necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "primUndefined", NECRO_VAR_VAR), NULL)));
    }

    // remInt
    {
        necro_append_top(arena, top,
            necro_ast_create_fn_type_sig(arena, intern, "remInt", NULL,
                necro_ast_create_type_fn(arena,
                    necro_ast_create_conid(arena, intern, "Int", NECRO_CON_TYPE_VAR),
                    necro_ast_create_type_fn(arena,
                        necro_ast_create_conid(arena, intern, "Int", NECRO_CON_TYPE_VAR),
                        necro_ast_create_conid(arena, intern, "Int", NECRO_CON_TYPE_VAR))),
                NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top,
            necro_ast_create_apats_assignment(arena, intern, "remInt",
                necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION), necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "y", NECRO_VAR_DECLARATION), NULL)),
                necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "primUndefined", NECRO_VAR_VAR), NULL)));
    }

    // quotI64
    {
        necro_append_top(arena, top,
            necro_ast_create_fn_type_sig(arena, intern, "quotI64", NULL,
                necro_ast_create_type_fn(arena,
                    necro_ast_create_conid(arena, intern, "I64", NECRO_CON_TYPE_VAR),
                    necro_ast_create_type_fn(arena,
                        necro_ast_create_conid(arena, intern, "I64", NECRO_CON_TYPE_VAR),
                        necro_ast_create_conid(arena, intern, "I64", NECRO_CON_TYPE_VAR))),
                NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top,
            necro_ast_create_apats_assignment(arena, intern, "quotI64",
                necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION), necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "y", NECRO_VAR_DECLARATION), NULL)),
                necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "primUndefined", NECRO_VAR_VAR), NULL)));
    }

    // remI64
    {
        necro_append_top(arena, top,
            necro_ast_create_fn_type_sig(arena, intern, "remI64", NULL,
                necro_ast_create_type_fn(arena,
                    necro_ast_create_conid(arena, intern, "I64", NECRO_CON_TYPE_VAR),
                    necro_ast_create_type_fn(arena,
                        necro_ast_create_conid(arena, intern, "I64", NECRO_CON_TYPE_VAR),
                        necro_ast_create_conid(arena, intern, "I64", NECRO_CON_TYPE_VAR))),
                NECRO_VAR_SIG, NECRO_SIG_DECLARATION));
        necro_append_top(arena, top,
            necro_ast_create_apats_assignment(arena, intern, "remI64",
                necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "x", NECRO_VAR_DECLARATION), necro_ast_create_apats(arena, necro_ast_create_var(arena, intern, "y", NECRO_VAR_DECLARATION), NULL)),
                necro_ast_create_rhs(arena, necro_ast_create_var(arena, intern, "primUndefined", NECRO_VAR_VAR), NULL)));
    }

    //--------------------
    // Compile, part I
    necro_append_top_decl(top, file_top); // Append contents of base.necro
    necro_build_scopes(info, scoped_symtable, &base.ast);
    unwrap(void, necro_rename(info, scoped_symtable, intern, &base.ast));


    //--------------------
    // Cache useful symbols
    base.prim_undefined         = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "primUndefined"));;

    base.tuple2_con             = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "(,)"));
    base.tuple3_con             = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "(,,)"));
    base.tuple4_con             = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "(,,,)"));
    base.tuple5_con             = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "(,,,,)"));
    base.tuple6_con             = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "(,,,,,)"));
    base.tuple7_con             = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "(,,,,,,)"));
    base.tuple8_con             = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "(,,,,,,,)"));
    base.tuple9_con             = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "(,,,,,,,,)"));
    base.tuple10_con            = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "(,,,,,,,,,)"));

    base.tuple2_type            = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "(,)"));
    base.tuple3_type            = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "(,,)"));
    base.tuple4_type            = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "(,,,)"));
    base.tuple5_type            = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "(,,,,)"));
    base.tuple6_type            = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "(,,,,,)"));
    base.tuple7_type            = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "(,,,,,,)"));
    base.tuple8_type            = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "(,,,,,,,)"));
    base.tuple9_type            = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "(,,,,,,,,)"));
    base.tuple10_type           = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "(,,,,,,,,,)"));

    for (size_t i = 2; i <= NECRO_MAX_UNBOXED_TUPLE_TYPES; ++i)
    {
        char name[16] = "";
        snprintf(name, 16, "(#,#)%zu", i);
        base.unboxed_tuple_types[i]              = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, name));
        base.unboxed_tuple_cons[i]               = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, name));
        base.unboxed_tuple_types[i]->is_unboxed  = true;
        base.unboxed_tuple_types[i]->primop_type = NECRO_PRIMOP_UNBOXED_CON;
        base.unboxed_tuple_cons[i]->is_unboxed   = true;
        base.unboxed_tuple_cons[i]->primop_type  = NECRO_PRIMOP_UNBOXED_CON;
    }

    for (size_t i = 0; i < NECRO_MAX_ENV_TYPES; ++i)
    {
        char env_name[16] = "";
        snprintf(env_name, 16, "Env%zu", i);
        base.env_types[i] = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, env_name));
        base.env_cons[i]  = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, env_name));
    }

    // for (size_t i = 2; i < NECRO_MAX_BRANCH_TYPES; ++i)
    // {
    //     char branch_name[24] = "";
    //     snprintf(branch_name, 24, "BranchFn%zu", i);
    //     base.branch_types[i] = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, branch_name));
    //     base.branch_cons[i]  = necro_paged_arena_alloc(arena, i * sizeof(NecroAstSymbol*));
    //     for (size_t i2 = 0; i2 < i; ++i2)
    //     {
    //         snprintf(branch_name, 24, "BranchFn%zuAlt%zu", i, i2);
    //         base.branch_cons[i][i2] = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, branch_name));
    //     }
    // }

    //--------------------
    // Cache symbols
    base.world_type             = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "World"));
    base.unit_type              = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "()"));
    base.unit_con               = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "()"));
    base.seq_type               = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "Seq"));
    base.seq_con                = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "Seq"));
    base.seq_value_type         = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "SeqValue"));
    base.share_type             = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "Share"));
    // base.list_type              = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "[]"));
    base.int_type               = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "Int"));
    base.uint_type              = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "UInt"));
    base.float_type             = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "Float"));
    base.int64_type             = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "I64"));
    base.float64_type           = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "F64"));
    base.char_type              = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "Char"));
    base.bool_type              = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "Bool"));
    base.true_con               = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "True"));
    base.false_con              = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "False"));
    base.num_type_class         = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "Num"));
    base.fractional_type_class  = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "Fractional"));
    base.eq_type_class          = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "Eq"));
    base.ord_type_class         = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "Ord"));
    base.functor_type_class     = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "Functor"));
    base.applicative_type_class = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "Applicative"));
    base.monad_type_class       = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "Monad"));
    base.default_type_class     = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "Default"));
    base.audio_type_class       = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "Audio"));
    base.mono_type              = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "Mono"));
    base.prev_fn                = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "prev"));
    // base.event_type             = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "Event"));
    // base.pattern_type           = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "Pattern"));
    base.ptr_type               = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "Ptr"));
    // base.ptr_malloc             = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "ptrMalloc"));
    base.array_type             = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "Array"));
    base.range_type             = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "Range"));
    base.range_con              = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "Range"));
    base.index_type             = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "Index"));
    // base.maybe_type             = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "Maybe"));;
    base.block_size_type        = necro_symtable_get_type_ast_symbol(scoped_symtable, necro_intern_string(intern, "BlockSize"));

    base.pipe_forward           = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "|>"));;
    base.pipe_back              = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "<|"));;
    base.compose_forward        = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, ".>"));;
    base.compose_back           = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "<."));;
    base.from_int               = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "fromInt"));;
    base.run_seq                = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "runSeq"));;
    base.seq_tick               = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "seqTick"));;
    base.tuple_tick             = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "tupleTick"));;
    base.interleave_tick        = necro_symtable_get_top_level_ast_symbol(scoped_symtable, necro_intern_string(intern, "interleaveTick"));;

    //--------------------
    // Primitives
    base.prim_undefined->is_primitive = true;
    base.unit_type->is_primitive      = true;
    base.unit_con->is_primitive       = true;
    base.int_type->is_primitive       = true;
    base.int64_type->is_primitive     = true;
    base.uint_type->is_primitive      = true;
    base.float_type->is_primitive     = true;
    base.float64_type->is_primitive   = true;
    base.char_type->is_primitive      = true;
    base.bool_type->is_primitive      = true;
    base.true_con->is_primitive       = true;
    base.false_con->is_primitive      = true;
    base.array_type->is_primitive     = true;
    base.index_type->is_primitive     = true;
    base.ptr_type->is_primitive       = true;

    // Runtime Functions/Values
    necro_base_setup_primitive(scoped_symtable, intern, "getMouseX",        &base.mouse_x_fn,        NECRO_PRIMOP_PRIM_FN);
    necro_base_setup_primitive(scoped_symtable, intern, "getMouseY",        &base.mouse_y_fn,        NECRO_PRIMOP_PRIM_FN);
    necro_base_setup_primitive(scoped_symtable, intern, "sampleRate",       &base.sample_rate,       NECRO_PRIMOP_PRIM_VAL);
    necro_base_setup_primitive(scoped_symtable, intern, "recipSampleRate",  &base.recip_sample_rate, NECRO_PRIMOP_PRIM_VAL);
    necro_base_setup_primitive(scoped_symtable, intern, "printInt",         &base.print_int,         NECRO_PRIMOP_PRIM_FN);
    necro_base_setup_primitive(scoped_symtable, intern, "printI64",         &base.print_i64,         NECRO_PRIMOP_PRIM_FN);
    necro_base_setup_primitive(scoped_symtable, intern, "printF64",         &base.print_f64,         NECRO_PRIMOP_PRIM_FN);
    necro_base_setup_primitive(scoped_symtable, intern, "printChar",        &base.print_char,        NECRO_PRIMOP_PRIM_FN);
    necro_base_setup_primitive(scoped_symtable, intern, "outAudioBlock",    &base.out_audio_block,   NECRO_PRIMOP_PRIM_FN);
    necro_base_setup_primitive(scoped_symtable, intern, "floor",            &base.floor,             NECRO_PRIMOP_PRIM_FN);

    // Misc
    necro_base_setup_primitive(scoped_symtable, intern, "_project",         &base.proj_fn,           NECRO_PRIMOP_PROJ);

    // Intrinsics
    necro_base_setup_primitive(scoped_symtable, intern, "fma",              &base.fma,               NECRO_PRIMOP_INTR_FMA);

    // Array
    necro_base_setup_primitive(scoped_symtable, intern, "unsafeEmptyArray", &base.unsafe_empty_array, NECRO_PRIMOP_ARRAY_EMPTY);
    necro_base_setup_primitive(scoped_symtable, intern, "readArray",        &base.read_array,         NECRO_PRIMOP_ARRAY_READ);
    necro_base_setup_primitive(scoped_symtable, intern, "writeArray",       &base.write_array,        NECRO_PRIMOP_ARRAY_WRITE);
    necro_base_setup_primitive(scoped_symtable, intern, "freezeArray",      NULL,                     NECRO_PRIMOP_ARRAY_FREEZE);

    // Ptr
    necro_base_setup_primitive(scoped_symtable, intern, "ptrMalloc",            NULL, NECRO_PRIMOP_PTR_ALLOC);
    necro_base_setup_primitive(scoped_symtable, intern, "ptrRealloc",           NULL, NECRO_PRIMOP_PTR_REALLOC);
    necro_base_setup_primitive(scoped_symtable, intern, "ptrFree",              NULL, NECRO_PRIMOP_PTR_FREE);
    necro_base_setup_primitive(scoped_symtable, intern, "unsafePtrPoke",        NULL, NECRO_PRIMOP_PTR_POKE);
    necro_base_setup_primitive(scoped_symtable, intern, "unsafePtrSwapElement", NULL, NECRO_PRIMOP_PTR_SWAP);

    necro_base_setup_primitive(scoped_symtable, intern, "mutRef", NULL, NECRO_PRIMOP_MREF);

    // PolyThunk
    // necro_base_setup_primitive(scoped_symtable, intern, "polyThunkAlloc", NULL, NECRO_PRIMOP_PTHUNK_ALLOC);
    // necro_base_setup_primitive(scoped_symtable, intern, "polyThunkInit",  NULL, NECRO_PRIMOP_PTHUNK_INIT);
    necro_base_setup_primitive(scoped_symtable, intern, "polyThunkEval",  NULL, NECRO_PRIMOP_PTHUNK_EVAL);
    necro_base_setup_primitive(scoped_symtable, intern, "dynDeepCopy",    NULL, NECRO_PRIMOP_DYN_DEEP_COPY);

    // Int
    necro_base_setup_primitive(scoped_symtable, intern, "add<Int>",     NULL, NECRO_PRIMOP_BINOP_IADD);
    necro_base_setup_primitive(scoped_symtable, intern, "sub<Int>",     NULL, NECRO_PRIMOP_BINOP_ISUB);
    necro_base_setup_primitive(scoped_symtable, intern, "mul<Int>",     NULL, NECRO_PRIMOP_BINOP_IMUL);
    necro_base_setup_primitive(scoped_symtable, intern, "abs<Int>",     NULL, NECRO_PRIMOP_UOP_IABS);
    necro_base_setup_primitive(scoped_symtable, intern, "signum<Int>",  NULL, NECRO_PRIMOP_UOP_ISGN);
    necro_base_setup_primitive(scoped_symtable, intern, "fromInt<Int>", NULL, NECRO_PRIMOP_UOP_ITOI);
    necro_base_setup_primitive(scoped_symtable, intern, "quotInt",      NULL, NECRO_PRIMOP_BINOP_IDIV);
    necro_base_setup_primitive(scoped_symtable, intern, "remInt",       NULL, NECRO_PRIMOP_BINOP_IREM);
    necro_base_setup_primitive(scoped_symtable, intern, "eq<Int>",      NULL, NECRO_PRIMOP_CMP_EQ);
    necro_base_setup_primitive(scoped_symtable, intern, "neq<Int>",     NULL, NECRO_PRIMOP_CMP_NE);
    necro_base_setup_primitive(scoped_symtable, intern, "gt<Int>",      NULL, NECRO_PRIMOP_CMP_GT);
    necro_base_setup_primitive(scoped_symtable, intern, "lt<Int>",      NULL, NECRO_PRIMOP_CMP_LT);
    necro_base_setup_primitive(scoped_symtable, intern, "gte<Int>",     NULL, NECRO_PRIMOP_CMP_GE);
    necro_base_setup_primitive(scoped_symtable, intern, "lte<Int>",     NULL, NECRO_PRIMOP_CMP_LE);

    // I64
    necro_base_setup_primitive(scoped_symtable, intern, "add<I64>",     NULL, NECRO_PRIMOP_BINOP_IADD);
    necro_base_setup_primitive(scoped_symtable, intern, "sub<I64>",     NULL, NECRO_PRIMOP_BINOP_ISUB);
    necro_base_setup_primitive(scoped_symtable, intern, "mul<I64>",     NULL, NECRO_PRIMOP_BINOP_IMUL);
    necro_base_setup_primitive(scoped_symtable, intern, "abs<I64>",     NULL, NECRO_PRIMOP_UOP_IABS);
    necro_base_setup_primitive(scoped_symtable, intern, "signum<I64>",  NULL, NECRO_PRIMOP_UOP_ISGN);
    necro_base_setup_primitive(scoped_symtable, intern, "fromInt<I64>", NULL, NECRO_PRIMOP_UOP_ITOI);
    necro_base_setup_primitive(scoped_symtable, intern, "quotI64",      NULL, NECRO_PRIMOP_BINOP_IDIV);
    necro_base_setup_primitive(scoped_symtable, intern, "remI64",       NULL, NECRO_PRIMOP_BINOP_IREM);
    necro_base_setup_primitive(scoped_symtable, intern, "eq<I64>",      NULL, NECRO_PRIMOP_CMP_EQ);
    necro_base_setup_primitive(scoped_symtable, intern, "neq<I64>",     NULL, NECRO_PRIMOP_CMP_NE);
    necro_base_setup_primitive(scoped_symtable, intern, "gt<I64>",      NULL, NECRO_PRIMOP_CMP_GT);
    necro_base_setup_primitive(scoped_symtable, intern, "lt<I64>",      NULL, NECRO_PRIMOP_CMP_LT);
    necro_base_setup_primitive(scoped_symtable, intern, "gte<I64>",     NULL, NECRO_PRIMOP_CMP_GE);
    necro_base_setup_primitive(scoped_symtable, intern, "lte<I64>",     NULL, NECRO_PRIMOP_CMP_LE);

    // UInt
    necro_base_setup_primitive(scoped_symtable, intern, "add<UInt>",     NULL, NECRO_PRIMOP_BINOP_UADD);
    necro_base_setup_primitive(scoped_symtable, intern, "sub<UInt>",     NULL, NECRO_PRIMOP_BINOP_USUB);
    necro_base_setup_primitive(scoped_symtable, intern, "mul<UInt>",     NULL, NECRO_PRIMOP_BINOP_UMUL);
    necro_base_setup_primitive(scoped_symtable, intern, "abs<UInt>",     NULL, NECRO_PRIMOP_UOP_UABS);
    necro_base_setup_primitive(scoped_symtable, intern, "signum<UInt>",  NULL, NECRO_PRIMOP_UOP_USGN);
    necro_base_setup_primitive(scoped_symtable, intern, "fromInt<UInt>", NULL, NECRO_PRIMOP_UOP_ITOU);
    // necro_base_setup_primitive(scoped_symtable, intern, "quotUInt64",    NULL, NECRO_PRIMOP_BINOP_IDIV); // TODO: Finish
    necro_base_setup_primitive(scoped_symtable, intern, "eq<UInt>",      NULL, NECRO_PRIMOP_CMP_EQ);
    necro_base_setup_primitive(scoped_symtable, intern, "neq<UInt>",     NULL, NECRO_PRIMOP_CMP_NE);
    necro_base_setup_primitive(scoped_symtable, intern, "gt<UInt>",      NULL, NECRO_PRIMOP_CMP_GT);
    necro_base_setup_primitive(scoped_symtable, intern, "lt<UInt>",      NULL, NECRO_PRIMOP_CMP_LT);
    necro_base_setup_primitive(scoped_symtable, intern, "gte<UInt>",     NULL, NECRO_PRIMOP_CMP_GE);
    necro_base_setup_primitive(scoped_symtable, intern, "lte<UInt>",     NULL, NECRO_PRIMOP_CMP_LE);

    // Float
    necro_base_setup_primitive(scoped_symtable, intern, "add<Float>",          NULL, NECRO_PRIMOP_BINOP_FADD);
    necro_base_setup_primitive(scoped_symtable, intern, "sub<Float>",          NULL, NECRO_PRIMOP_BINOP_FSUB);
    necro_base_setup_primitive(scoped_symtable, intern, "mul<Float>",          NULL, NECRO_PRIMOP_BINOP_FMUL);
    necro_base_setup_primitive(scoped_symtable, intern, "div<Float>",          NULL, NECRO_PRIMOP_BINOP_FDIV);
    necro_base_setup_primitive(scoped_symtable, intern, "abs<Float>",          NULL, NECRO_PRIMOP_UOP_FABS);
    necro_base_setup_primitive(scoped_symtable, intern, "signum<Float>",       NULL, NECRO_PRIMOP_UOP_FSGN);
    necro_base_setup_primitive(scoped_symtable, intern, "fromInt<Float>",      NULL, NECRO_PRIMOP_UOP_ITOF);
    necro_base_setup_primitive(scoped_symtable, intern, "fromRational<Float>", NULL, NECRO_PRIMOP_UOP_FTOF);
    necro_base_setup_primitive(scoped_symtable, intern, "eq<Float>",           NULL, NECRO_PRIMOP_CMP_EQ);
    necro_base_setup_primitive(scoped_symtable, intern, "neq<Float>",          NULL, NECRO_PRIMOP_CMP_NE);
    necro_base_setup_primitive(scoped_symtable, intern, "gt<Float>",           NULL, NECRO_PRIMOP_CMP_GT);
    necro_base_setup_primitive(scoped_symtable, intern, "lt<Float>",           NULL, NECRO_PRIMOP_CMP_LT);
    necro_base_setup_primitive(scoped_symtable, intern, "gte<Float>",          NULL, NECRO_PRIMOP_CMP_GE);
    necro_base_setup_primitive(scoped_symtable, intern, "lte<Float>",          NULL, NECRO_PRIMOP_CMP_LE);
    necro_base_setup_primitive(scoped_symtable, intern, "fastFloor",           &base.fast_floor, NECRO_PRIMOP_UOP_FFLR);

    // F64
    necro_base_setup_primitive(scoped_symtable, intern, "add<F64>",          NULL, NECRO_PRIMOP_BINOP_FADD);
    necro_base_setup_primitive(scoped_symtable, intern, "sub<F64>",          NULL, NECRO_PRIMOP_BINOP_FSUB);
    necro_base_setup_primitive(scoped_symtable, intern, "mul<F64>",          NULL, NECRO_PRIMOP_BINOP_FMUL);
    necro_base_setup_primitive(scoped_symtable, intern, "div<F64>",          NULL, NECRO_PRIMOP_BINOP_FDIV);
    necro_base_setup_primitive(scoped_symtable, intern, "abs<F64>",          NULL, NECRO_PRIMOP_UOP_FABS);
    necro_base_setup_primitive(scoped_symtable, intern, "signum<F64>",       NULL, NECRO_PRIMOP_UOP_FSGN);
    necro_base_setup_primitive(scoped_symtable, intern, "fromInt<F64>",      NULL, NECRO_PRIMOP_UOP_ITOF);
    necro_base_setup_primitive(scoped_symtable, intern, "fromRational<F64>", NULL, NECRO_PRIMOP_UOP_FTOF);
    necro_base_setup_primitive(scoped_symtable, intern, "eq<F64>",           NULL, NECRO_PRIMOP_CMP_EQ);
    necro_base_setup_primitive(scoped_symtable, intern, "neq<F64>",          NULL, NECRO_PRIMOP_CMP_NE);
    necro_base_setup_primitive(scoped_symtable, intern, "gt<F64>",           NULL, NECRO_PRIMOP_CMP_GT);
    necro_base_setup_primitive(scoped_symtable, intern, "lt<F64>",           NULL, NECRO_PRIMOP_CMP_LT);
    necro_base_setup_primitive(scoped_symtable, intern, "gte<F64>",          NULL, NECRO_PRIMOP_CMP_GE);
    necro_base_setup_primitive(scoped_symtable, intern, "lte<F64>",          NULL, NECRO_PRIMOP_CMP_LE);

    // ()
    necro_base_setup_primitive(scoped_symtable, intern, "eq<()>",  NULL, NECRO_PRIMOP_CMP_EQ);
    necro_base_setup_primitive(scoped_symtable, intern, "neq<()>", NULL, NECRO_PRIMOP_CMP_NE);
    necro_base_setup_primitive(scoped_symtable, intern, "gt<()>",  NULL, NECRO_PRIMOP_CMP_GT);
    necro_base_setup_primitive(scoped_symtable, intern, "lt<()>",  NULL, NECRO_PRIMOP_CMP_LT);
    necro_base_setup_primitive(scoped_symtable, intern, "gte<()>", NULL, NECRO_PRIMOP_CMP_GE);
    necro_base_setup_primitive(scoped_symtable, intern, "lte<()>", NULL, NECRO_PRIMOP_CMP_LE);

    // Bool
    necro_base_setup_primitive(scoped_symtable, intern, "eq<Bool>",  NULL, NECRO_PRIMOP_CMP_EQ);
    necro_base_setup_primitive(scoped_symtable, intern, "neq<Bool>", NULL, NECRO_PRIMOP_CMP_NE);
    necro_base_setup_primitive(scoped_symtable, intern, "gt<Bool>",  NULL, NECRO_PRIMOP_CMP_GT);
    necro_base_setup_primitive(scoped_symtable, intern, "lt<Bool>",  NULL, NECRO_PRIMOP_CMP_LT);
    necro_base_setup_primitive(scoped_symtable, intern, "gte<Bool>", NULL, NECRO_PRIMOP_CMP_GE);
    necro_base_setup_primitive(scoped_symtable, intern, "lte<Bool>", NULL, NECRO_PRIMOP_CMP_LE);
    necro_base_setup_primitive(scoped_symtable, intern, "&&",        NULL, NECRO_PRIMOP_BINOP_AND);
    necro_base_setup_primitive(scoped_symtable, intern, "||",        NULL, NECRO_PRIMOP_BINOP_OR);

    //--------------------
    // Compile, part II
    necro_dependency_analyze(info, intern, &base, &base.ast);
    necro_alias_analysis(info, &base.ast); // NOTE: Consider merging alias_analysis into RENAME_VAR phase?
    base.scoped_symtable = scoped_symtable;
    unwrap(void, necro_infer(info, intern, scoped_symtable, &base, &base.ast));
    unwrap(void, necro_monomorphize(info, intern, scoped_symtable, &base, &base.ast));

    // Finish
    return base;
}

NecroAstSymbol* necro_base_get_tuple_type(NecroBase* base, size_t num)
{
    switch (num)
    {
    case 2:  return base->tuple2_type;
    case 3:  return base->tuple3_type;
    case 4:  return base->tuple4_type;
    case 5:  return base->tuple5_type;
    case 6:  return base->tuple6_type;
    case 7:  return base->tuple7_type;
    case 8:  return base->tuple8_type;
    case 9:  return base->tuple9_type;
    case 10: return base->tuple10_type;
    default:
        assert(false && "Tuple arities above 10 are not supported");
        return NULL;
    }
}

NecroAstSymbol* necro_base_get_tuple_con(NecroBase* base, size_t num)
{
    switch (num)
    {
    case 2:  return base->tuple2_con;
    case 3:  return base->tuple3_con;
    case 4:  return base->tuple4_con;
    case 5:  return base->tuple5_con;
    case 6:  return base->tuple6_con;
    case 7:  return base->tuple7_con;
    case 8:  return base->tuple8_con;
    case 9:  return base->tuple9_con;
    case 10: return base->tuple10_con;
    default:
        assert(false && "Tuple arities above 10 are not supported");
        return NULL;
    }
}

// TODO: Instead of asserting this should be a user facing error!
// TODO: Dynamically create different arities!
NecroAstSymbol* necro_base_get_unboxed_tuple_type(NecroBase* base, size_t num)
{
    assert((num < NECRO_MAX_UNBOXED_TUPLE_TYPES) && "Unsupported Unboxed Tuple arity!");
    return base->unboxed_tuple_types[num];
}

NecroAstSymbol* necro_base_get_unboxed_tuple_con(NecroBase* base, size_t num)
{
    assert((num < NECRO_MAX_UNBOXED_TUPLE_TYPES) && "Unsupported Unboxed Tuple arity!");
    return base->unboxed_tuple_cons[num];
}

NecroAstSymbol* necro_base_get_env_type(NecroBase* base, size_t num)
{
    // TODO: Refactor to create env types dynamically, caching arities instead of blowing up at certain amounts?
    assert((num < NECRO_MAX_ENV_TYPES) && "Unsupported Env arity!");
    return base->env_types[num];
}

NecroAstSymbol* necro_base_get_env_con(NecroBase* base, size_t num)
{
    assert((num < NECRO_MAX_ENV_TYPES) && "Unsupported Env arity!");
    return base->env_cons[num];
}

NecroAstSymbol* necro_base_get_branch_type(NecroBase* base, size_t branch_size)
{
    assert((branch_size >= 2) && "Unsupported Branch Function arity!");
    assert((branch_size < NECRO_MAX_BRANCH_TYPES) && "Unsupported Branch Function arity!");
    return base->branch_types[branch_size];
}

NecroAstSymbol* necro_base_get_branch_con(NecroBase* base, size_t branch_size, size_t alternative)
{
    assert((branch_size >= 2) && "Unsupported Branch Function arity!");
    assert((branch_size < NECRO_MAX_BRANCH_TYPES) && "Unsupported Branch Function arity!");
    assert((alternative < branch_size) && "Alternative too large for Branch Function arity!");
    return base->branch_cons[branch_size][alternative];
}

NecroCoreAstSymbol* necro_base_get_proj_symbol(NecroPagedArena* arena, NecroBase* base)
{
    if (base->proj_fn->core_ast_symbol == NULL)
    {
        base->proj_fn->core_ast_symbol        = necro_core_ast_symbol_create_from_ast_symbol(arena, base->proj_fn);
        base->proj_fn->core_ast_symbol->arity = 2;
    }
    return base->proj_fn->core_ast_symbol;
}

#define NECRO_BASE_TEST_VERBOSE 0

void necro_base_test()
{
    necro_announce_phase("NecroBase");

    // Set up
    NecroIntern         intern          = necro_intern_create();
    NecroAstArena       ast             = necro_ast_arena_create(necro_intern_string(&intern, "Test"));
    NecroScopedSymTable scoped_symtable = necro_scoped_symtable_create();
    NecroBase           base            = necro_base_compile(&intern, &scoped_symtable);

    //--------------------
    // TODO list for Chad...
    //--------------------
    // Make tests that check that the base types are correct!!!!!

#if NECRO_BASE_TEST_VERBOSE
    necro_ast_arena_print(&base.ast);
    necro_scoped_symtable_print_top_scopes(&scoped_symtable);
#endif

    // Clean up
    necro_ast_arena_destroy(&ast);
    necro_base_destroy(&base);
    necro_scoped_symtable_destroy(&scoped_symtable);
    necro_intern_destroy(&intern);
}
