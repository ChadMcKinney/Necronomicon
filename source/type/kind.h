/* Copyright (C) Chad McKinney and Curtis McKinney - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */

#ifndef KIND_H
#define KIND_H 1

#include <stdlib.h>
#include <stdbool.h>
#include "utility.h"
#include "type.h"

struct NecroBase;

NecroType*             necro_kind_fresh_kind_var(NecroPagedArena* arena, struct NecroBase* base, struct NecroScope* scope);
void                   necro_kind_init_kinds(struct NecroBase* base, struct NecroScopedSymTable* scoped_symtable, NecroIntern* intern);
NecroResult(NecroType) necro_kind_unify_with_info(NecroType* kind1, NecroType* kind2, NecroScope* scope, NecroSourceLoc source_loc, NecroSourceLoc end_loc);
NecroResult(NecroType) necro_kind_unify(NecroType* kind1, NecroType* kind2, NecroScope* scope);
NecroResult(NecroType) necro_kind_infer(NecroPagedArena* arena, struct NecroBase* base, NecroType* type, NecroSourceLoc source_loc, NecroSourceLoc end_loc);
void                   necro_kind_default_type_kinds(NecroPagedArena* arena, struct NecroBase* base, NecroType* type);
NecroResult(void)      necro_kind_infer_default_unify_with_star(NecroPagedArena* arena, struct NecroBase* base, NecroType* type, NecroScope* scope, NecroSourceLoc source_loc, NecroSourceLoc end_loc);
NecroResult(void)      necro_kind_infer_default_unify_with_kind(NecroPagedArena* arena, struct NecroBase* base, NecroType* type, NecroType* kind, NecroSourceLoc source_loc, NecroSourceLoc end_loc);
NecroResult(void)      necro_kind_infer_default(NecroPagedArena* arena, struct NecroBase* base, NecroType* type, NecroSourceLoc source_loc, NecroSourceLoc end_loc);

bool                   necro_kind_is_type(const struct NecroBase* base, const NecroType* type_with_kind_to_check);
bool                   necro_kind_is_ownership(const struct NecroBase* base, const NecroType* type_with_kind_to_check);
bool                   necro_kind_is_nat(const struct NecroBase* base, const NecroType* type_with_kind_to_check);
bool                   necro_kind_is_kind(const struct NecroBase* base, const NecroType* type_with_kind_to_check);

#endif // KIND_H