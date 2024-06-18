// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "toolchain/check/generic_region_stack.h"

namespace Carbon::Check {

auto GenericRegionStack::Push() -> void {
  regions_.push_back({.first_symbolic_constant_index = static_cast<int32_t>(
                          symbolic_constant_inst_ids_.size()),
                      .first_symbolic_type_index = static_cast<int32_t>(
                          symbolic_type_inst_ids_.size())});
}

auto GenericRegionStack::PopGeneric() -> SemIR::Generic::Region {
  auto symbolic_constant_insts_id =
      inst_block_store_->Add(PeekSymbolicConstantInsts());
  auto symbolic_type_insts_id = inst_block_store_->Add(PeekSymbolicTypeInsts());
  PopAndDiscard();
  return {.symbolic_constant_insts_id = symbolic_constant_insts_id,
          .substituted_type_insts_id = symbolic_type_insts_id};
}

auto GenericRegionStack::PopAndDiscard() -> void {
  auto region = regions_.pop_back_val();
  symbolic_constant_inst_ids_.truncate(region.first_symbolic_constant_index);
  symbolic_type_inst_ids_.truncate(region.first_symbolic_type_index);
}

auto GenericRegionStack::PopNotGeneric() -> void {
  auto region = regions_.pop_back_val();
  CARBON_CHECK(region.first_symbolic_constant_index ==
               static_cast<int32_t>(symbolic_constant_inst_ids_.size()))
      << "Symbolic constant used in non-generic region. Have "
      << symbolic_constant_inst_ids_.size() << " symbolic constants, expected "
      << region.first_symbolic_constant_index;
  CARBON_CHECK(region.first_symbolic_type_index ==
               static_cast<int32_t>(symbolic_type_inst_ids_.size()))
      << "Symbolic type used in non-generic region. Have "
      << symbolic_type_inst_ids_.size() << " symbolic types, expected "
      << region.first_symbolic_type_index;
}

}  // namespace Carbon::Check
