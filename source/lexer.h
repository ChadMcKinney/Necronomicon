/* Copyright (C) Chad McKinney and Curtis McKinney - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */

#ifndef LEXER_H
#define LEXER_H 1

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "utility.h"
#include "intern.h"

//=====================================================
// Lexing
//=====================================================
typedef enum
{
    // Keywords        // NOTE THESE KEYWORDS MUST BE THE FIRST ENTRIES IN THE NECRO_LEX_TOKEN_TYPE enum!!!!
    NECRO_LEX_LET = 0, // NOTE! let, where, of, and do are layout keywords. Their order in this list is important { 0, 1, 2, 3 }, and should not be changed!!!!
    NECRO_LEX_WHERE,
    NECRO_LEX_OF,
    NECRO_LEX_DO,
    NECRO_LEX_CASE,
    NECRO_LEX_CLASS,
    NECRO_LEX_DATA,
    NECRO_LEX_DERIVING,
    NECRO_LEX_FORALL,
    NECRO_LEX_IF,
    NECRO_LEX_ELSE,
    NECRO_LEX_THEN,
    NECRO_LEX_IMPORT,
    NECRO_LEX_INSTANCE,
    NECRO_LEX_IN,
    NECRO_LEX_MODULE,
    NECRO_LEX_NEWTYPE,
    NECRO_LEX_TYPE,

    // End of Keywords
    // NOTE: This must come after all of the keywords in this enum!
    NECRO_LEX_END_OF_KEY_WORDS,

    // Literals
    NECRO_LEX_INTEGER_LITERAL,
    NECRO_LEX_FLOAT_LITERAL,
    NECRO_LEX_IDENTIFIER,
    NECRO_LEX_TYPE_IDENTIFIER,
    NECRO_LEX_STRING_LITERAL,
    NECRO_LEX_CHAR_LITERAL,

    // Operators
    NECRO_LEX_ADD,
    NECRO_LEX_SUB,
    NECRO_LEX_MUL,
    NECRO_LEX_DIV,
    NECRO_LEX_MOD,
    NECRO_LEX_GT,
    NECRO_LEX_LT,
    NECRO_LEX_GTE,
    NECRO_LEX_LTE,
    NECRO_LEX_DOUBLE_COLON,
    NECRO_LEX_LEFT_SHIFT,
    NECRO_LEX_RIGHT_SHIFT,
    NECRO_LEX_PIPE,
    NECRO_LEX_FORWARD_PIPE,
    NECRO_LEX_BACK_PIPE,
    NECRO_LEX_EQUALS,
    NECRO_LEX_NOT_EQUALS,
    NECRO_LEX_AND,
    NECRO_LEX_OR,
    NECRO_LEX_BIND_RIGHT,
    NECRO_LEX_BIND_LEFT,
    NECRO_LEX_DOUBLE_EXCLAMATION,
    NECRO_LEX_APPEND,

    // Punctuation
    NECRO_LEX_ACCENT,
    NECRO_LEX_DOT,
    NECRO_LEX_DOUBLE_DOT,
    NECRO_LEX_TILDE,
    NECRO_LEX_BACK_SLASH,
    NECRO_LEX_CARET,
    NECRO_LEX_DOLLAR,
    NECRO_LEX_AT,
    NECRO_LEX_AMPERSAND,
    NECRO_LEX_COLON,
    NECRO_LEX_SEMI_COLON,
    NECRO_LEX_LEFT_BRACKET,
    NECRO_LEX_RIGHT_BRACKET,
    NECRO_LEX_LEFT_PAREN,
    NECRO_LEX_RIGHT_PAREN,
    NECRO_LEX_LEFT_BRACE,
    NECRO_LEX_RIGHT_BRACE,
    NECRO_LEX_COMMA,
    NECRO_LEX_UNDER_SCORE,
    NECRO_LEX_ASSIGN,
    NECRO_LEX_QUESTION_MARK,
    NECRO_LEX_EXCLAMATION,
    NECRO_LEX_HASH,
    NECRO_LEX_UNIT,
    NECRO_LEX_LEFT_ARROW,
    NECRO_LEX_RIGHT_ARROW,
    NECRO_LEX_FAT_RIGHT_ARROW,

    NECRO_LEX_END_OF_STREAM
} NECRO_LEX_TOKEN_TYPE;

typedef struct
{
    union
    {
        int64_t          int_literal;
        double           double_literal;
        NecroSymbol      symbol;
        bool             boolean_literal;
        char             char_literal;
    };
    // size_t               character_number;
    // size_t               line_number;
    NecroSourceLoc       source_loc;
    NECRO_LEX_TOKEN_TYPE token;
} NecroLexToken;
NECRO_DECLARE_VECTOR(NecroLexToken, NecroLexToken, lex_token)

#define NECRO_MAX_INDENTATIONS 128

typedef struct
{
    size_t               character_number;
    size_t               line_number;
    size_t               pos;
    size_t               block_indentation_levels[NECRO_MAX_INDENTATIONS];
    size_t               current_indentation_block;
    const char*          str;
    NecroLexTokenVector  tokens;
    NecroIntern          intern;
    NecroError           error;
} NecroLexer;

typedef enum
{
    NECRO_LEX_RESULT_SUCCESSFUL,
    NECRO_LEX_RESULT_ERROR
} NECRO_LEX_RESULT;

typedef enum
{
    NECRO_LEX_NUM_STATE_INT,
    NECRO_LEX_NUM_STATE_FLOAT_AT_DOT,
    NECRO_LEX_NUM_STATE_FLOAT_POST_DOT,
} NECRO_LEX_NUM_STATE;

// API
NecroLexer        necro_create_lexer(const char* str);
void              necro_destroy_lexer(NecroLexer* lexer);
void              necro_print_lexer(NecroLexer* lexer);
NECRO_RETURN_CODE necro_lex(NecroLexer* lexer);
const char*       necro_lex_token_type_string(NECRO_LEX_TOKEN_TYPE token);
void              necro_test_lexer();

#endif // LEXER_H
