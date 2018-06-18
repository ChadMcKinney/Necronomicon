/* Copyright (C) Chad McKinney and Curtis McKinney - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */

#ifndef TYPE_PRIM_H
#define TYPE_PRIM_H 1

#include <stdlib.h>
#include <stdbool.h>
#include "utility.h"
#include "utility/intern.h"
#include "type.h"
#include "type_class.h"
#include "symtable.h"
#include "renamer.h"
#include "d_analyzer.h"
#include "driver.h"
#include "llvm-c/Types.h"


//=====================================================
// Forward Declarations and typedefs
//=====================================================
typedef NecroAST_Node_Reified NecroASTNode;
struct NecroCodeGen;

//=====================================================
// NecroPrimDef
//=====================================================
struct NecroPrimDef;
typedef enum
{
    NECRO_PRIM_DEF_DATA,
    NECRO_PRIM_DEF_FUN,
    NECRO_PRIM_DEF_BIN_OP,
    NECRO_PRIM_DEF_CLASS,
    NECRO_PRIM_DEF_INSTANCE,
} NECRO_PRIM_DEF;

typedef struct
{
    NecroType*      type_type;
    NecroType*      type_fully_applied_type;
    NecroASTNode*   data_declaration_ast;
} NecroPrimDefData;

typedef struct
{
    NecroType*      type;
    NecroASTNode*   type_sig_ast;
} NecroPrimDefFun;

typedef struct
{
    NecroType*      type;
    NecroASTNode*   type_sig_ast;
    NecroASTNode*   definition_ast;
} NecroPrimDefBinOp;

typedef struct
{
    NecroTypeClass* type_class;
    NecroASTNode*   type_class_ast;
} NecroPrimDefClass;

typedef struct
{
    NecroTypeClassInstance* instance;
    NecroASTNode*           instance_ast;
} NecroPrimDefInstance;

typedef struct NecroPrimDef
{
    union
    {
        NecroPrimDefClass    class_def;
        NecroPrimDefInstance instance_def;
        NecroPrimDefFun      fun_def;
        NecroPrimDefData     data_def;
        NecroPrimDefBinOp    bin_op_def;
    };
    NecroCon              name;
    NecroCon*             global_name;
    struct NecroPrimDef*  next;
    NECRO_PRIM_DEF        type;
} NecroPrimDef;

//=====================================================
// PrimTypes
//=====================================================
typedef struct
{
    NecroCon two;
    NecroCon three;
    NecroCon four;
    NecroCon five;
    NecroCon six;
    NecroCon seven;
    NecroCon eight;
    NecroCon nine;
    NecroCon ten;
} NecroTupleTypes;

typedef struct NecroPrimTypes
{
    // Primitive types and functions
    NecroTupleTypes   tuple_types;
    NecroCon          necro_val_type;
    NecroCon          necro_data_type;
    NecroCon          any_type;
    NecroCon          io_type;
    NecroCon          unit_type;
    NecroCon          unit_con;
    NecroCon          list_type;
    NecroCon          sequence_type;
    NecroCon          unboxed_int_type;
    NecroCon          unboxed_float_type;
    NecroCon          int_type;
    NecroCon          float_type;
    NecroCon          audio_type;
    NecroCon          rational_type;
    NecroCon          char_type;
    NecroCon          bool_type;
    NecroCon          num_type_class;
    NecroCon          fractional_type_class;
    NecroCon          eq_type_class;
    NecroCon          ord_type_class;
    NecroCon          functor_type_class;
    NecroCon          applicative_type_class;
    NecroCon          monad_type_class;
    NecroCon          event_type;
    NecroCon          pattern_type;
    NecroCon          delay_fn;

    // Runtime functions
    NecroCon          mouse_x_fn;
    NecroCon          mouse_y_fn;

    // Utility
    NecroPrimDef*     defs;
    NecroPrimDef*     def_head;
    NecroPagedArena   arena;
    LLVMModuleRef     llvm_mod;
} NecroPrimTypes;

//=====================================================
// API
//=====================================================
NecroPrimTypes necro_create_prim_types(NecroIntern* intern);

NECRO_RETURN_CODE necro_prim_build_scope(NecroPrimTypes* prim_types, NecroScopedSymTable* scoped_symtable);
NECRO_RETURN_CODE necro_prim_rename(NecroPrimTypes* prim_types, NecroRenamer* renamer);
NECRO_RETURN_CODE necro_prim_infer(NecroPrimTypes* prim_types, NecroDependencyAnalyzer* d_analyzer, NecroInfer* infer, NECRO_PHASE phase);
void              necro_init_prim_defs(NecroPrimTypes* prim_types, NecroIntern* intern);
NECRO_RETURN_CODE necro_codegen_primitives(struct NecroCodeGen* codegen);

#endif // TYPE_PRIM_H