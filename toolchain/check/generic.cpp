// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "toolchain/check/generic.h"

#include "toolchain/check/subst.h"
#include "toolchain/sem_ir/ids.h"

namespace Carbon::Check {

// Register the symbolic constants in the given list as being owned by the
// specified generic.
static auto RegisterSymbolicConstants(Context& context,
                                      SemIR::InstBlockId insts_id,
                                      SemIR::GenericId generic_id,
                                      bool definition) -> void {
  for (auto [i, inst_id] :
       llvm::enumerate(context.inst_blocks().Get(insts_id))) {
    auto const_inst_id = context.constant_values().GetConstantInstId(inst_id);
    CARBON_CHECK(const_inst_id.is_valid())
        << "Non-constant instruction " << context.insts().Get(inst_id)
        << " in symbolic constants list.";
    int32_t index = definition ? -i - 1 : i;
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
                                              bool definition) -> void {
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
    int32_t index = definition ? -i - 1 : i;
    inst.SetType(context.types().AddSubstitutedType(
        {.pattern_id = pattern_id, .generic_id = generic_id, .index = index}));
    // Access via sem_ir: context intentionally provides only a const handle to
    // the instruction store.
    context.sem_ir().insts().Set(inst_id, inst);
  }
}

auto FinishGenericDecl(Context& context, SemIR::InstId decl_id)
    -> SemIR::GenericId {
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
                            generic_id, /*definition=*/false);
  RegisterInstsWithSubstitutedTypes(context,
                                    decl_region.substituted_type_insts_id,
                                    generic_id, /*definition=*/false);
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

auto FinishGenericDefinition(Context& context, SemIR::GenericId generic_id)
    -> void {
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
  RegisterSymbolicConstants(context,
                            generic.definition.symbolic_constant_insts_id,
                            generic_id, /*definition=*/true);
  RegisterInstsWithSubstitutedTypes(
      context, generic.definition.substituted_type_insts_id, generic_id,
      /*definition=*/true);
}

static auto SubstituteRegion(Context& context,
                             SemIR::Generic::Region& generic_region,
                             SemIR::GenericInstance::Region& instance_region,
                             Substitutions substitutions) -> void {
  auto types_in_generic =
      context.inst_blocks().Get(generic_region.substituted_type_insts_id);
  llvm::SmallVector<SemIR::TypeId> types_in_instance;
  types_in_instance.reserve(types_in_generic.size());
  for (auto inst_id : types_in_generic) {
    // TODO: We do a lot of repeated work here. Cache the values we substitute
    // into so we only substitute into each once.
    types_in_instance.push_back(SubstType(
        context, context.insts().Get(inst_id).type_id(), substitutions));
  }
  instance_region.substituted_types_id =
      context.type_blocks().Add(types_in_instance);

    // TODO: Substitute into constants.
}

auto MakeGenericInstance(Context& context, SemIR::GenericId generic_id,
                         SemIR::InstBlockId args_id)
    -> SemIR::GenericInstanceId {
  auto instance_id = context.generic_instances().GetOrAdd(generic_id, args_id);
  auto& instance = context.generic_instances().Get(instance_id);

  // TODO: Remove this once we import generics properly.
  if (!generic_id.is_valid()) {
    return instance_id;
  }

  if (!instance.decl.substituted_types_id.is_valid()) {
    // Perform substitution into the declaration.
    auto substitutions = context.inst_blocks().Get(args_id);
    auto& generic = context.generics().Get(generic_id);
    SubstituteRegion(context, generic.decl, instance.decl, substitutions);
  }
  return instance_id;
}

auto MakeUnsubstitutedGenericInstance(
    Context& context, SemIR::GenericId generic_id) -> SemIR::GenericInstanceId {
  // TODO: Remove this once we import generics properly.
  if (!generic_id.is_valid()) {
    return SemIR::GenericInstanceId::Invalid;
  }

  auto& generic = context.generics().Get(generic_id);
  auto args = context.inst_blocks().Get(generic.bindings_id);

  // Form a canonical argument list for the generic.
  llvm::SmallVector<SemIR::InstId> arg_ids;
  arg_ids.reserve(args.size());
  for (auto arg_id : args) {
    arg_ids.push_back(context.constant_values().GetConstantInstId(arg_id));
  }
  auto args_id = context.inst_blocks().AddCanonical(arg_ids);

  // Build a corresponding instance.
  // TODO: This could be made more efficient. We don't need to perform
  // substitution here; we know we want identity mappings for all constants and
  // types. We could also consider not storing the mapping at all in this case.
  return MakeGenericInstance(context, generic_id, args_id);
}

auto ResolveGenericInstance(Context& context,
                            SemIR::GenericInstanceId instance_id) -> bool {
  auto& instance = context.generic_instances().Get(instance_id);
  auto generic_id = instance.generic_id;

  // TODO: Remove this once we import generics properly.
  if (!generic_id.is_valid()) {
    return true;
  }

  if (!instance.definition.substituted_types_id.is_valid()) {
    // Perform substitution into the definition.
    auto substitutions = context.inst_blocks().Get(instance.args_id);
    auto& generic = context.generics().Get(generic_id);
    if (!generic.definition.substituted_type_insts_id.is_valid()) {
      // The generic is not defined yet.
      return false;
    }
    SubstituteRegion(context, generic.definition, instance.definition,
                     substitutions);
  }
  return true;
}

}  // namespace Carbon::Check
