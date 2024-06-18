// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// AUTOUPDATE
// TIP: To test this file alone, run:
// TIP:   bazel test //toolchain/testing:file_test --test_arg=--file_tests=toolchain/check/testdata/class/generic/subst_into_decl.carbon
// TIP: To dump output, run:
// TIP:   bazel run //toolchain/testing:file_test -- --dump_output --file_tests=toolchain/check/testdata/class/generic/subst_into_decl.carbon

class Class(T:! type) {
  var x: T;
}

fn F(x: Class(i32)) -> i32 {
  return x.x;
}

// CHECK:STDOUT: --- subst_into_decl.carbon
// CHECK:STDOUT:
// CHECK:STDOUT: constants {
// CHECK:STDOUT:   %T: type = bind_symbolic_name T 0 [symbolic]
// CHECK:STDOUT:   %Class.type: type = generic_class_type @Class [template]
// CHECK:STDOUT:   %.1: type = tuple_type () [template]
// CHECK:STDOUT:   %Class.1: %Class.type = struct_value () [template]
// CHECK:STDOUT:   %Class.2: type = class_type @Class, (%T) [symbolic]
// CHECK:STDOUT:   %.2: type = unbound_element_type %Class.2, %T [symbolic]
// CHECK:STDOUT:   %.3: type = struct_type {.x: %T} [symbolic]
// CHECK:STDOUT:   %Int32.type: type = fn_type @Int32 [template]
// CHECK:STDOUT:   %Int32: %Int32.type = struct_value () [template]
// CHECK:STDOUT:   %Class.3: type = class_type @Class, (i32) [template]
// CHECK:STDOUT:   %F.type: type = fn_type @F [template]
// CHECK:STDOUT:   %F: %F.type = struct_value () [template]
// CHECK:STDOUT:   %.4: type = unbound_element_type %Class.3, i32 [template]
// CHECK:STDOUT:   %.5: type = ptr_type %.3 [symbolic]
// CHECK:STDOUT: }
// CHECK:STDOUT:
// CHECK:STDOUT: file {
// CHECK:STDOUT:   package: <namespace> = namespace [template] {
// CHECK:STDOUT:     .Core = %Core
// CHECK:STDOUT:     .Class = %Class.decl
// CHECK:STDOUT:     .F = %F.decl
// CHECK:STDOUT:   }
// CHECK:STDOUT:   %Core: <namespace> = namespace [template] {}
// CHECK:STDOUT:   %Class.decl: %Class.type = class_decl @Class [template = constants.%Class.1] {
// CHECK:STDOUT:     %T.loc11_13.1: type = param T
// CHECK:STDOUT:     %T.loc11_13.2: type = bind_symbolic_name T 0, %T.loc11_13.1 [symbolic = constants.%T]
// CHECK:STDOUT:   }
// CHECK:STDOUT:   %import_ref.1: %Int32.type = import_ref ir3, inst+3, loaded [template = constants.%Int32]
// CHECK:STDOUT:   %import_ref.2: %Int32.type = import_ref ir3, inst+3, loaded [template = constants.%Int32]
// CHECK:STDOUT:   %F.decl: %F.type = fn_decl @F [template = constants.%F] {
// CHECK:STDOUT:     %Class.ref: %Class.type = name_ref Class, %Class.decl [template = constants.%Class.1]
// CHECK:STDOUT:     %int.make_type_32.loc15_15: init type = call constants.%Int32() [template = i32]
// CHECK:STDOUT:     %.loc15_14.1: type = value_of_initializer %int.make_type_32.loc15_15 [template = i32]
// CHECK:STDOUT:     %.loc15_14.2: type = converted %int.make_type_32.loc15_15, %.loc15_14.1 [template = i32]
// CHECK:STDOUT:     %.loc15_14.3: init type = call %Class.ref(%.loc15_14.2) [template = constants.%Class.3]
// CHECK:STDOUT:     %.loc15_18.1: type = value_of_initializer %.loc15_14.3 [template = constants.%Class.3]
// CHECK:STDOUT:     %.loc15_18.2: type = converted %.loc15_14.3, %.loc15_18.1 [template = constants.%Class.3]
// CHECK:STDOUT:     %x.loc15_6.1: %Class.3 = param x
// CHECK:STDOUT:     @F.%x: %Class.3 = bind_name x, %x.loc15_6.1
// CHECK:STDOUT:     %int.make_type_32.loc15_24: init type = call constants.%Int32() [template = i32]
// CHECK:STDOUT:     %.loc15_24.1: type = value_of_initializer %int.make_type_32.loc15_24 [template = i32]
// CHECK:STDOUT:     %.loc15_24.2: type = converted %int.make_type_32.loc15_24, %.loc15_24.1 [template = i32]
// CHECK:STDOUT:     @F.%return: ref i32 = var <return slot>
// CHECK:STDOUT:   }
// CHECK:STDOUT: }
// CHECK:STDOUT:
// CHECK:STDOUT: class @Class {
// CHECK:STDOUT:   %T.ref: type = name_ref T, file.%T.loc11_13.2 [symbolic = constants.%T]
// CHECK:STDOUT:   %.loc12: %.2 = field_decl x, element0 [template]
// CHECK:STDOUT:
// CHECK:STDOUT: !members:
// CHECK:STDOUT:   .Self = constants.%Class.2
// CHECK:STDOUT:   .x = %.loc12
// CHECK:STDOUT: }
// CHECK:STDOUT:
// CHECK:STDOUT: fn @Int32() -> type = "int.make_type_32";
// CHECK:STDOUT:
// CHECK:STDOUT: fn @F(%x: %Class.3) -> i32 {
// CHECK:STDOUT: !entry:
// CHECK:STDOUT:   %x.ref.loc19_10: %Class.3 = name_ref x, %x
// CHECK:STDOUT:   %x.ref.loc19_11: %.4 = name_ref x, @Class.%.loc12 [template = @Class.%.loc12]
// CHECK:STDOUT:   %.loc19_11.1: ref i32 = class_element_access %x.ref.loc19_10, element0
// CHECK:STDOUT:   %.loc19_11.2: i32 = bind_value %.loc19_11.1
// CHECK:STDOUT:   return %.loc19_11.2
// CHECK:STDOUT: }
// CHECK:STDOUT:
