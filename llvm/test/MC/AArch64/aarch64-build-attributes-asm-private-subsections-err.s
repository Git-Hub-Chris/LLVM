// RUN: not llvm-mc -triple=aarch64 %s 2>&1 | FileCheck --check-prefix=ERR %s

.aeabi_subsection private_subsection, optional, uleb128

.aeabi_subsection private_subsection, required, uleb128
// ERR: error: optionality mismatch! subsection 'private_subsection' already exists with optionality defined as '1' and not '0' (0: required, 1: optional)
// ERR-NEXT: .aeabi_subsection private_subsection, required, uleb128

.aeabi_subsection private_subsection, optional, ntbs
// ERR: error: type mismatch! subsection 'private_subsection' already exists with type defined as '0' and not '1' (0: uleb128, 1: ntbs)
// ERR-NEXT: .aeabi_subsection private_subsection, optional, ntbs

.aeabi_subsection private_subsection_1, optional, ntbs
.aeabi_attribute 324, 1
// ERR: error: active subsection type is NTBS (string), found ULEB128 (unsigned)
// ERR-NEXT: .aeabi_attribute 324, 1