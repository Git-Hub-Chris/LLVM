//===- AMDGPUDelayedMCExpr.cpp - Delayed MCExpr resolve ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AMDGPUDelayedMCExpr.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCValue.h"

using namespace llvm;

static msgpack::DocNode getNode(msgpack::DocNode DN, msgpack::Type Type,
                                MCValue Val) {
  msgpack::Document *Doc = DN.getDocument();
  switch (Type) {
  default:
    return Doc->getEmptyNode();
  case msgpack::Type::Int:
    return Doc->getNode(static_cast<int64_t>(Val.getConstant()));
  case msgpack::Type::UInt:
    return Doc->getNode(static_cast<uint64_t>(Val.getConstant()));
  case msgpack::Type::Boolean:
    return Doc->getNode(static_cast<bool>(Val.getConstant()));
  }
}

void DelayedMCExpr::AssignDocNode(msgpack::DocNode &DN, msgpack::Type Type,
                                  const MCExpr *Expr) {
  MCValue Res;
  if (Expr->evaluateAsRelocatable(Res, nullptr, nullptr)) {
    if (Res.isAbsolute()) {
      DN = getNode(DN, Type, Res);
      return;
    }
  }

  DelayedExprs.push_back(DelayedExpr{DN, Type, Expr});
}

bool DelayedMCExpr::ResolveDelayedExpressions() {
  bool Success;

  while (!DelayedExprs.empty()) {
    DelayedExpr DE = DelayedExprs.front();
    MCValue Res;

    Success = DE.Expr->evaluateAsRelocatable(Res, nullptr, nullptr);
    Success &= Res.isAbsolute();
    if (!Success)
      return false;

    DelayedExprs.pop_front();
    DE.DN = getNode(DE.DN, DE.Type, Res);
  }

  return true;
}

void DelayedMCExpr::clear() { DelayedExprs.clear(); }

bool DelayedMCExpr::empty() { return DelayedExprs.empty(); }
