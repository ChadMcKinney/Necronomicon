/* Copyright (C) Chad McKinney and Curtis McKinney - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */

#include <stdio.h>
#include <inttypes.h>
#include "utility.h"
#include "lexer.h"
#include "parser.h"
#include "intern.h"
#include "runtime.h"
#include "vault.h"
#include "symtable.h"
#include "ast.h"
#include "renamer.h"
#include "driver.h"

void necro_compile(const char* input_string, NECRO_PHASE compilation_phase)
{
    if (compilation_phase == NECRO_PHASE_NONE)
        return;

    //=====================================================
    // Lexing, PRE - Layout
    //=====================================================
    necro_announce_phase("Lexing");
    NecroLexer lexer = necro_create_lexer(input_string);
    if (necro_lex(&lexer) != NECRO_SUCCESS)
    {
        necro_print_error(&lexer.error, input_string, "Syntax");
        return;
    }
    necro_print_lexer(&lexer);
    if (compilation_phase == NECRO_PHASE_LEX_PRE_LAYOUT)
        return;
    //=====================================================
    // Lexing, Layout
    //=====================================================
    if (necro_lex_fixup_layout(&lexer) != NECRO_SUCCESS)
    {
        necro_print_error(&lexer.error, input_string, "Syntax");
        return;
    }
    necro_print_lexer(&lexer);
    if (compilation_phase == NECRO_PHASE_LEX)
        return;

    //=====================================================
    // Parsing
    //=====================================================
    necro_announce_phase("Parsing");
    NecroAST    ast = { construct_arena(lexer.tokens.length * sizeof(NecroAST_Node)) };
    NecroParser parser;
    construct_parser(&parser, &ast, lexer.tokens.data, &lexer.intern);
    NecroAST_LocalPtr root_node_ptr = null_local_ptr;
    if (parse_ast(&parser, &root_node_ptr) != NECRO_SUCCESS)
    {
        necro_print_error(&parser.error, input_string, "Parsing");
        return;
    }
    print_ast(&ast, &lexer.intern, root_node_ptr);
    if (compilation_phase == NECRO_PHASE_PARSE)
        return;

    //=====================================================
    // Reifying
    //=====================================================
    necro_announce_phase("Reifying");
    NecroAST_Reified ast_r = necro_reify_ast(&ast, root_node_ptr);
    necro_print_reified_ast(&ast_r, &lexer.intern);
    if (compilation_phase == NECRO_PHASE_REIFY)
        return;

    //=====================================================
    // Build Scopes
    //=====================================================
    necro_announce_phase("Building Scopes");
    NecroSymTable       symtable        = necro_create_symtable(&lexer.intern);
    NecroScopedSymTable scoped_symtable = necro_create_scoped_symtable(&symtable);
    if (necro_build_scopes(&scoped_symtable, &ast_r) != NECRO_SUCCESS)
    {
        necro_print_error(&scoped_symtable.error, input_string, "Building Scopes");
        return;
    }
    necro_symtable_print(&symtable);
    necro_scoped_symtable_print(&scoped_symtable);
    if (compilation_phase == NECRO_PHASE_BUILD_SCOPES)
        return;

    //=====================================================
    // Renaming
    //=====================================================
    necro_announce_phase("Renaming");
    NecroRenamer renamer = necro_create_renamer(&scoped_symtable);
    if (necro_rename_declare_pass(&renamer, &ast_r) != NECRO_SUCCESS)
    {
        necro_print_reified_ast(&ast_r, &lexer.intern);
        necro_symtable_print(&symtable);
        necro_print_error(&renamer.error, input_string, "Renaming (Declare Pass)");
        return;
    }
    if (necro_rename_var_pass(&renamer, &ast_r) != NECRO_SUCCESS)
    {
        necro_print_reified_ast(&ast_r, &lexer.intern);
        necro_symtable_print(&symtable);
        necro_print_error(&renamer.error, input_string, "Renaming (Var Pass)");
        return;
    }
    necro_print_reified_ast(&ast_r, &lexer.intern);
    puts("");
    necro_symtable_print(&symtable);
    if (compilation_phase == NECRO_PHASE_RENAME)
        return;

    //=====================================================
    // Cleaning up
    //=====================================================
    necro_announce_phase("Cleaning Up");
    destruct_parser(&parser);
    necro_destroy_lexer(&lexer);
    destruct_arena(&ast.arena);
}

void necro_test(NECRO_TEST test)
{
    switch (test)
    {
    case NECRO_TEST_VM:        necro_test_vm();        break;
    case NECRO_TEST_DVM:       necro_test_dvm();       break;
    case NECRO_TEST_SYMTABLE:  necro_symtable_test();  break;
    case NECRO_TEST_SLAB:      necro_test_slab();      break;
    case NECRO_TEST_TREADMILL: necro_test_treadmill(); break;
    case NECRO_TEST_LEXER:     necro_test_lexer();     break;
    case NECRO_TEST_INTERN:    necro_test_intern();    break;
    case NECRO_TEST_VAULT:     necro_vault_test();     break;
    case NECRO_TEST_ARCHIVE:   necro_archive_test();   break;
    case NECRO_TEST_REGION:    necro_region_test();    break;
    case NECRO_TEST_ALL:
        necro_test_dvm();
        necro_symtable_test();
        necro_test_lexer();
        necro_test_intern();
        necro_archive_test();
        necro_region_test();
        break;
    default:                                           break;
    }
}