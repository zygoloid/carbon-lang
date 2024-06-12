// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "toolchain/sem_ir/constant.h"

#include "toolchain/sem_ir/file.h"

namespace Carbon::SemIR {

// Profile a generic instance.
static auto ProfileGenericInstance(
    llvm::FoldingSetNodeID& id, GenericId generic_id,
    llvm::ArrayRef<InstId> arg_ids) -> void {
  id.AddInteger(generic_id.index);
  for (auto arg_id : arg_ids) {
    id.AddInteger(arg_id.index);
  }
}

// Profile a GenericInstance that has already been created.
auto GenericInstanceStore::Node::Profile(llvm::FoldingSetNodeID& id,
                                         GenericInstanceStore* store) -> void {
  auto& generic = store->generic_instances_.Get(generic_instance_id);
  ProfileGenericInstance(id, generic.generic_id,
                         store->inst_block_store_->Get(generic.args_id));
}

auto GenericInstanceStore::GetOrAdd(
    GenericId generic_id, llvm::ArrayRef<InstId> arg_ids) -> GenericInstanceId {
  // Compute the generic instance's profile.
  llvm::FoldingSetNodeID id;
  ProfileGenericInstance(id, generic_id, arg_ids);

  // Check if we have already created this generic instance.
  void* insert_pos;
  if (Node* found = lookup_table_.FindNodeOrInsertPos(id, insert_pos)) {
    return found->generic_instance_id;
  }

  // Create the new instance and insert the new node.
  llvm::SmallVector<InstId> arg_inst_ids;
  arg_inst_ids.reserve(arg_ids.size());
  for (auto arg_const_id : arg_ids) {
    arg_inst_ids.push_back(arg_const_id);
  }
  auto generic_instance_id =
      generic_instances_.Add({.generic_id = generic_id,
                              .args_id = inst_block_store_->Add(arg_inst_ids)});
  lookup_table_.InsertNode(new (*allocator_)
                               Node{.generic_instance_id = generic_instance_id},
                           insert_pos);
  return generic_instance_id;
}

// Gets the instance of a substituted type within a specified generic
// instance.
auto GetTypeInstance(const File& file, GenericInstanceId instance_id,
                     TypeId type_id) -> TypeId {
  if (!type_id.is_substituted()) {
    return type_id;
  }

  const auto& info = file.types().GetSubstitutedTypeInfo(type_id);
  if (!instance_id.is_valid()) {
    return info.pattern_id;
  }

  const auto& instance = file.generic_instances().Get(instance_id);
  CARBON_CHECK(instance.generic_id == info.generic_id)
      << "Given instance for wrong generic.";

  const auto& region = info.index >= 0 ? instance.decl : instance.definition;
  if (!region.substituted_types_id.is_valid()) {
    // TODO: Can we CHECK-fail here?
    return TypeId::Invalid;
  }
  auto types = file.type_blocks().Get(region.substituted_types_id);
  return types[info.index >= 0 ? info.index : -info.index - 1];
}

}  // namespace Carbon::SemIR
