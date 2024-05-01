// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef CARBON_TOOLCHAIN_CHECK_GENERIC_REGION_STACK_H_
#define CARBON_TOOLCHAIN_CHECK_GENERIC_REGION_STACK_H_

#include "toolchain/sem_ir/generic.h"
#include "toolchain/sem_ir/ids.h"
#include "toolchain/sem_ir/inst.h"

namespace Carbon::Check {

// A stack of enclosing regions that might be declaring or defining a generic
// entity. In such a region, we track the generic constructs that are used, such
// as symbolic constants and types, and instructions that depend on a template
// parameter.
// TODO: For now we're just tracking symbolic constants.
class GenericRegionStack {
 public:
  explicit GenericRegionStack(SemIR::InstBlockStore& inst_block_store)
      : inst_block_store_(&inst_block_store) {}

  // Pushes a region that might be declaring or defining a generic.
  auto Push() -> void;

  // Pops a generic region.
  auto PopGeneric() -> SemIR::Generic::Region;

  // Pops a region and discards the result. This is used when the region is a
  // redeclaration of a previous region, and so we don't need a representation
  // of it.
  auto PopAndDiscard() -> void;

  // Pops a region that is not generic.
  auto PopNotGeneric() -> void;

  // Adds a symbolic constant to the list of symbolic constants used in the
  // current region.
  auto AddSymbolicConstant(SemIR::ConstantId constant_id) -> void {
    CARBON_CHECK(!regions_.empty())
        << "Formed a symbolic constant while not in a generic region.";
    CARBON_CHECK(constant_id.is_symbolic()) << "Expected a symbolic constant.";
    symbolic_constant_ids_.push_back(constant_id.inst_id());
  }

 private:
  struct RegionInfo {
    int32_t first_symbolic_constant_id;
  };

  // Storage for instruction blocks.
  SemIR::InstBlockStore* inst_block_store_;

  // The current set of enclosing generic regions.
  llvm::SmallVector<RegionInfo> regions_;

  // List of symbolic constants used in any of the enclosing generic regions.
  llvm::SmallVector<SemIR::InstId> symbolic_constant_ids_;
};

}  // namespace Carbon::Check

#endif  // CARBON_TOOLCHAIN_CHECK_GENERIC_REGION_STACK_H_
