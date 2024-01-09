// ===- Partition.h - Partition dialect -------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef MLIR_DIALECT_PARTITION_IR_PARTITION_H
#define MLIR_DIALECT_PARTITION_IR_PARTITION_H

#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/OpImplementation.h"
#include "mlir/IR/TypeSupport.h"
#include "mlir/IR/TypeUtilities.h"
#include "mlir/IR/Types.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#include <optional>

#define GET_ATTRDEF_CLASSES
#include "mlir/Dialect/PartTensor/IR/PartTensorAttrDefs.h.inc"

#define GET_OP_CLASSES
#include "mlir/Dialect/PartTensor/IR/PartTensorOps.h.inc"

#include "mlir/Dialect/PartTensor/IR/PartTensorOpsDialect.h.inc"

#endif // MLIR_DIALECT_PARTITION_IR_PARTITION_H
