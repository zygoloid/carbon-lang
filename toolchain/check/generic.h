// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef CARBON_TOOLCHAIN_CHECK_GENERIC_H_
#define CARBON_TOOLCHAIN_CHECK_GENERIC_H_

#include "toolchain/check/context.h"
#include "toolchain/sem_ir/generic.h"
#include "toolchain/sem_ir/ids.h"

namespace Carbon::Check {

// Finish processing a potentially generic declaration and produce a
// corresponding generic object. Returns SemIR::GenericId::Invalid if this
// declaration is not actually generic.
//
// This pops the top entry from the `GenericRegionStack`. The caller is
// reponsible for pushing that entry at the start of the declaration.
auto FinishGenericDecl(Context& context,
                       SemIR::InstId decl_id) -> SemIR::GenericId;

// Merge a redeclaration of a generic into the original declaration.
//
// This pops the top entry from the `GenericRegionStack`. The caller is
// reponsible for pushing that entry at the start of the declaration.
auto FinishGenericRedecl(Context& context, SemIR::InstId decl_id,
                         SemIR::GenericId generic_id) -> void;

// Finish processing a potentially generic definition.
//
// This pops the top entry from the `GenericRegionStack`. The caller is
// reponsible for pushing that entry at the start of the definition, after the
// end of the declaration portion.
auto FinishGenericDefinition(Context& context,
                             SemIR::GenericId generic_id) -> void;

}  // namespace Carbon::Check

#endif  // CARBON_TOOLCHAIN_CHECK_GENERIC_H_
