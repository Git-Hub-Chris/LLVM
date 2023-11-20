// Test that verifies that hwasan produces valid symbolizer markup when enabled.
// RUN: %clang_hwasan %s -o %t
// RUN: env HWASAN_OPTIONS=enable_symbolizer_markup=1 not %run %t 2>&1 | FileCheck %s
// REQUIRES: linux && aarch64-target-arch

#include <sanitizer/hwasan_interface.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  __hwasan_enable_allocator_tagging();
  char *x = (char *)malloc(10 * sizeof(char));
  free(x);
  __hwasan_disable_allocator_tagging();

  int r = 0;
  r = x[5];
  return r;
}

// COM: For element syntax see: https://llvm.org/docs/SymbolizerMarkupFormat.html
// COM: OPEN is {{{ and CLOSE is }}}
// CHECK: [[OPEN:{{{]]reset[[CLOSE:}}}]]
// CHECK: [[OPEN]]module:[[MOD_ID:[0-9]+]]:{{.+}}:elf:{{[0-9a-fA-F]+}}[[CLOSE]]
// CHECK: [[OPEN]]mmap:{{0x[0-9a-fA-F]+:0x[0-9a-fA-F]+}}:load:[[MOD_ID]]:{{r[wx]{0,2}:0x[0-9a-fA-F]+}}[[CLOSE]]
// CHECK: [[OPEN]]bt:{{[0-9]+}}:0x{{[0-9a-fA-F]+}}[[CLOSE]]
