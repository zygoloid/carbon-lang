// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "toolchain/sem_ir/dominance.h"

#include "common/check.h"
#include "common/error.h"
#include "common/map.h"
#include "common/ostream.h"
#include "common/set.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/GraphTraits.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/iterator.h"
#include "llvm/Support/GenericDomTree.h"
#include "llvm/Support/GenericDomTreeConstruction.h"
#include "toolchain/sem_ir/file.h"
#include "toolchain/sem_ir/function.h"
#include "toolchain/sem_ir/generic.h"
#include "toolchain/sem_ir/id_kind.h"
#include "toolchain/sem_ir/ids.h"
#include "toolchain/sem_ir/inst.h"
#include "toolchain/sem_ir/typed_insts.h"

namespace Carbon::SemIR {
namespace {

class Cfg;

// Returns the block that `inst` transfers control to, or `InstBlockId::None` if
// `inst` is not a branch.
auto GetBranchTargetId(Inst inst) -> InstBlockId {
  if (auto branch = inst.TryAs<Branch>()) {
    return branch->target_id;
  }
  if (auto branch_if = inst.TryAs<BranchIf>()) {
    return branch_if->target_id;
  }
  if (auto branch_with_arg = inst.TryAs<BranchWithArg>()) {
    return branch_with_arg->target_id;
  }
  return InstBlockId::None;
}

// A node in the control flow graph of a function body: one of the body's
// blocks, along with the edges between it and the other blocks.
class CfgBlock {
 public:
  explicit CfgBlock(Cfg* cfg, int index, InstBlockId inst_block_id)
      : cfg_(cfg), index_(index), inst_block_id_(inst_block_id) {}

  // Adds an edge from this block to `successor`, if there isn't one already.
  // Duplicate edges are omitted because a block can branch to the same target
  // more than once, and the dominator tree doesn't benefit from seeing that.
  auto AddSuccessor(CfgBlock* successor) -> void {
    if (llvm::is_contained(successors_, successor)) {
      return;
    }
    successors_.push_back(successor);
    successor->predecessors_.push_back(this);
  }

  // Prints this block's name. This is required by LLVM's dominator tree, which
  // names blocks this way in its debug output.
  auto printAsOperand(llvm::raw_ostream& out, bool /*print_type*/) const
      -> void {
    out << inst_block_id_;
  }

  auto cfg() const -> Cfg* { return cfg_; }
  auto index() const -> int { return index_; }
  auto inst_block_id() const -> InstBlockId { return inst_block_id_; }
  auto successors() const -> llvm::ArrayRef<CfgBlock*> { return successors_; }
  auto predecessors() const -> llvm::ArrayRef<CfgBlock*> {
    return predecessors_;
  }

 private:
  Cfg* cfg_;
  int index_;
  InstBlockId inst_block_id_;

  llvm::SmallVector<CfgBlock*, 2> successors_;
  llvm::SmallVector<CfgBlock*, 2> predecessors_;
};

// The control flow graph of a function body. This is the graph that the
// dominator tree is built over; see the `llvm::GraphTraits` specializations
// below for how LLVM walks it.
class Cfg {
 public:
  // Builds the graph for a function body consisting of `body_block_ids`, which
  // must not be empty.
  auto Build(const File& file, llvm::ArrayRef<InstBlockId> body_block_ids)
      -> ErrorOr<Success>;

  // Execution of a function body starts in its first block.
  auto entry_block() -> CfgBlock* { return &blocks_.front(); }

  auto blocks() -> llvm::MutableArrayRef<CfgBlock> { return blocks_; }
  auto blocks() const -> llvm::ArrayRef<CfgBlock> { return blocks_; }
  auto size() const -> int { return blocks_.size(); }

 private:
  llvm::SmallVector<CfgBlock> blocks_;
};

auto Cfg::Build(const File& file, llvm::ArrayRef<InstBlockId> body_block_ids)
    -> ErrorOr<Success> {
  CARBON_CHECK(!body_block_ids.empty());

  // The blocks are created up front so that their addresses, which the edges
  // below refer to, don't change as more blocks are added.
  Map<InstBlockId, CfgBlock*> blocks_by_id;
  blocks_.reserve(body_block_ids.size());
  for (int i = 0; i != static_cast<int>(body_block_ids.size()); ++i) {
    blocks_.emplace_back(this, i, body_block_ids[i]);
    blocks_by_id.Insert(body_block_ids[i], &blocks_.back());
  }

  for (CfgBlock& from : blocks_) {
    for (InstId inst_id : file.inst_blocks().Get(from.inst_block_id())) {
      InstBlockId target_id = GetBranchTargetId(file.insts().Get(inst_id));
      if (!target_id.has_value()) {
        continue;
      }
      CfgBlock** to = blocks_by_id[target_id];
      if (!to) {
        return ErrorBuilder() << "Branch in block " << from.inst_block_id()
                              << " targets block " << target_id
                              << " which is not in function body";
      }
      from.AddSuccessor(*to);
    }
  }
  return Success();
}

}  // namespace
}  // namespace Carbon::SemIR

// Teaches LLVM's dominator tree how to walk a `Cfg`. Only the operations that
// building a forward dominator tree needs are provided.
namespace llvm {

template <>
struct GraphTraits<Carbon::SemIR::CfgBlock*> {
  using NodeRef = Carbon::SemIR::CfgBlock*;
  using ChildIteratorType = ArrayRef<NodeRef>::iterator;

  static auto getEntryNode(NodeRef block) -> NodeRef { return block; }
  static auto child_begin(NodeRef block) -> ChildIteratorType {
    return block->successors().begin();
  }
  static auto child_end(NodeRef block) -> ChildIteratorType {
    return block->successors().end();
  }
  static auto getNumber(const Carbon::SemIR::CfgBlock* block) -> unsigned {
    return block->index();
  }
};

// The dominator tree indexes its nodes through the traits for a pointer to
// const, so it needs `getNumber`, but it never walks the graph through them.
template <>
struct GraphTraits<const Carbon::SemIR::CfgBlock*> {
  using NodeRef = const Carbon::SemIR::CfgBlock*;

  static auto getNumber(NodeRef block) -> unsigned { return block->index(); }
};

// Building the dominator tree walks the graph backwards, from each block to its
// predecessors.
template <>
struct GraphTraits<Inverse<Carbon::SemIR::CfgBlock*>> {
  using NodeRef = Carbon::SemIR::CfgBlock*;
  using ChildIteratorType = ArrayRef<NodeRef>::iterator;

  static auto getEntryNode(Inverse<NodeRef> graph) -> NodeRef {
    return graph.Graph;
  }
  static auto child_begin(NodeRef block) -> ChildIteratorType {
    return block->predecessors().begin();
  }
  static auto child_end(NodeRef block) -> ChildIteratorType {
    return block->predecessors().end();
  }
  static auto getNumber(const Carbon::SemIR::CfgBlock* block) -> unsigned {
    return block->index();
  }
};

template <>
struct GraphTraits<Carbon::SemIR::Cfg*>
    : public GraphTraits<Carbon::SemIR::CfgBlock*> {
  using nodes_iterator = pointer_iterator<Carbon::SemIR::CfgBlock*>;

  static auto getEntryNode(Carbon::SemIR::Cfg* cfg) -> NodeRef {
    return cfg->entry_block();
  }

  static auto nodes_begin(Carbon::SemIR::Cfg* cfg) -> nodes_iterator {
    return nodes_iterator(cfg->blocks().begin());
  }
  static auto nodes_end(Carbon::SemIR::Cfg* cfg) -> nodes_iterator {
    return nodes_iterator(cfg->blocks().end());
  }
  static auto size(Carbon::SemIR::Cfg* cfg) -> size_t { return cfg->size(); }

  // A block's number is its position in the graph, so numbers are less than the
  // number of blocks, and never change once the graph is built.
  static auto getMaxNumber(Carbon::SemIR::Cfg* cfg) -> unsigned {
    return cfg->size();
  }
  static auto getNumberEpoch(Carbon::SemIR::Cfg* /*cfg*/) -> unsigned {
    return 0;
  }
};

template <>
struct DomTreeNodeTraits<Carbon::SemIR::CfgBlock> {
  using NodeType = Carbon::SemIR::CfgBlock;
  using NodePtr = Carbon::SemIR::CfgBlock*;
  using ParentPtr = Carbon::SemIR::Cfg*;
  using ParentType = Carbon::SemIR::Cfg;

  static auto getEntryNode(ParentPtr cfg) -> NodePtr {
    return cfg->entry_block();
  }
  static auto getParent(NodePtr block) -> ParentPtr { return block->cfg(); }
};

}  // namespace llvm

namespace Carbon::SemIR {
namespace {

// A node of the dominator tree of a function body, corresponding to one block.
using DomTreeNode = llvm::DomTreeNodeBase<CfgBlock>;

// The maximum depth to which we look through spliced instructions. This is a
// safeguard against malformed IR in which spliced instructions form a cycle;
// well-formed IR nests far more shallowly than this.
constexpr int MaxSpliceDepth = 50;

// A step in the walk over a function's dominator tree.
struct WalkStep {
  // Returns a step that verifies the instructions in `node`'s block and queues
  // up the blocks it dominates.
  static auto EnterBlock(const DomTreeNode* node) -> WalkStep {
    return {.node = node, .scope_start = 0};
  }

  // Returns a step that leaves the scope of a block that was entered when the
  // list of evaluated instructions had size `scope_start`.
  static auto LeaveBlock(int scope_start) -> WalkStep {
    return {.node = nullptr, .scope_start = scope_start};
  }

  // The block to enter, or null if this step leaves a block instead.
  const DomTreeNode* node;
  // The number of instructions that had been evaluated when the block being
  // left was entered. Only used when leaving a block.
  int scope_start;
};

// Collects the instructions that are referenced from function bodies despite
// being evaluated at file scope, or not evaluated at all, into `decl_insts`.
//
// TODO: These are pre-existing dominance violations, not genuine exemptions:
//
// -   File-scope instructions are evaluated in `__global_init`, if at all, so
//     they don't dominate uses in another function.
// -   A `let` in a class body produces a `wrapper_binding` that isn't evaluated
//     in any function, but a qualified name reference to it can appear in one.
//     For example, from the `public_global_access` case in
//     `check/testdata/class/access/access_modifiers.carbon`:
//
//         class A { let x: i32 = 5; }
//         let x: i32 = A.x;
//
//     Here the `name_ref` for `A.x` is in `__global_init`, and the
//     `wrapper_binding` for `x` is only in `A`'s body block.
//
// Lowering only works today because such uses either happen to be constant or
// are never lowered. Remove this allowlist and diagnose the uses instead once
// global initialization semantics and the member reference model are settled.
auto CollectDeclInsts(const File& file, Set<InstId>& decl_insts) -> void {
  auto add_block = [&](InstBlockId block_id) {
    if (block_id.has_value()) {
      for (InstId inst_id : file.inst_blocks().Get(block_id)) {
        decl_insts.Insert(inst_id);
      }
    }
  };

  add_block(file.top_inst_block_id());
  for (const auto& class_info : file.classes().values()) {
    add_block(class_info.body_block_id);
  }
}

// Verifies that every operand of every instruction in one function body is
// dominated by an evaluation of that operand.
//
// This walks the dominator tree of the function's control flow graph, tracking
// the set of instructions whose evaluations dominate the point currently being
// checked. An instruction joins that set when its evaluation is reached, and
// leaves it again when the walk leaves the blocks that the evaluation
// dominates.
class DominanceVerifier {
 public:
  explicit DominanceVerifier(const File& file, const Set<InstId>& decl_insts,
                             const Function& function, SpecificId specific_id)
      : file_(file),
        decl_insts_(decl_insts),
        function_(function),
        specific_id_(specific_id) {}

  auto Verify() -> ErrorOr<Success>;

 private:
  // Diagnoses blocks that are unreachable from the entry block, and so are not
  // in the dominator tree.
  auto CheckAllBlocksReachable() -> ErrorOr<Success>;

  // Verifies every block, walking the dominator tree from the entry block.
  auto VerifyBlocks() -> ErrorOr<Success>;

  // Verifies the operands of `inst_id`, then records `inst_id`, along with any
  // instructions it splices into the enclosing block, as evaluated.
  auto VerifyAndRecordInst(InstId inst_id, InstBlockId block_id, int depth)
      -> ErrorOr<Success>;

  // Verifies a single operand of `user_id`.
  auto VerifyArg(InstId user_id, IdAndKind arg, InstBlockId block_id)
      -> ErrorOr<Success>;

  // Verifies that `operand_id`, used by `user_id` in `block_id`, is either
  // constant or dominated by an evaluation.
  auto VerifyOperand(InstId user_id, InstId operand_id, InstBlockId block_id)
      -> ErrorOr<Success>;

  // Records that `inst_id` is evaluated at the point currently being verified.
  auto RecordEvaluated(InstId inst_id) -> void;

  // Records that every instruction in `block_id`, if it has a value, is
  // evaluated at the point currently being verified.
  auto RecordEvaluatedBlock(InstBlockId block_id) -> void;

  // Returns the instruction that `splice` splices in, or `InstId::None` if that
  // can't be determined.
  auto GetSplicedInstId(SpliceInst splice) const -> InstId;

  const File& file_;
  const Set<InstId>& decl_insts_;
  const Function& function_;
  SpecificId specific_id_;

  // The control flow graph of the function body, and its dominator tree.
  Cfg cfg_;
  llvm::DomTreeBase<CfgBlock> dom_tree_;

  // The instructions whose evaluations dominate the point currently being
  // verified, and the order in which they were added, so that they can be
  // removed again when leaving a block.
  Set<InstId> evaluated_;
  llvm::SmallVector<InstId> evaluated_order_;
};

auto DominanceVerifier::Verify() -> ErrorOr<Success> {
  CARBON_RETURN_IF_ERROR(cfg_.Build(file_, function_.body_block_ids));
  dom_tree_.recalculate(cfg_);
  CARBON_RETURN_IF_ERROR(CheckAllBlocksReachable());

  // Parameters and other instructions from the function declaration are
  // evaluated before the body begins, so they dominate the whole body.
  RecordEvaluatedBlock(function_.call_params_id);
  RecordEvaluatedBlock(function_.call_param_patterns_id);
  RecordEvaluatedBlock(function_.call_param_default_values_id);
  RecordEvaluatedBlock(function_.pattern_block_id);
  RecordEvaluated(function_.self_param_id);
  RecordEvaluated(function_.return_form_inst_id);
  RecordEvaluated(function_.return_pattern_id);
  for (InstId decl_id :
       {function_.definition_id, function_.first_owning_decl_id,
        function_.non_owning_decl_id}) {
    if (!decl_id.has_value()) {
      continue;
    }
    if (auto decl = file_.insts().TryGetAs<FunctionDecl>(decl_id)) {
      RecordEvaluatedBlock(decl->decl_block_id);
    }
  }

  return VerifyBlocks();
}

auto DominanceVerifier::CheckAllBlocksReachable() -> ErrorOr<Success> {
  for (const CfgBlock& block : cfg_.blocks()) {
    if (!dom_tree_.getNode(&block)) {
      return ErrorBuilder()
             << "Block " << block.inst_block_id() << " in function "
             << function_.name_id << " is unreachable from entry block";
    }
  }
  return Success();
}

auto DominanceVerifier::VerifyBlocks() -> ErrorOr<Success> {
  llvm::SmallVector<WalkStep> worklist = {
      WalkStep::EnterBlock(dom_tree_.getRootNode())};

  while (!worklist.empty()) {
    auto [node, scope_start] = worklist.pop_back_val();

    if (!node) {
      for (InstId inst_id : llvm::drop_begin(evaluated_order_, scope_start)) {
        evaluated_.Erase(inst_id);
      }
      evaluated_order_.truncate(scope_start);
      continue;
    }

    // Evaluations in this block dominate the rest of this block and the blocks
    // below it in the dominator tree, but nothing else. This step is beneath
    // this block's children on the worklist, so it runs once they're done.
    worklist.push_back(WalkStep::LeaveBlock(evaluated_order_.size()));

    InstBlockId block_id = node->getBlock()->inst_block_id();
    for (InstId inst_id : file_.inst_blocks().Get(block_id)) {
      CARBON_RETURN_IF_ERROR(
          VerifyAndRecordInst(inst_id, block_id, /*depth=*/0));
    }
    for (const DomTreeNode* child : *node) {
      worklist.push_back(WalkStep::EnterBlock(child));
    }
  }
  return Success();
}

auto DominanceVerifier::VerifyAndRecordInst(InstId inst_id,
                                            InstBlockId block_id, int depth)
    -> ErrorOr<Success> {
  if (depth > MaxSpliceDepth) {
    return ErrorBuilder() << "Spliced instructions are nested more than "
                          << MaxSpliceDepth << " deep at instruction "
                          << inst_id << " in function " << function_.name_id;
  }

  Inst inst = file_.insts().Get(inst_id);

  // A `SpliceBlock` evaluates the instructions in its block, and then produces
  // the value of one of them.
  if (auto splice_block = inst.TryAs<SpliceBlock>()) {
    if (splice_block->block_id.has_value()) {
      for (InstId spliced_id :
           file_.inst_blocks().Get(splice_block->block_id)) {
        CARBON_RETURN_IF_ERROR(
            VerifyAndRecordInst(spliced_id, block_id, depth + 1));
      }
    }
    CARBON_RETURN_IF_ERROR(
        VerifyOperand(inst_id, splice_block->result_id, block_id));
    RecordEvaluated(inst_id);
    return Success();
  }

  // A `SpliceInst` evaluates the instruction that its operand names.
  if (auto splice = inst.TryAs<SpliceInst>()) {
    CARBON_RETURN_IF_ERROR(VerifyOperand(inst_id, splice->inst_id, block_id));
    RecordEvaluated(inst_id);
    if (InstId spliced_id = GetSplicedInstId(*splice); spliced_id.has_value()) {
      CARBON_RETURN_IF_ERROR(
          VerifyAndRecordInst(spliced_id, block_id, depth + 1));
    }
    return Success();
  }

  CARBON_RETURN_IF_ERROR(VerifyArg(inst_id, inst.arg0_and_kind(), block_id));
  CARBON_RETURN_IF_ERROR(VerifyArg(inst_id, inst.arg1_and_kind(), block_id));
  RecordEvaluated(inst_id);
  return Success();
}

auto DominanceVerifier::VerifyArg(InstId user_id, IdAndKind arg,
                                  InstBlockId block_id) -> ErrorOr<Success> {
  // These operand kinds name the value produced by another instruction, so that
  // instruction's evaluation must dominate this use.
  if (arg.kind() == IdKind::For<InstId>) {
    return VerifyOperand(user_id, arg.As<InstId>(), block_id);
  }
  if (arg.kind() == IdKind::For<DestInstId>) {
    return VerifyOperand(user_id, arg.As<DestInstId>(), block_id);
  }
  // A `TypeInstId` always names a constant of type `type`, so this check should
  // always pass, but checking it means we notice if that stops being true.
  if (arg.kind() == IdKind::For<TypeInstId>) {
    return VerifyOperand(user_id, arg.As<TypeInstId>(), block_id);
  }
  if (arg.kind() == IdKind::For<InstBlockId>) {
    InstBlockId operands_id = arg.As<InstBlockId>();
    if (operands_id.has_value()) {
      for (InstId operand_id : file_.inst_blocks().Get(operands_id)) {
        CARBON_RETURN_IF_ERROR(VerifyOperand(user_id, operand_id, block_id));
      }
    }
    return Success();
  }

  // Every other operand kind either doesn't name an instruction at all, or
  // names one in a way that doesn't require dominance:
  //
  // -   `MetaInstId` and `MetaInstBlockId` name the identity of an instruction
  //     rather than its value.
  // -   `AbsoluteInstId` and `AbsoluteInstBlockId` name instructions that are
  //     typically in a different entity.
  // -   `LabelId` names another block in this function's control flow.
  // -   `DeclInstBlockId` names a declaration rather than a computation.
  return Success();
}

auto DominanceVerifier::VerifyOperand(InstId user_id, InstId operand_id,
                                      InstBlockId block_id)
    -> ErrorOr<Success> {
  if (!operand_id.has_value() || operand_id == ErrorInst::InstId) {
    return Success();
  }
  // A constant isn't evaluated in the function body, so can be used anywhere.
  //
  // TODO: Use `GetConstantValueInSpecific` here, so that an instruction that is
  // only constant in this specific is also exempt. That currently crashes,
  // because a function body can name an instruction whose constant value is
  // attached to an enclosing generic rather than to this function's generic,
  // which `GetConstantInSpecific` rejects. Lowering should hit the same crash;
  // see `FunctionContext::LowerInst`.
  if (file_.constant_values().Get(operand_id).is_constant()) {
    return Success();
  }
  if (evaluated_.Contains(operand_id)) {
    return Success();
  }
  // TODO: Remove this allowlist; see `CollectDeclInsts`.
  if (decl_insts_.Contains(operand_id)) {
    return Success();
  }
  Inst operand = file_.insts().Get(operand_id);

  // TODO: A non-constant import isn't evaluated in the importing file at all,
  // so this is a real violation: `__global_init` can contain a `name_ref` to an
  // `import_ref` for a non-constant imported variable. Remove this exemption
  // and diagnose such uses once imported variables have a value model.
  if (operand.Is<ImportRefLoaded>() || operand.Is<ImportRefUnloaded>()) {
    return Success();
  }

  return ErrorBuilder()
         << "Operand " << operand_id << " (" << operand.kind().ir_name()
         << ") used by instruction " << user_id << " ("
         << file_.insts().Get(user_id).kind().ir_name() << ") in block "
         << block_id << " of function " << function_.name_id
         << " is not dominated by any evaluation and is not constant";
}

auto DominanceVerifier::RecordEvaluated(InstId inst_id) -> void {
  if (!inst_id.has_value()) {
    return;
  }
  // An instruction can be evaluated more than once, for example in two blocks
  // that don't dominate each other. Only the first evaluation in scope needs to
  // be undone.
  if (evaluated_.Insert(inst_id).is_inserted()) {
    evaluated_order_.push_back(inst_id);
  }
}

auto DominanceVerifier::RecordEvaluatedBlock(InstBlockId block_id) -> void {
  if (!block_id.has_value()) {
    return;
  }
  for (InstId inst_id : file_.inst_blocks().Get(block_id)) {
    RecordEvaluated(inst_id);
  }
}

auto DominanceVerifier::GetSplicedInstId(SpliceInst splice) const -> InstId {
  ConstantId const_id =
      GetConstantValueInSpecific(file_, specific_id_, splice.inst_id);
  if (!const_id.is_constant()) {
    return InstId::None;
  }
  InstId const_inst_id = file_.constant_values().GetInstIdIfValid(const_id);
  if (!const_inst_id.has_value()) {
    return InstId::None;
  }
  if (auto inst_value = file_.insts().TryGetAs<InstValue>(const_inst_id)) {
    return inst_value->inst_id;
  }
  return InstId::None;
}

}  // namespace

auto VerifyDominance(const File& file) -> ErrorOr<Success> {
  // Invariants don't necessarily hold for invalid IR.
  if (file.has_errors()) {
    return Success();
  }

  Set<InstId> decl_insts;
  CollectDeclInsts(file, decl_insts);

  for (const Function& function : file.functions().values()) {
    if (function.body_block_ids.empty()) {
      continue;
    }

    // Verify the body in the general, unspecialized context.
    CARBON_RETURN_IF_ERROR(
        DominanceVerifier(file, decl_insts, function, SpecificId::None)
            .Verify());

    // For a generic function, also verify the body as it will be evaluated in
    // each of its specifics, in which spliced instructions can be resolved.
    if (!function.generic_id.has_value()) {
      continue;
    }
    for (const auto& [specific_id, specific] : file.specifics().enumerate()) {
      if (specific.generic_id == function.generic_id &&
          !specific.IsUnresolved() && !specific.HasError()) {
        CARBON_RETURN_IF_ERROR(
            DominanceVerifier(file, decl_insts, function, specific_id)
                .Verify());
      }
    }
  }

  return Success();
}

}  // namespace Carbon::SemIR
