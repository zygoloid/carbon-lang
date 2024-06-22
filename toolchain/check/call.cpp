// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "toolchain/check/call.h"

#include "toolchain/base/kind_switch.h"
#include "toolchain/check/context.h"
#include "toolchain/check/convert.h"
#include "toolchain/check/function.h"
#include "toolchain/check/generic.h"
#include "toolchain/check/subst.h"
#include "toolchain/sem_ir/generic.h"
#include "toolchain/sem_ir/ids.h"
#include "toolchain/sem_ir/inst.h"
#include "toolchain/sem_ir/typed_insts.h"

namespace Carbon::Check {

// Performs a call where the callee is the name of a generic class, such as
// `Vector(i32)`.
static auto PerformCallToGenericClass(Context& context, Parse::NodeId node_id,
                                      SemIR::InstId callee_id,
                                      SemIR::ClassId class_id,
                                      llvm::ArrayRef<SemIR::InstId> arg_ids)
    -> SemIR::InstId {
  auto& class_info = context.classes().Get(class_id);

  // Convert the arguments to match the parameters.
  auto converted_args_id = ConvertCallArgs(
      context, node_id, /*self_id=*/SemIR::InstId::Invalid, arg_ids,
      /*return_storage_id=*/SemIR::InstId::Invalid, class_info.decl_id,
      class_info.implicit_param_refs_id, class_info.param_refs_id);
  return context.AddInst<SemIR::Call>(node_id,
                                      {.type_id = SemIR::TypeId::TypeType,
                                       .callee_id = callee_id,
                                       .args_id = converted_args_id});
}

// Performs a call where the callee is the name of a generic interface, such as
// `AddWith(i32)`.
// TODO: Refactor with PerformCallToGenericClass.
static auto PerformCallToGenericInterface(Context& context,
                                          Parse::NodeId node_id,
                                          SemIR::InstId callee_id,
                                          SemIR::InterfaceId interface_id,
                                          llvm::ArrayRef<SemIR::InstId> arg_ids)
    -> SemIR::InstId {
  auto& interface_info = context.interfaces().Get(interface_id);

  // Convert the arguments to match the parameters.
  auto converted_args_id = ConvertCallArgs(
      context, node_id, /*self_id=*/SemIR::InstId::Invalid, arg_ids,
      /*return_storage_id=*/SemIR::InstId::Invalid, interface_info.decl_id,
      interface_info.implicit_param_refs_id, interface_info.param_refs_id);
  return context.AddInst<SemIR::Call>(node_id,
                                      {.type_id = SemIR::TypeId::TypeType,
                                       .callee_id = callee_id,
                                       .args_id = converted_args_id});
}

struct DeductionWorklist {
  auto Add(SemIR::InstId param, SemIR::InstId arg) -> void {
    deductions.push_back({.param = param, .arg = arg});
  }

  auto AddBlock(llvm::ArrayRef<SemIR::InstId> params,
                llvm::ArrayRef<SemIR::InstId> args) -> void {
    if (params.size() != args.size()) {
      // TODO: Issue a diagnostic.
      failed = true;
      return;
    }
    for (auto [param, arg] : llvm::zip_equal(params, args)) {
      Add(param, arg);
    }
  }

  auto AddBlock(SemIR::InstBlockId params,
                llvm::ArrayRef<SemIR::InstId> args) -> void {
    AddBlock(context.inst_blocks().Get(params), args);
  }

  auto AddBlock(SemIR::InstBlockId params, SemIR::InstBlockId args) -> void {
    AddBlock(context.inst_blocks().Get(params), context.inst_blocks().Get(args));
  }

  struct PendingDeduction {
    SemIR::InstId param;
    SemIR::InstId arg;
  };
  Context& context;
  llvm::SmallVector<PendingDeduction> deductions;
  bool failed = false;
};

static auto DeduceGenericCallArguments(
    Context& context, Parse::NodeId node_id, SemIR::InstId decl_id,
    SemIR::GenericId generic_id, SemIR::GenericInstanceId instance_id,
    SemIR::InstBlockId implicit_params_id, SemIR::InstBlockId params_id,
    SemIR::InstId self_id,
    llvm::ArrayRef<SemIR::InstId> arg_ids) -> SemIR::GenericInstanceId {
  DeductionWorklist worklist = {.context = context};

  // TODO: Diagnostics.
  static_cast<void>(node_id);
  static_cast<void>(decl_id);

  // TODO: Perform deduction for type of self
  static_cast<void>(implicit_params_id);
  static_cast<void>(self_id);

  worklist.AddBlock(params_id, arg_ids);

  // Copy any outer generic arguments from the specified instance.
  llvm::SmallVector<SemIR::InstId> results;
  if (instance_id.is_valid()) {
    auto args = context.inst_blocks().Get(
        context.generic_instances().Get(instance_id).args_id);
    results.assign(args.begin(), args.end());
  }
  auto first_deduced_index = SemIR::CompileTimeBindIndex(results.size());

  results.resize(context.inst_blocks()
                     .Get(context.generics().Get(generic_id).bindings_id)
                     .size(),
                 SemIR::InstId::Invalid);
  auto substitutions =
      llvm::ArrayRef(results).take_front(first_deduced_index.index);

  while (!worklist.deductions.empty() && !worklist.failed) {
    auto [param_id, arg_id] = worklist.deductions.pop_back_val();

    // If the parameter has a symbolic type, deduce against that.
    auto param_type_id = context.insts().Get(param_id).type_id();
    if (param_type_id.is_substituted()) {
      worklist.Add(
          context.types().GetInstId(param_type_id),
          context.types().GetInstId(context.insts().Get(arg_id).type_id()));
    }

    // If the parameter is a symbolic constant, deduce against it.
    auto param_const_id = context.constant_values().Get(param_id);
    if (!param_const_id.is_valid() || !param_const_id.is_symbolic()) {
      continue;
    }

    // Substitute in any parameters from an enclosing context.
    param_const_id = SubstConstant(context, param_const_id, substitutions);
    if (!param_const_id.is_valid() || !param_const_id.is_symbolic()) {
      continue;
    }

    CARBON_KIND_SWITCH(context.insts().Get(
        context.constant_values().GetInstId(param_const_id))) {
      case CARBON_KIND(SemIR::BindSymbolicName bind): {
        auto index = context.bind_names().Get(bind.bind_name_id).bind_index;
        if (index.is_valid() && index >= first_deduced_index) {
          CARBON_CHECK(static_cast<size_t>(index.index) < results.size())
              << "Deduced value for unexpected index " << index
              << "; expected to deduce " << results.size() << " arguments.";
          auto arg_const_inst_id =
              context.constant_values().GetConstantInstId(arg_id);
          if (arg_const_inst_id.is_valid()) {
            // TODO: Check for consistency.
            results[index.index] = arg_const_inst_id;
          }
        }
        break;
      }

      // TODO: Other cases.

      default:
        break;
    }
  }

  if (worklist.failed) {
    return SemIR::GenericInstanceId::Invalid;
  }

  // TODO: Check we deduced an argument value for every parameter.
  return MakeGenericInstance(context, generic_id,
                             context.inst_blocks().AddCanonical(results));
}

auto PerformCall(Context& context, Parse::NodeId node_id,
                 SemIR::InstId callee_id, llvm::ArrayRef<SemIR::InstId> arg_ids)
    -> SemIR::InstId {
  // Identify the function we're calling.
  auto callee_function = GetCalleeFunction(context.sem_ir(), callee_id);
  if (!callee_function.function_id.is_valid()) {
    auto type_inst =
        context.types().GetAsInst(context.insts().Get(callee_id).type_id());
    CARBON_KIND_SWITCH(type_inst) {
      case CARBON_KIND(SemIR::GenericClassType generic_class): {
        return PerformCallToGenericClass(context, node_id, callee_id,
                                         generic_class.class_id, arg_ids);
      }
      case CARBON_KIND(SemIR::GenericInterfaceType generic_interface): {
        return PerformCallToGenericInterface(context, node_id, callee_id,
                                             generic_interface.interface_id,
                                             arg_ids);
      }
      default: {
        if (!callee_function.is_error) {
          CARBON_DIAGNOSTIC(CallToNonCallable, Error,
                            "Value of type `{0}` is not callable.",
                            SemIR::TypeId);
          context.emitter().Emit(node_id, CallToNonCallable,
                                 context.insts().Get(callee_id).type_id());
        }
        return SemIR::InstId::BuiltinError;
      }
    }
  }
  auto& callable = context.functions().Get(callee_function.function_id);

  // Determine the generic arguments in the call.
  auto instance_id = SemIR::GenericInstanceId::Invalid;
  if (callable.generic_id.is_valid()) {
    instance_id = DeduceGenericCallArguments(
        context, node_id, callable.decl_id, callable.generic_id,
        callee_function.instance_id, callable.implicit_param_refs_id,
        callable.param_refs_id, callee_function.self_id, arg_ids);
    if (!instance_id.is_valid()) {
      return SemIR::InstId::BuiltinError;
    }
  }

  // For functions with an implicit return type, the return type is the empty
  // tuple type.
  SemIR::TypeId type_id =
      callable.declared_return_type(context.sem_ir(), instance_id);
  if (!type_id.is_valid()) {
    type_id = context.GetTupleType({});
  }

  // If there is a return slot, build storage for the result.
  SemIR::InstId return_storage_id = SemIR::InstId::Invalid;
  {
    DiagnosticAnnotationScope annotate_diagnostics(
        &context.emitter(), [&](auto& builder) {
          CARBON_DIAGNOSTIC(IncompleteReturnTypeHere, Note,
                            "Return type declared here.");
          builder.Note(callable.return_storage_id, IncompleteReturnTypeHere);
        });
    CheckFunctionReturnType(context, callee_id, callable);
  }
  switch (callable.return_slot) {
    case SemIR::Function::ReturnSlot::Present:
      // Tentatively put storage for a temporary in the function's return slot.
      // This will be replaced if necessary when we perform initialization.
      return_storage_id = context.AddInst<SemIR::TemporaryStorage>(
          node_id, {.type_id = type_id});
      break;
    case SemIR::Function::ReturnSlot::Absent:
      break;
    case SemIR::Function::ReturnSlot::Error:
      // Don't form an initializing expression with an incomplete type.
      type_id = SemIR::TypeId::Error;
      break;
    case SemIR::Function::ReturnSlot::NotComputed:
      CARBON_FATAL() << "Missing return slot category in call to " << callable;
  }

  // Convert the arguments to match the parameters.
  // TODO: Pass in the instance.
  auto converted_args_id =
      ConvertCallArgs(context, node_id, callee_function.self_id, arg_ids,
                      return_storage_id, callable.decl_id,
                      callable.implicit_param_refs_id, callable.param_refs_id);
  auto call_inst_id =
      context.AddInst<SemIR::Call>(node_id, {.type_id = type_id,
                                             .callee_id = callee_id,
                                             .args_id = converted_args_id});

  return call_inst_id;
}

}  // namespace Carbon::Check
