//===- llvm/MC/DXContainerRootSignature.h - DXContainer RootSignature -*- C++
//-------*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <cstdint>
#include <limits>

namespace llvm {

class raw_ostream;

namespace mcdxbc {
struct RootSignatureHeader {
  uint32_t Version;
  uint32_t Flags;

  void swapBytes();
  void write(raw_ostream &OS,
             uint32_t Version = std::numeric_limits<uint32_t>::max());
};
} // namespace mcdxbc
} // namespace llvm
