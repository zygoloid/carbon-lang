// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "toolchain/check/generic_region_stack.h"

namespace Carbon::Check {

auto GenericRegionStack::Push() -> void {
  regions_.push_back({.first_symbolic_constant_index = static_cast<int32_t>(
                          symbolic_constant_inst_ids_.size())});
}

auto GenericRegionStack::PopGeneric() -> SemIR::Generic::Region {
  auto region = regions_.back();
  auto symbolic_constant_insts_id = inst_block_store_->Add(
      llvm::ArrayRef(symbolic_constant_inst_ids_)
          .drop_front(region.first_symbolic_constant_index));
  PopAndDiscard();
  return {.symbolic_constant_insts_id = symbolic_constant_insts_id};
}

auto GenericRegionStack::PopAndDiscard() -> void {
  auto region = regions_.pop_back_val();
  symbolic_constant_inst_ids_.truncate(region.first_symbolic_constant_index);
}

auto GenericRegionStack::PopNotGeneric() -> void {
  auto region = regions_.pop_back_val();
  CARBON_CHECK(region.first_symbolic_constant_index ==
               static_cast<int32_t>(symbolic_constant_inst_ids_.size()))
      << "Symbolic constant used in non-generic region. Have "
      << symbolic_constant_inst_ids_.size() << " symbolic constants, expected "
      << region.first_symbolic_constant_index;
}

}  // namespace Carbon::Check
