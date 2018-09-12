/* Copyright (C) Chad McKinney and Curtis McKinney - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */

#ifndef PARSER_H
#define PARSER_H 1

#include <stdlib.h>
#include <stdbool.h>
#include "lexer.h"
#include "arena.h"

/*
    TODO:
        * correct source_locs!!!!!
        * Consider removing OpPats
        * Better Type Class error messages
        * Better Class Instance error messages
*/

// Local offset into AST arena
typedef size_t NecroAST_LocalPtr;
static const NecroAST_LocalPtr null_local_ptr = (size_t) -1;

typedef enum
{
    NECRO_AST_UNDEFINED,
    NECRO_AST_CONSTANT,
    NECRO_AST_UN_OP,
    NECRO_AST_BIN_OP,
    NECRO_AST_IF_THEN_ELSE,
    NECRO_AST_TOP_DECL,
    NECRO_AST_DECL,
    NECRO_AST_SIMPLE_ASSIGNMENT,
    NECRO_AST_APATS_ASSIGNMENT,
    NECRO_AST_PAT_ASSIGNMENT,
    NECRO_AST_RIGHT_HAND_SIDE,
    NECRO_AST_LET_EXPRESSION,
    NECRO_AST_FUNCTION_EXPRESSION,
    // NECRO_AST_INFIX_EXPRESSION,
    NECRO_AST_VARIABLE,
    NECRO_AST_APATS,
    NECRO_AST_WILDCARD,
    NECRO_AST_LAMBDA,
    NECRO_AST_DO,
    NECRO_AST_PAT_EXPRESSION,
    NECRO_AST_LIST_NODE,
    NECRO_AST_EXPRESSION_LIST,
    NECRO_AST_EXPRESSION_ARRAY,
    NECRO_AST_TUPLE,
    NECRO_BIND_ASSIGNMENT,
    NECRO_PAT_BIND_ASSIGNMENT,
    NECRO_AST_ARITHMETIC_SEQUENCE,
    NECRO_AST_CASE,
    NECRO_AST_CASE_ALTERNATIVE,
    NECRO_AST_CONID,
    NECRO_AST_TYPE_APP,
    NECRO_AST_BIN_OP_SYM,
    NECRO_AST_OP_LEFT_SECTION,
    NECRO_AST_OP_RIGHT_SECTION,
    NECRO_AST_CONSTRUCTOR,
    NECRO_AST_SIMPLE_TYPE,
    NECRO_AST_DATA_DECLARATION,
    NECRO_AST_TYPE_CLASS_CONTEXT,
    NECRO_AST_TYPE_CLASS_DECLARATION,
    NECRO_AST_TYPE_CLASS_INSTANCE,
    NECRO_AST_TYPE_SIGNATURE,
    NECRO_AST_FUNCTION_TYPE,
    // NECRO_AST_MODULE,
} NecroAST_NodeType;

//=====================================================
// AST FunctionType
//=====================================================
typedef struct
{
    NecroAST_LocalPtr type;
    NecroAST_LocalPtr next_on_arrow;
} NecroAST_FunctionType;

//=====================================================
// AST TypeClassInstance
//=====================================================
typedef struct
{
    NecroAST_LocalPtr context;// optional, null_local_ptr if not present
    NecroAST_LocalPtr qtycls;
    NecroAST_LocalPtr inst;
    NecroAST_LocalPtr declarations; // Points to the next in the list, null_local_ptr if the end
} NecroAST_TypeClassInstance;

//=====================================================
// AST TypeClassDeclaration
//=====================================================
typedef struct
{
    NecroAST_LocalPtr context; // optional, null_local_ptr if not present
    NecroAST_LocalPtr tycls;
    NecroAST_LocalPtr tyvar;
    NecroAST_LocalPtr declarations; // Points to the next in the list, null_local_ptr if the end
} NecroAST_TypeClassDeclaration;

//=====================================================
// AST TypeClassContext
//=====================================================
typedef struct
{
    NecroAST_LocalPtr conid;
    NecroAST_LocalPtr varid;
} NecroAST_TypeClassContext;

//=====================================================
// AST TypeSignature
//=====================================================
typedef enum
{
    NECRO_SIG_DECLARATION,
    NECRO_SIG_TYPE_CLASS
} NECRO_SIG_TYPE;

typedef struct
{
    NecroAST_LocalPtr var;
    NecroAST_LocalPtr context; // optional, null_local_ptr if not present
    NecroAST_LocalPtr type;
    NECRO_SIG_TYPE    sig_type;
} NecroAST_TypeSignature;

//=====================================================
// AST DataDeclaration
//=====================================================
typedef struct
{
    NecroAST_LocalPtr simpletype;
    NecroAST_LocalPtr constructor_list; // Points to the next in the list, null_local_ptr if the end
} NecroAST_DataDeclaration;

//=====================================================
// AST TypeConstructor
//=====================================================
typedef struct
{
    NecroAST_LocalPtr conid;
    NecroAST_LocalPtr arg_list; // Points to the next in the list, null_local_ptr if the end
} NecroAST_Constructor;

//=====================================================
// AST SimpleType
//=====================================================
typedef struct
{
    NecroAST_LocalPtr type_con;
    NecroAST_LocalPtr type_var_list; // Points to the next in the list, null_local_ptr if the end
} NecroAST_SimpleType;

//=====================================================
// AST BinOpSym
//=====================================================
typedef struct
{
    NecroAST_LocalPtr left;
    NecroAST_LocalPtr op;
    NecroAST_LocalPtr right;
} NecroAST_BinOpSym;

//=====================================================
// AST Type App
//=====================================================
typedef struct
{
    NecroAST_LocalPtr ty;
    NecroAST_LocalPtr next_ty; // Points to the next in the list, null_local_ptr if the end
} NecroAST_TypeApp;

//=====================================================
// AST ConID
//=====================================================
typedef enum
{
    NECRO_CON_VAR,
    NECRO_CON_TYPE_VAR,
    NECRO_CON_DATA_DECLARATION,
    NECRO_CON_TYPE_DECLARATION,
} NECRO_CON_TYPE;
const char* con_type_string(NECRO_CON_TYPE symbol_type);

typedef struct
{
    NecroSymbol    symbol;
    NECRO_CON_TYPE con_type;
} NecroAST_ConID;

//=====================================================
// AST Case
//=====================================================
typedef struct
{
    NecroAST_LocalPtr expression;
    NecroAST_LocalPtr alternatives;
} NecroAST_Case;

//=====================================================
// AST CaseAlternative
//=====================================================
typedef struct
{
    NecroAST_LocalPtr pat;
    NecroAST_LocalPtr body;
} NecroAST_CaseAlternative;

//=====================================================
// AST Module
//=====================================================

//=====================================================
// AST Undefined
//=====================================================
typedef struct
{
    uint8_t _pad;
} NecroAST_Undefined;

//=====================================================
// AST Constant
//=====================================================
typedef enum
{
    NECRO_AST_CONSTANT_FLOAT,
    NECRO_AST_CONSTANT_INTEGER,
    NECRO_AST_CONSTANT_STRING,
    // NECRO_AST_CONSTANT_BOOL,
    NECRO_AST_CONSTANT_CHAR,

    NECRO_AST_CONSTANT_FLOAT_PATTERN,
    NECRO_AST_CONSTANT_INTEGER_PATTERN,
    NECRO_AST_CONSTANT_CHAR_PATTERN,
} NecroAST_ConstantType;

typedef struct
{
    union
    {
        double double_literal;
        int64_t int_literal;
        NecroSymbol symbol;
        uint32_t char_literal;
    };
    NecroAST_ConstantType type;
} NecroAST_Constant;

//=====================================================
// AST Unary Operation
//=====================================================

// typedef enum
// {
//     NECRO_UN_OP_NEG = 0,
//     NECRO_UN_OP_COUNT,
//     NECRO_UN_OP_UNDEFINED = NECRO_UN_OP_COUNT
// } NecroAST_UnaryOpType;
//
// typedef struct
// {
//     NecroAST_LocalPtr rhs;
//     NecroAST_UnaryOpType type;
// } NecroAST_UnaryOp;

//=====================================================
// AST Binary Operation
//=====================================================

typedef enum
{
    NECRO_BIN_OP_ADD = 0,
    NECRO_BIN_OP_SUB,
    NECRO_BIN_OP_MUL,
    NECRO_BIN_OP_DIV,
    NECRO_BIN_OP_MOD,
    NECRO_BIN_OP_GT,
    NECRO_BIN_OP_LT,
    NECRO_BIN_OP_GTE,
    NECRO_BIN_OP_LTE,
    NECRO_BIN_OP_COLON,
	NECRO_BIN_OP_DOUBLE_COLON,
	NECRO_BIN_OP_LEFT_SHIFT,
	NECRO_BIN_OP_RIGHT_SHIFT,
	NECRO_BIN_OP_PIPE,
	NECRO_BIN_OP_FORWARD_PIPE,
	NECRO_BIN_OP_BACK_PIPE,
    NECRO_BIN_OP_EQUALS,
    NECRO_BIN_OP_NOT_EQUALS,
	NECRO_BIN_OP_AND,
	NECRO_BIN_OP_OR,
    NECRO_BIN_OP_DOT,
    NECRO_BIN_OP_DOLLAR,
    NECRO_BIN_OP_BIND_RIGHT,
    NECRO_BIN_OP_BIND_LEFT,
    NECRO_BIN_OP_DOUBLE_EXCLAMATION,
    NECRO_BIN_OP_APPEND,
    NECRO_BIN_OP_FBY,
    NECRO_BIN_OP_COUNT,
    NECRO_BIN_OP_UNDEFINED = NECRO_BIN_OP_COUNT
} NecroAST_BinOpType;

typedef struct
{
    NecroAST_LocalPtr  lhs;
    NecroAST_LocalPtr  rhs;
    NecroAST_BinOpType type;
    NecroSymbol        symbol;
} NecroAST_BinOp;

//=====================================================
// AST Op Left Section
//=====================================================
typedef struct
{
    NecroAST_LocalPtr  left;
    NecroAST_BinOpType type;
    NecroSymbol        symbol;
} NecroAST_OpLeftSection;

//=====================================================
// AST Op Right Section
//=====================================================
typedef struct
{
    NecroAST_LocalPtr right;
    NecroAST_BinOpType type;
    NecroSymbol        symbol;
} NecroAST_OpRightSection;

//=====================================================
// AST if then else
//=====================================================

typedef struct
{
    NecroAST_LocalPtr if_expr;
    NecroAST_LocalPtr then_expr;
    NecroAST_LocalPtr else_expr;
} NecroAST_IfThenElse;

//=====================================================
// AST Right Hand Side
//=====================================================
typedef struct
{
    NecroAST_LocalPtr expression;
    NecroAST_LocalPtr declarations;
} NecroAST_RightHandSide;

//=====================================================
// AST Let Expression
//=====================================================

typedef struct
{
    NecroAST_LocalPtr expression;
    NecroAST_LocalPtr declarations;
} NecroAST_LetExpression;

//=====================================================
// AST Simple Assignment
//=====================================================

typedef struct
{
    NecroSymbol       variable_name;
    NecroAST_LocalPtr rhs;
    NecroAST_LocalPtr initializer;
} NecroAST_SimpleAssignment;

//=====================================================
// AST Bind Assignment
//=====================================================

typedef struct
{
    NecroSymbol variable_name;
    NecroAST_LocalPtr expression;
} NecroAST_BindAssignment;

//=====================================================
// AST Pat Bind Assignment
//=====================================================
typedef struct
{
    NecroAST_LocalPtr pat;
    NecroAST_LocalPtr expression;
} NecroAST_PatBindAssignment;

//=====================================================
// AST apats
//=====================================================
typedef struct
{
    NecroAST_LocalPtr apat;
    NecroAST_LocalPtr next_apat;
} NecroAST_Apats;

//=====================================================
// AST Apats Assignment
//=====================================================
typedef struct
{
    NecroSymbol variable_name;
    NecroAST_LocalPtr apats;
    NecroAST_LocalPtr rhs;
} NecroAST_ApatsAssignment;

//=====================================================
// AST Pat Assignment
//=====================================================
typedef struct
{
    NecroAST_LocalPtr pat;
    NecroAST_LocalPtr rhs;
} NecroAST_PatAssignment;

//=====================================================
// AST Lambda
//=====================================================
typedef struct
{
    NecroAST_LocalPtr apats;
    NecroAST_LocalPtr expression;
} NecroAST_Lambda;

//=====================================================
// AST List Node
//=====================================================
typedef struct
{
    NecroAST_LocalPtr item;
    NecroAST_LocalPtr next_item;
} NecroAST_ListNode;

//=====================================================
// AST Expression List
//=====================================================
typedef struct
{
    NecroAST_LocalPtr expressions; // NecroAST_ListNode of expressions
} NecroAST_ExpressionList;

//=====================================================
// AST Expression Array
//=====================================================
typedef struct
{
    NecroAST_LocalPtr expressions; // NecroAST_ListNode of expressions
} NecroAST_ExpressionArray;

//=====================================================
// AST Tuple
//=====================================================
typedef struct
{
    NecroAST_LocalPtr expressions; // NecroAST_ListNode of expressions
} NecroAST_Tuple;

//=====================================================
// AST Do
//=====================================================
typedef struct
{
    NecroAST_LocalPtr statement_list; // NecroAST_ListNode of do statement items
} NecroAST_Do;

//=====================================================
// AST Pattern Expression
//=====================================================
typedef struct
{
    NecroAST_LocalPtr expressions; // NecroAST_ListNode of expressions
} NecroAST_PatternExpression;

//=====================================================
// AST Variable
//=====================================================
typedef enum
{
    NECRO_VAR_VAR,
    NECRO_VAR_TYPE_FREE_VAR,
    NECRO_VAR_TYPE_VAR_DECLARATION,
    NECRO_VAR_DECLARATION,
    // NECRO_VAR_PAT_BIND, // TODO: FINISH!!!!
    NECRO_VAR_SIG,
    NECRO_VAR_CLASS_SIG,
} NECRO_VAR_TYPE;
const char* var_type_string(NECRO_VAR_TYPE symbol_type);

typedef struct
{
    NecroSymbol       symbol;
    NECRO_VAR_TYPE    var_type;
    NecroAST_LocalPtr initializer;
} NecroAST_Variable;

//=====================================================
// AST Function Expression
//=====================================================
typedef struct
{
    NecroAST_LocalPtr aexp;
    NecroAST_LocalPtr next_fexpression; // Points to the next in the list, null_local_ptr if the end
} NecroAST_FunctionExpression;

//=====================================================
// AST Declarations
//=====================================================
typedef struct
{
    NecroAST_LocalPtr declaration_impl;
    NecroAST_LocalPtr next_declaration; // Points to the next in the list, null_local_ptr if the end
} NecroAST_Declaration;

//=====================================================
// AST Top Declarations
//=====================================================
typedef struct
{
    NecroAST_LocalPtr declaration;
    NecroAST_LocalPtr next_top_decl; // Points to the next in the list, null_local_ptr if the end
} NecroAST_TopDeclaration;

//=====================================================
// AST Module
//=====================================================

// To Do: Define these!
// typedef struct
// {
//     NecroAST_LocalPtr body;
// } NecroAST_SimpleModule;
//
// typedef struct
// {
//     NecroAST_LocalPtr body;
// } NecroAST_ModuleWithExports;
//
// typedef union
// {
//     NecroAST_Body body;
//     NecroAST_SimpleModule simple_module;
//     NecroAST_ModuleWithExports module_with_exports;
// } NecroAST_Module;

//=====================================================
// AST Arithmetic Sequence
//=====================================================
typedef enum
{
    NECRO_ARITHMETIC_ENUM_FROM,
    NECRO_ARITHMETIC_ENUM_FROM_TO,
    NECRO_ARITHMETIC_ENUM_FROM_THEN_TO,
} NecroAST_ArithmeticSeqType;

typedef struct
{
    NecroAST_LocalPtr from;
    NecroAST_LocalPtr then;
    NecroAST_LocalPtr to;
    NecroAST_ArithmeticSeqType type;
} NecroAST_ArithmeticSequence;


//=====================================================
// AST Node
//=====================================================
typedef struct
{
    union
    {
        // NecroAST_UnaryOp unary_op; // Do we need this?
        NecroAST_Undefined undefined;
        NecroAST_Constant constant;
        NecroAST_BinOp bin_op;
        NecroAST_IfThenElse if_then_else;
        NecroAST_TopDeclaration top_declaration;
        NecroAST_Declaration declaration;
        NecroAST_SimpleAssignment simple_assignment;
        NecroAST_Apats apats;
        NecroAST_ApatsAssignment apats_assignment;
        NecroAST_PatAssignment pat_assignment;
        NecroAST_RightHandSide right_hand_side;
        NecroAST_LetExpression let_expression;
        NecroAST_FunctionExpression fexpression;
        NecroAST_Variable variable;
        NecroAST_Lambda lambda;
        NecroAST_Do do_statement;
        NecroAST_ListNode list;
        NecroAST_ExpressionList expression_list;
        NecroAST_ExpressionArray expression_array;
        NecroAST_Tuple tuple;
        NecroAST_BindAssignment bind_assignment;
        NecroAST_PatBindAssignment pat_bind_assignment;
        NecroAST_ArithmeticSequence arithmetic_sequence;
        NecroAST_Case case_expression;
        NecroAST_CaseAlternative case_alternative;
        NecroAST_ConID conid;
        NecroAST_BinOpSym bin_op_sym;
        NecroAST_OpLeftSection op_left_section;
        NecroAST_OpRightSection op_right_section;
        NecroAST_TypeApp type_app;
        NecroAST_SimpleType simple_type;
        NecroAST_Constructor constructor;
        NecroAST_DataDeclaration data_declaration;
        NecroAST_TypeClassContext type_class_context;
        NecroAST_TypeClassDeclaration type_class_declaration;
        NecroAST_TypeClassInstance type_class_instance;
        NecroAST_TypeSignature type_signature;
        NecroAST_FunctionType function_type;
        NecroAST_PatternExpression pattern_expression;
    };
    NecroAST_NodeType type;
    NecroSourceLoc    source_loc;
    NecroSourceLoc    end_loc;
} NecroAST_Node;

//=====================================================
// AST
//=====================================================
typedef struct
{
    NecroArena        arena;
    NecroAST_LocalPtr root;
} NecroAST;
NecroAST       necro_empty_ast();
void           necro_destroy_ast(NecroAST* ast);
NecroAST_Node* ast_get_node(NecroAST* ast, NecroAST_LocalPtr local_ptr);
NecroAST_Node* ast_get_root_node(NecroAST* ast);
NecroAST_Node* necro_parse_ast_alloc(NecroArena* arena, NecroAST_LocalPtr* local_ptr);
void           print_ast(NecroAST* ast, NecroIntern* intern, NecroAST_LocalPtr root_node_ptr);

//=====================================================
// Parsing
//=====================================================
NecroResult(void) necro_parse(NecroLexTokenVector* tokens, NecroIntern* intern, NecroAST* out_ast, NecroCompileInfo info);
const char*       bin_op_name(NecroAST_BinOpType type);

#endif // PARSER_H