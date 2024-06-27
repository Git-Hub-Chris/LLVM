// tests arith operations on indices.

// RUN: mlir-opt %s --test-lower-to-llvm | \
// RUN:   mlir-cpu-runner -e entry -entry-point-result=void \
// RUN:                   --shared-libs=%mlir_c_runner_utils | \
// RUN:   FileCheck %s --match-full-lines

func.func @i32Neg1IndexCast() {
    // index casting of -1 : i32 -> 2^64 - 1
    // index_cast(-1 : i32) = 2^64 - 1;
    // CHECK-NEXT: -1
    // CHECK-NEXT: 18446744073709551615
    %c-1_i32 = arith.constant -1 : i32
    %0 = arith.index_cast %c-1_i32 : i32 to index
    vector.print %c-1_i32 : i32
    vector.print %0 : index
    return
}

func.func @indexCastuiAndSi() {
    // casting to index, ui and si
    // -1 : i32; index_cast(-1 : i32) = 2^64 - 1; index_castui(-1 : i32) = 2^32 - 1
    // CHECK-NEXT: -1
    // CHECK-NEXT: 18446744073709551615
    // CHECK-NEXT: 4294967295
    %c-1_i32 = arith.constant -1 : i32
    %0 = arith.index_cast %c-1_i32 : i32 to index
    %1 = arith.index_castui %c-1_i32 : i32 to index
    vector.print %c-1_i32 : i32
    vector.print %0 : index
    vector.print %1 : index
    return
}

func.func @indexCastAfterConversion() {
    // index should be represented as unsigned ints, and reflected through comparison:
    // index_cast(x) `slt` index_cast(y) == x `slt` y
    // CHECK-NEXT: 1
    %c15608_i32 = arith.constant 15608 : i32
    %c-9705694_i64 = arith.constant -9705694 : i64
    %0 = arith.index_cast %c15608_i32 : i32 to index      // small positive num
    %1 = arith.index_cast %c-9705694_i64 : i64 to index   // large positive num
    %2 = arith.cmpi slt, %1, %0 : index                   // %1 `slt` %0 => true
    vector.print %2 : i1
    return
}

func.func @indexCastuiDowncast() {
    // index_castui casting down truncates bits
    // index_castui 11277513 = 1 : i3
    // CHECK-NEXT: 1
    %c11277513 = arith.constant 11277513 : index
    %0 = arith.index_castui %c11277513 : index to i3
    vector.print %0 : i3
    return
}

func.func @indexCastDowncast() {
    // index_cast casting down truncates bits
    // index_cast -3762 = 334 : i12
    // CHECK-NEXT: 334
    %c3762 = arith.constant -3762 : index
    %0 = arith.index_cast %c3762 : index to i12
    vector.print %0 : i12
    return
}

func.func @c0() -> i64 {
    // heper function for below test
    %c100_i64 = arith.constant 100 : i64
    return %c100_i64 : i64
}

func.func @entry() {
    func.call @i32Neg1IndexCast() : () -> ()
    func.call @indexCastuiAndSi() : () -> ()
    func.call @indexCastAfterConversion() : () -> ()
    func.call @indexCastuiDowncast() : () -> ()
    func.call @indexCastDowncast() : () -> ()

    return
}
