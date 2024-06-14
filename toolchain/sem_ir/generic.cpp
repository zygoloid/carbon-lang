// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "toolchain/sem_ir/file.h"

namespace Carbon::SemIR {

auto GenericInstanceStore::GetOrAdd(GenericId generic_id, InstBlockId args_id)
    -> GenericInstanceId {
  return lookup_table_
      .Insert(
          Key{.generic_id = generic_id, .args_id = args_id},
          [&](Key /*key*/, void* storage) {
            new (storage) GenericInstanceId(generic_instances_.Add(
                {.generic_id = generic_id, .args_id = args_id}));
          },
          KeyContext{.instances = generic_instances_.array_ref()})
      .key();
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
