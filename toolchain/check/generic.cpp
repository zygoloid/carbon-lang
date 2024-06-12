// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "toolchain/check/generic.h"

#include "toolchain/sem_ir/ids.h"

namespace Carbon::Check {

// Register the symbolic constants in the given list as being owned by the
// specified generic.
static auto RegisterSymbolicConstants(Context& context,
                                      SemIR::InstBlockId insts_id,
                                      SemIR::GenericId generic_id,
                                      uint32_t first_index) -> void {
  for (auto [i, inst_id] :
       llvm::enumerate(context.inst_blocks().Get(insts_id))) {
    auto const_inst_id = context.constant_values().GetConstantInstId(inst_id);
    CARBON_CHECK(const_inst_id.is_valid())
        << "Non-constant instruction " << context.insts().Get(inst_id)
        << " in symbolic constants list.";
    uint32_t index = first_index + i;
    CARBON_CHECK(index >= first_index) << "Overflow in symbolic constant index";
    context.constant_values().Set(
        inst_id,
        context.constant_values().AddSymbolicConstant({.inst_id = const_inst_id,
                                                       .generic_id = generic_id,
                                                       .index = index}));
  }
}

// Register the instructions with symbolic types in the given list as having
// their types substituted as part of the specified generic.
static auto RegisterInstsWithSubstitutedTypes(Context& context,
                                              SemIR::InstBlockId insts_id,
                                              SemIR::GenericId generic_id,
                                              uint32_t first_index) -> void {
  for (auto [i, inst_id] :
       llvm::enumerate(context.inst_blocks().Get(insts_id))) {
    auto inst = context.insts().Get(inst_id);
    auto pattern_id = inst.type_id();
    if (pattern_id.is_substituted()) {
      // We can end up with the same instruction listed multiple times if it
      // gets replaced. We only need to set it as substituted once.
      continue;
    }
    CARBON_CHECK(context.types().GetConstantId(pattern_id).is_symbolic())
        << "Non-symbolic type " << pattern_id << " in type of " << inst
        << " in list of instructions with symbolic types.";
    uint32_t index = first_index + i;
    CARBON_CHECK(index >= first_index) << "Overflow in symbolic type index";
    inst.SetType(context.types().AddSubstitutedType(
        {.pattern_id = pattern_id, .generic_id = generic_id, .index = index}));
    // Access via sem_ir: context intentionally provides only a const handle to
    // the instruction store.
    context.sem_ir().insts().Set(inst_id, inst);
  }
}

auto FinishGenericDecl(Context& context,
                       SemIR::InstId decl_id) -> SemIR::GenericId {
  // For a generic function, build the corresponding Generic entity.
  if (context.scope_stack().compile_time_binding_stack().empty()) {
    context.generic_region_stack().PopNotGeneric();
    return SemIR::GenericId::Invalid;
  }

  auto bindings_id = context.inst_blocks().Add(
      context.scope_stack().compile_time_binding_stack());
  auto decl_region = context.generic_region_stack().PopGeneric();

  auto generic_id = context.generics().Add(SemIR::Generic{
      .decl_id = decl_id, .bindings_id = bindings_id, .decl = decl_region});
  RegisterSymbolicConstants(context, decl_region.symbolic_constant_insts_id,
                            generic_id, /*first_index=*/0);
  RegisterInstsWithSubstitutedTypes(context,
                                    decl_region.substituted_type_insts_id,
                                    generic_id, /*first_index=*/0);
  return generic_id;
}

auto FinishGenericRedecl(Context& context, SemIR::InstId decl_id,
                         SemIR::GenericId generic_id) -> void {
  // TODO: Compare contents of this region with the existing one on the
  // generic.
  (void)decl_id;
  (void)generic_id;
  context.generic_region_stack().PopAndDiscard();
}

auto FinishGenericDefinition(Context& context,
                             SemIR::GenericId generic_id) -> void {
  if (!generic_id.is_valid()) {
    // TODO: We can have symbolic constants in a context that had a non-generic
    // declaration, for example if there's a local generic let binding in a
    // function definition. Handle this case somehow -- perhaps by forming
    // substituted constant values now.
    context.generic_region_stack().PopAndDiscard();
    return;
  }

  auto& generic = context.generics().Get(generic_id);
  generic.definition = context.generic_region_stack().PopGeneric();
  // TODO: Indexing like this will be inefficient: we need to read the size of
  // the declaration's constants block to determine which block we index into.
  // Store the symbolic constant index as a discriminated union of a declaration
  // or definition index instead.
  auto first_constant_index_in_definition =
      context.inst_blocks().Get(generic.decl.symbolic_constant_insts_id).size();
  RegisterSymbolicConstants(context,
                            generic.definition.symbolic_constant_insts_id,
                            generic_id, first_constant_index_in_definition);
  auto first_type_index_in_definition =
      context.inst_blocks().Get(generic.decl.substituted_type_insts_id).size();
  RegisterInstsWithSubstitutedTypes(
      context, generic.definition.substituted_type_insts_id, generic_id,
      first_type_index_in_definition);
}

}  // namespace Carbon::Check
