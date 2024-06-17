// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef CARBON_TOOLCHAIN_SEM_IR_GENERIC_H_
#define CARBON_TOOLCHAIN_SEM_IR_GENERIC_H_

#include "common/set.h"
#include "toolchain/sem_ir/ids.h"
#include "toolchain/sem_ir/inst.h"

namespace Carbon::SemIR {

// Information for a generic entity, such as a generic class, a generic
// interface, or generic function.
struct Generic : public Printable<Generic> {
  // A portion of the generic corresponding to either the declaration or the
  // definition. These are tracked separately because they're built and resolved
  // at different times.
  struct Region {
    // A block containing instructions that are used in this fragment and have
    // symbolic constant values.
    InstBlockId symbolic_constant_insts_id = InstBlockId::Invalid;
    // A block containing instructions that are used in this fragment and have
    // symbolic types that have been replaced with substituted types.
    InstBlockId substituted_type_insts_id = InstBlockId::Invalid;
    // TODO: Also track:
    // - Types required to be complete in this generic?
    //   Perhaps instead tracked as part of constraints on the generic.
    // - Template-dependent instructions in this generic.
  };

  auto Print(llvm::raw_ostream& out) const -> void {
    out << "{decl: " << decl_id << ", bindings: " << bindings_id << "}";
  }

  // The following members always have values, and do not change throughout the
  // lifetime of the generic.

  // The first declaration of the generic entity.
  InstId decl_id;
  // A block containing the IDs of compile time bindings in this generic scope.
  // The index in this block will match the `bind_index` of the instruction.
  InstBlockId bindings_id;

  // The following members are accumulated when the region is completed.

  // The region of the generic corresponding to the declaration of the entity.
  Region decl;
  // The region of the generic corresponding to the definition of the entity.
  Region definition;
};

// An instance of a generic entity, such as an instance of a generic function.
// For each construct that depends on a compile-time parameter in the generic
// entity, this contains the corresponding non-generic value. This includes
// values for the compile-time parameters themselves.
struct GenericInstance : Printable<GenericInstance> {
  // Values corresponding to a region of a generic.
  struct Region {
    InstBlockId symbolic_constant_values_id = InstBlockId::Invalid;
    TypeBlockId substituted_types_id = TypeBlockId::Invalid;
  };

  auto Print(llvm::raw_ostream& out) const -> void {
    out << "{generic: " << generic_id << ", args: " << args_id << "}";
  }

  // The generic that this is an instance of.
  GenericId generic_id;
  // Argument values, corresponding to the bindings in `Generic::bindings_id`.
  InstBlockId args_id;

  // The following members are accumulated when the region is completed.

  // Values used in the declaration of the generic instance.
  Region decl;
  // Values used in the definition of the generic instance.
  Region definition;
};

// Provides storage for deduplicated instances of generics.
class GenericInstanceStore {
 public:
  // Adds a new generic instance, or gets the existing generic instance for a
  // specified generic and argument list. Returns the ID of the generic
  // instance. The argument IDs must be for instructions in the constant block,
  // and must be a canonical instruction block ID.
  auto GetOrAdd(GenericId generic_id, InstBlockId args_id) -> GenericInstanceId;

  // Gets the specified generic instance.
  auto Get(GenericInstanceId instance_id) const -> const GenericInstance& {
    return generic_instances_.Get(instance_id);
  }

  // Gets the specified generic instance.
  auto Get(GenericInstanceId instance_id) -> GenericInstance& {
    return generic_instances_.Get(instance_id);
  }

 private:
  // A lookup key for a generic instance.
  struct Key {
    GenericId generic_id;
    InstBlockId args_id;

    friend auto operator==(const Key&, const Key&) -> bool = default;
  };

  // Context for hashing keys.
  struct KeyContext {
    llvm::ArrayRef<GenericInstance> instances;

    auto AsKey(GenericInstanceId id) const -> Key {
      const auto& instance = instances[id.index];
      return {.generic_id = instance.generic_id, .args_id = instance.args_id};
    }
    static auto AsKey(Key key) -> Key { return key; }

    template <typename KeyT>
    auto HashKey(KeyT key, uint64_t seed) const -> HashCode {
      return HashValue(AsKey(key), seed);
    }

    template <typename LHSKeyT, typename RHSKeyT>
    auto KeyEq(const LHSKeyT& lhs_key, const RHSKeyT& rhs_key) const -> bool {
      return AsKey(lhs_key) == AsKey(rhs_key);
    }
  };

  ValueStore<GenericInstanceId> generic_instances_;
  Carbon::Set<GenericInstanceId, 0, KeyContext> lookup_table_;
};

// Gets the instance of a substituted type within a specified generic
// instance. Note that this does not perform substitution, and will return
// `Invalid` if the substituted type is not yet known.
auto GetTypeInstance(const File& file, GenericInstanceId instance_id,
                     TypeId type_id) -> TypeId;

}  // namespace Carbon::SemIR

#endif  // CARBON_TOOLCHAIN_SEM_IR_GENERIC_H_