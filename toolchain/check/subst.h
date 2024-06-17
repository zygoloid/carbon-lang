// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef CARBON_TOOLCHAIN_CHECK_SUBST_H_
#define CARBON_TOOLCHAIN_CHECK_SUBST_H_

#include "toolchain/check/context.h"
#include "toolchain/sem_ir/ids.h"

namespace Carbon::Check {

// A list of substitutions to perform. This is an array indexed by
// `CompileTimeBindIndex`. The instruction indexes should be those of constant
// instructions, or `Invalid` if substitution is not performed for a particular
// index.
using Substitutions = llvm::ArrayRef<SemIR::InstId>;

// Build a list of substitutions that performs a single replacement.
inline auto BuildSingleSubstitution(SemIR::CompileTimeBindIndex index,
                                    SemIR::InstId replacement)
    -> llvm::SmallVector<SemIR::InstId> {
  llvm::SmallVector<SemIR::InstId> substitutions;
  substitutions.resize(index.index + 1, SemIR::InstId::Invalid);
  substitutions[index.index] = replacement;
  return substitutions;
}

// Replaces the `BindSymbolicName` instruction `bind_id` with `replacement_id`
// throughout the constant `const_id`, and returns the substituted value.
auto SubstConstant(Context& context, SemIR::ConstantId const_id,
                   Substitutions substitutions) -> SemIR::ConstantId;

// Replaces the `BindSymbolicName` instruction `bind_id` with `replacement_id`
// throughout the type `type_id`, and returns the substituted value.
auto SubstType(Context& context, SemIR::TypeId type_id,
               Substitutions substitutions) -> SemIR::TypeId;

}  // namespace Carbon::Check

#endif  // CARBON_TOOLCHAIN_CHECK_SUBST_H_
