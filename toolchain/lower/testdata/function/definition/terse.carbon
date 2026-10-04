// Part of the Carbon Language project, under the Apache License v2.0 with LLVM
// Exceptions. See /LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// INCLUDE-FILE: toolchain/testing/testdata/min_prelude/int.carbon
//
// AUTOUPDATE
// TIP: To test this file alone, run:
// TIP:   bazel test //toolchain/testing:file_test --test_arg=--file_tests=toolchain/lower/testdata/function/definition/terse.carbon
// TIP: To dump output, run:
// TIP:   bazel run //toolchain/testing:file_test -- --dump_output --file_tests=toolchain/lower/testdata/function/definition/terse.carbon

// --- lib.carbon
library "[[@TEST_NAME]]";

fn ImportedId(x: i32) => x;
fn ImportedGenericId[T: Core.Copy](x: T) => x;

// --- main.carbon
library "[[@TEST_NAME]]";

import library "lib";

fn LocalId(x: i32) => x;
fn LocalGenericId[T: Core.Copy](x: T) => x;

fn Unit() => ();

fn MakePair[T: Core.Copy, U: Core.Copy](x: T, y: U) => {.a = x, .b = y};

class C {
  fn Get(self: Self) => self.x;
  var x: i32;
}

fn Test(c: C) -> i32 {
  Unit();
  let p: {.a: i32, .b: i32} =
      MakePair(ImportedGenericId(ImportedId(c.Get())), LocalGenericId(LocalId(2)));
  return p.a;
}

// CHECK:STDOUT: ; ---
// CHECK:STDOUT: ; ModuleID = 'lib.carbon'
// CHECK:STDOUT: source_filename = "lib.carbon"
// CHECK:STDOUT:
// CHECK:STDOUT: ; Function Attrs: nounwind
// CHECK:STDOUT: define i32 @_CImportedId.Main(i32 %x) #0 !dbg !23 {
// CHECK:STDOUT: entry:
// CHECK:STDOUT:   ret i32 %x, !dbg !26
// CHECK:STDOUT: }
// CHECK:STDOUT:
// CHECK:STDOUT: attributes #0 = { nounwind }
// CHECK:STDOUT:
// CHECK:STDOUT: !llvm.dbg.cu = !{!19}
// CHECK:STDOUT: !llvm.module.flags = !{!2, !3}
// CHECK:STDOUT:
// CHECK:STDOUT: !2 = !{i32 7, !"Dwarf Version", i32 5}
// CHECK:STDOUT: !3 = !{i32 2, !"Debug Info Version", i32 3}
// CHECK:STDOUT: !18 = !DIFile(filename: "lib.carbon", directory: "")
// CHECK:STDOUT: !19 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !18, producer: "carbon", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
// CHECK:STDOUT: !20 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
// CHECK:STDOUT: !21 = !{!20, !20}
// CHECK:STDOUT: !22 = !DISubroutineType(types: !21)
// CHECK:STDOUT: !23 = distinct !DISubprogram(name: "ImportedId", linkageName: "_CImportedId.Main", scope: null, file: !18, line: 3, type: !22, spFlags: DISPFlagDefinition, unit: !19, retainedNodes: !27)
// CHECK:STDOUT: !24 = !DILocalVariable(arg: 1, scope: !23, type: !20)
// CHECK:STDOUT: !26 = !DILocation(line: 3, column: 26, scope: !23)
// CHECK:STDOUT: !27 = !{!24}
// CHECK:STDOUT:
// CHECK:STDOUT: ; ---
// CHECK:STDOUT: ; ModuleID = 'main.carbon'
// CHECK:STDOUT: source_filename = "main.carbon"
// CHECK:STDOUT:
// CHECK:STDOUT: ; Function Attrs: nounwind
// CHECK:STDOUT: define i32 @_CLocalId.Main(i32 %x) #0 !dbg !39 {
// CHECK:STDOUT: entry:
// CHECK:STDOUT:   ret i32 %x, !dbg !42
// CHECK:STDOUT: }
// CHECK:STDOUT:
// CHECK:STDOUT: ; Function Attrs: nounwind
// CHECK:STDOUT: define void @_CUnit.Main() #0 !dbg !44 {
// CHECK:STDOUT: entry:
// CHECK:STDOUT:   ret void, !dbg !45
// CHECK:STDOUT: }
// CHECK:STDOUT:
// CHECK:STDOUT: ; Function Attrs: nounwind
// CHECK:STDOUT: define i32 @_CGet.C.Main(ptr %self) #0 !dbg !46 {
// CHECK:STDOUT: entry:
// CHECK:STDOUT:   %.loc13_29.1.x = getelementptr inbounds nuw { i32 }, ptr %self, i32 0, i32 0, !dbg !49
// CHECK:STDOUT:   %.loc13_29.2 = load i32, ptr %.loc13_29.1.x, align 4, !dbg !49
// CHECK:STDOUT:   ret i32 %.loc13_29.2, !dbg !49
// CHECK:STDOUT: }
// CHECK:STDOUT:
// CHECK:STDOUT: ; Function Attrs: nounwind
// CHECK:STDOUT: define i32 @_CTest.Main(ptr %c) #0 !dbg !51 {
// CHECK:STDOUT: entry:
// CHECK:STDOUT:   %.loc20_82.5.temp = alloca { i32, i32 }, align 4, !dbg !62
// CHECK:STDOUT:   %struct = alloca { i32, i32 }, align 4, !dbg !62
// CHECK:STDOUT:   call void @_CUnit.Main(), !dbg !56
// CHECK:STDOUT:   %C.Get.call = call i32 @_CGet.C.Main(ptr %c), !dbg !57
// CHECK:STDOUT:   %ImportedId.call = call i32 @_CImportedId.Main(i32 %C.Get.call), !dbg !58
// CHECK:STDOUT:   %ImportedGenericId.call = call i32 @_CImportedGenericId.Main.4d6ceefeb7f772f4(i32 %ImportedId.call), !dbg !59
// CHECK:STDOUT:   %LocalId.call = call i32 @_CLocalId.Main(i32 2), !dbg !60
// CHECK:STDOUT:   %LocalGenericId.call = call i32 @_CLocalGenericId.Main.4d6ceefeb7f772f4(i32 %LocalId.call), !dbg !61
// CHECK:STDOUT:   call void @llvm.lifetime.start.p0(ptr %.loc20_82.5.temp), !dbg !62
// CHECK:STDOUT:   call void @_CMakePair.Main.8608f3c6f62d80cb(ptr %.loc20_82.5.temp, i32 %ImportedGenericId.call, i32 %LocalGenericId.call), !dbg !62
// CHECK:STDOUT:   %.loc20_82.7.a = getelementptr inbounds nuw { i32, i32 }, ptr %.loc20_82.5.temp, i32 0, i32 0, !dbg !62
// CHECK:STDOUT:   %.loc20_82.8 = load i32, ptr %.loc20_82.7.a, align 4, !dbg !62
// CHECK:STDOUT:   %.loc20_82.9.b = getelementptr inbounds nuw { i32, i32 }, ptr %.loc20_82.5.temp, i32 0, i32 1, !dbg !62
// CHECK:STDOUT:   %.loc20_82.10 = load i32, ptr %.loc20_82.9.b, align 4, !dbg !62
// CHECK:STDOUT:   call void @llvm.lifetime.start.p0(ptr %struct), !dbg !62
// CHECK:STDOUT:   %struct1 = getelementptr inbounds nuw { i32, i32 }, ptr %struct, i32 0, i32 0, !dbg !62
// CHECK:STDOUT:   store i32 %.loc20_82.8, ptr %struct1, align 4, !dbg !62
// CHECK:STDOUT:   %struct2 = getelementptr inbounds nuw { i32, i32 }, ptr %struct, i32 0, i32 1, !dbg !62
// CHECK:STDOUT:   store i32 %.loc20_82.10, ptr %struct2, align 4, !dbg !62
// CHECK:STDOUT:   %.loc21.a = getelementptr inbounds nuw { i32, i32 }, ptr %struct, i32 0, i32 0, !dbg !67
// CHECK:STDOUT:   %.loc21.a.load = load i32, ptr %.loc21.a, align 4, !dbg !67
// CHECK:STDOUT:   call void @"_CSelfDestruct.b4c762864cd9a02b:core.Destroy.Core"(ptr %.loc20_82.5.temp), !dbg !62
// CHECK:STDOUT:   ret i32 %.loc21.a.load, !dbg !68
// CHECK:STDOUT: }
// CHECK:STDOUT:
// CHECK:STDOUT: declare i32 @_CImportedId.Main(i32)
// CHECK:STDOUT:
// CHECK:STDOUT: ; Function Attrs: nounwind
// CHECK:STDOUT: define weak_odr void @"_CSelfDestruct.f58524831c37807e:core.Destroy.Core"(ptr %self) #0 !dbg !70 {
// CHECK:STDOUT: entry:
// CHECK:STDOUT:   ret void, !dbg !72
// CHECK:STDOUT: }
// CHECK:STDOUT:
// CHECK:STDOUT: ; Function Attrs: nounwind
// CHECK:STDOUT: define weak_odr void @"_CSubobjectDestroy.b4c762864cd9a02b:core.Destroy.Core"(ptr %self) #0 !dbg !74 {
// CHECK:STDOUT: entry:
// CHECK:STDOUT:   %.loc20_82.1.b = getelementptr inbounds nuw { i32, i32 }, ptr %self, i32 0, i32 1, !dbg !76
// CHECK:STDOUT:   call void @"_CSelfDestruct.f58524831c37807e:core.Destroy.Core"(ptr %.loc20_82.1.b), !dbg !76
// CHECK:STDOUT:   %.loc20_82.2.a = getelementptr inbounds nuw { i32, i32 }, ptr %self, i32 0, i32 0, !dbg !76
// CHECK:STDOUT:   call void @"_CSelfDestruct.f58524831c37807e:core.Destroy.Core"(ptr %.loc20_82.2.a), !dbg !76
// CHECK:STDOUT:   ret void, !dbg !76
// CHECK:STDOUT: }
// CHECK:STDOUT:
// CHECK:STDOUT: ; Function Attrs: nounwind
// CHECK:STDOUT: define weak_odr void @"_CSelfDestruct.b4c762864cd9a02b:core.Destroy.Core"(ptr %self) #0 !dbg !78 {
// CHECK:STDOUT: entry:
// CHECK:STDOUT:   call void @"_CSubobjectDestroy.b4c762864cd9a02b:core.Destroy.Core"(ptr %self), !dbg !80
// CHECK:STDOUT:   ret void, !dbg !80
// CHECK:STDOUT: }
// CHECK:STDOUT:
// CHECK:STDOUT: ; Function Attrs: nounwind
// CHECK:STDOUT: define linkonce_odr i32 @_CImportedGenericId.Main.4d6ceefeb7f772f4(i32 %x) #0 !dbg !82 {
// CHECK:STDOUT:   ret i32 %x, !dbg !85
// CHECK:STDOUT: }
// CHECK:STDOUT:
// CHECK:STDOUT: ; Function Attrs: nounwind
// CHECK:STDOUT: define linkonce_odr i32 @_CLocalGenericId.Main.4d6ceefeb7f772f4(i32 %x) #0 !dbg !87 {
// CHECK:STDOUT: entry:
// CHECK:STDOUT:   ret i32 %x, !dbg !90
// CHECK:STDOUT: }
// CHECK:STDOUT:
// CHECK:STDOUT: ; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
// CHECK:STDOUT: declare void @llvm.lifetime.start.p0(ptr captures(none)) #1
// CHECK:STDOUT:
// CHECK:STDOUT: ; Function Attrs: nounwind
// CHECK:STDOUT: define linkonce_odr void @_CMakePair.Main.8608f3c6f62d80cb(ptr sret({ i32, i32 }) %return, i32 %x, i32 %y) #0 !dbg !92 {
// CHECK:STDOUT: entry:
// CHECK:STDOUT:   %.loc10_71.2.a = getelementptr inbounds nuw { i32, i32 }, ptr %return, i32 0, i32 0, !dbg !97
// CHECK:STDOUT:   store i32 %x, ptr %.loc10_71.2.a, align 4, !dbg !97
// CHECK:STDOUT:   %.loc10_71.4.b = getelementptr inbounds nuw { i32, i32 }, ptr %return, i32 0, i32 1, !dbg !97
// CHECK:STDOUT:   store i32 %y, ptr %.loc10_71.4.b, align 4, !dbg !97
// CHECK:STDOUT:   ret void, !dbg !97
// CHECK:STDOUT: }
// CHECK:STDOUT:
// CHECK:STDOUT: ; uselistorder directives
// CHECK:STDOUT: uselistorder ptr @llvm.lifetime.start.p0, { 1, 0 }
// CHECK:STDOUT:
// CHECK:STDOUT: attributes #0 = { nounwind }
// CHECK:STDOUT: attributes #1 = { nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
// CHECK:STDOUT:
// CHECK:STDOUT: !llvm.dbg.cu = !{!29}
// CHECK:STDOUT: !llvm.module.flags = !{!2, !3}
// CHECK:STDOUT:
// CHECK:STDOUT: !2 = !{i32 7, !"Dwarf Version", i32 5}
// CHECK:STDOUT: !3 = !{i32 2, !"Debug Info Version", i32 3}
// CHECK:STDOUT: !18 = !DIFile(filename: "lib.carbon", directory: "")
// CHECK:STDOUT: !20 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
// CHECK:STDOUT: !21 = !{!20, !20}
// CHECK:STDOUT: !22 = !DISubroutineType(types: !21)
// CHECK:STDOUT: !28 = !DIFile(filename: "main.carbon", directory: "")
// CHECK:STDOUT: !29 = distinct !DICompileUnit(language: DW_LANG_C_plus_plus, file: !28, producer: "carbon", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug)
// CHECK:STDOUT: !30 = !{null}
// CHECK:STDOUT: !31 = !DISubroutineType(types: !30)
// CHECK:STDOUT: !32 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 64)
// CHECK:STDOUT: !33 = !{!20, !32}
// CHECK:STDOUT: !34 = !DISubroutineType(types: !33)
// CHECK:STDOUT: !35 = !{null, !20}
// CHECK:STDOUT: !36 = !DISubroutineType(types: !35)
// CHECK:STDOUT: !37 = !{null, !32}
// CHECK:STDOUT: !38 = !DISubroutineType(types: !37)
// CHECK:STDOUT: !39 = distinct !DISubprogram(name: "LocalId", linkageName: "_CLocalId.Main", scope: null, file: !28, line: 5, type: !22, spFlags: DISPFlagDefinition, unit: !29, retainedNodes: !43)
// CHECK:STDOUT: !40 = !DILocalVariable(arg: 1, scope: !39, type: !20)
// CHECK:STDOUT: !42 = !DILocation(line: 5, column: 23, scope: !39)
// CHECK:STDOUT: !43 = !{!40}
// CHECK:STDOUT: !44 = distinct !DISubprogram(name: "Unit", linkageName: "_CUnit.Main", scope: null, file: !28, line: 8, type: !31, spFlags: DISPFlagDefinition, unit: !29)
// CHECK:STDOUT: !45 = !DILocation(line: 8, column: 14, scope: !44)
// CHECK:STDOUT: !46 = distinct !DISubprogram(name: "Get", linkageName: "_CGet.C.Main", scope: null, file: !28, line: 13, type: !34, spFlags: DISPFlagDefinition, unit: !29, retainedNodes: !50)
// CHECK:STDOUT: !47 = !DILocalVariable(arg: 1, scope: !46, type: !32)
// CHECK:STDOUT: !49 = !DILocation(line: 13, column: 25, scope: !46)
// CHECK:STDOUT: !50 = !{!47}
// CHECK:STDOUT: !51 = distinct !DISubprogram(name: "Test", linkageName: "_CTest.Main", scope: null, file: !28, line: 17, type: !34, spFlags: DISPFlagDefinition, unit: !29, retainedNodes: !69)
// CHECK:STDOUT: !52 = !DILocalVariable(arg: 1, scope: !51, type: !32)
// CHECK:STDOUT: !56 = !DILocation(line: 18, column: 3, scope: !51)
// CHECK:STDOUT: !57 = !DILocation(line: 20, column: 45, scope: !51)
// CHECK:STDOUT: !58 = !DILocation(line: 20, column: 34, scope: !51)
// CHECK:STDOUT: !59 = !DILocation(line: 20, column: 16, scope: !51)
// CHECK:STDOUT: !60 = !DILocation(line: 20, column: 71, scope: !51)
// CHECK:STDOUT: !61 = !DILocation(line: 20, column: 56, scope: !51)
// CHECK:STDOUT: !62 = !DILocation(line: 20, column: 7, scope: !51)
// CHECK:STDOUT: !63 = !{!32, !20, !20}
// CHECK:STDOUT: !64 = !DISubroutineType(types: !63)
// CHECK:STDOUT: !67 = !DILocation(line: 21, column: 10, scope: !51)
// CHECK:STDOUT: !68 = !DILocation(line: 21, column: 3, scope: !51)
// CHECK:STDOUT: !69 = !{!52}
// CHECK:STDOUT: !70 = distinct !DISubprogram(name: "SelfDestruct", linkageName: "_CSelfDestruct.f58524831c37807e:core.Destroy.Core", scope: null, file: !28, line: 20, type: !36, spFlags: DISPFlagDefinition, unit: !29, retainedNodes: !73)
// CHECK:STDOUT: !71 = !DILocalVariable(arg: 1, scope: !70, type: !20)
// CHECK:STDOUT: !72 = !DILocation(line: 20, column: 7, scope: !70)
// CHECK:STDOUT: !73 = !{!71}
// CHECK:STDOUT: !74 = distinct !DISubprogram(name: "SubobjectDestroy", linkageName: "_CSubobjectDestroy.b4c762864cd9a02b:core.Destroy.Core", scope: null, file: !28, line: 20, type: !38, spFlags: DISPFlagDefinition, unit: !29, retainedNodes: !77)
// CHECK:STDOUT: !75 = !DILocalVariable(arg: 1, scope: !74, type: !32)
// CHECK:STDOUT: !76 = !DILocation(line: 20, column: 7, scope: !74)
// CHECK:STDOUT: !77 = !{!75}
// CHECK:STDOUT: !78 = distinct !DISubprogram(name: "SelfDestruct", linkageName: "_CSelfDestruct.b4c762864cd9a02b:core.Destroy.Core", scope: null, file: !28, line: 20, type: !38, spFlags: DISPFlagDefinition, unit: !29, retainedNodes: !81)
// CHECK:STDOUT: !79 = !DILocalVariable(arg: 1, scope: !78, type: !32)
// CHECK:STDOUT: !80 = !DILocation(line: 20, column: 7, scope: !78)
// CHECK:STDOUT: !81 = !{!79}
// CHECK:STDOUT: !82 = distinct !DISubprogram(name: "ImportedGenericId", linkageName: "_CImportedGenericId.Main.4d6ceefeb7f772f4", scope: null, file: !18, line: 4, type: !22, spFlags: DISPFlagDefinition, unit: !29, retainedNodes: !86)
// CHECK:STDOUT: !83 = !DILocalVariable(arg: 1, scope: !82, type: !20)
// CHECK:STDOUT: !85 = !DILocation(line: 4, column: 45, scope: !82)
// CHECK:STDOUT: !86 = !{!83}
// CHECK:STDOUT: !87 = distinct !DISubprogram(name: "LocalGenericId", linkageName: "_CLocalGenericId.Main.4d6ceefeb7f772f4", scope: null, file: !28, line: 6, type: !22, spFlags: DISPFlagDefinition, unit: !29, retainedNodes: !91)
// CHECK:STDOUT: !88 = !DILocalVariable(arg: 1, scope: !87, type: !20)
// CHECK:STDOUT: !90 = !DILocation(line: 6, column: 42, scope: !87)
// CHECK:STDOUT: !91 = !{!88}
// CHECK:STDOUT: !92 = distinct !DISubprogram(name: "MakePair", linkageName: "_CMakePair.Main.8608f3c6f62d80cb", scope: null, file: !28, line: 10, type: !64, spFlags: DISPFlagDefinition, unit: !29, retainedNodes: !100)
// CHECK:STDOUT: !93 = !DILocalVariable(arg: 1, scope: !92, type: !20)
// CHECK:STDOUT: !94 = !DILocalVariable(arg: 2, scope: !92, type: !20)
// CHECK:STDOUT: !97 = !DILocation(line: 10, column: 56, scope: !92)
// CHECK:STDOUT: !100 = !{!93, !94}
// CHECK:STDOUT:
