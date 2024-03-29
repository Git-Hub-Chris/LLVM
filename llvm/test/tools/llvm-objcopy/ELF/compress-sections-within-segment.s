## Disallow (de)compression for sections within a segment as they are
## effectively immutable.
# RUN: rm -rf %t && mkdir %t && cd %t
# RUN: yaml2obj %s -o a
# RUN: not llvm-objcopy a /dev/null --compress-sections .text=zlib 2>&1 | FileCheck %s --implicit-check-not=error:

# CHECK: error: 'a': section '.text' within a segment cannot be (de)compressed

# RUN: yaml2obj --docnum=2 %s -o b
# RUN: not llvm-objcopy b /dev/null --compress-sections foo=none 2>&1 | FileCheck %s --check-prefix=CHECK2 --implicit-check-not=error:

# CHECK2: error: 'b': section 'foo' within a segment cannot be (de)compressed

--- !ELF
FileHeader:
  Class:      ELFCLASS64
  Data:       ELFDATA2LSB
  Type:       ET_EXEC
  Machine:    EM_X86_64
ProgramHeaders:
  - Type:     PT_LOAD
    FirstSec: .text
    LastSec:  foo
    VAddr:    0x201000
    Align:    0x1000
    Offset:   0x1000
Sections:
  - Name:     .text
    Type:     SHT_PROGBITS
    Flags:    [ SHF_ALLOC ]
    Address:  0x201000
    Offset:   0x1000
    Content:  C3
  - Name:     foo
    Type:     SHT_PROGBITS
    Flags:    [ SHF_ALLOC, SHF_COMPRESSED ]
    Content:  010000000000000040000000000000000100000000000000789cd36280002d3269002f800151

--- !ELF
FileHeader:
  Class:      ELFCLASS64
  Data:       ELFDATA2LSB
  Type:       ET_EXEC
  Machine:    EM_X86_64
ProgramHeaders:
  - Type:     PT_LOAD
    FirstSec: foo
    LastSec:  foo
    VAddr:    0x201000
    Align:    0x1000
    Offset:   0x1000
Sections:
  - Name:     foo
    Type:     SHT_PROGBITS
    Flags:    [ SHF_ALLOC, SHF_COMPRESSED ]
    Address:  0x201000
    Offset:   0x1000
    Content:  010000000000000040000000000000000100000000000000789cd36280002d3269002f800151
