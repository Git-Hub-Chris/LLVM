## Note: This file is compiled from the following code, for 
## 		the purpose of creating an non split-dwarf binary. 
## clang -g -S -gdwarf-4 main.c

#int main() {
#    return 0;
#}

        .text
        .file   "main.c"
        .globl  main                            # -- Begin function main
        .p2align        4, 0x90
        .type   main,@function
main:                                   # @main
.Lfunc_begin0:
        .file   1 "xxx/xxx/xxx" "main.c"
        .loc    1 1 0                           # main.c:1:0
        .cfi_startproc
# %bb.0:                                # %entry
        pushq   %rbp
        .cfi_def_cfa_offset 16
        .cfi_offset %rbp, -16
        movq    %rsp, %rbp
        .cfi_def_cfa_register %rbp
        xorl    %eax, %eax
        movl    $0, -4(%rbp)
.Ltmp0:
        .loc    1 2 5 prologue_end              # main.c:2:5
        popq    %rbp
        .cfi_def_cfa %rsp, 8
        retq
.Ltmp1:
.Lfunc_end0:
        .size   main, .Lfunc_end0-main
        .cfi_endproc
                                        # -- End function
        .section        .debug_abbrev,"",@progbits
        .byte   1                               # Abbreviation Code
        .byte   17                              # DW_TAG_compile_unit
        .byte   1                               # DW_CHILDREN_yes
        .byte   37                              # DW_AT_producer
        .byte   14                              # DW_FORM_strp
        .byte   19                              # DW_AT_language
        .byte   5                               # DW_FORM_data2
        .byte   3                               # DW_AT_name
        .byte   14                              # DW_FORM_strp
        .byte   16                              # DW_AT_stmt_list
        .byte   23                              # DW_FORM_sec_offset
        .byte   27                              # DW_AT_comp_dir
        .byte   14                              # DW_FORM_strp
        .byte   17                              # DW_AT_low_pc
        .byte   1                               # DW_FORM_addr
        .byte   18                              # DW_AT_high_pc
        .byte   6                               # DW_FORM_data4
        .byte   0                               # EOM(1)
        .byte   0                               # EOM(2)
        .byte   2                               # Abbreviation Code
        .byte   46                              # DW_TAG_subprogram
        .byte   0                               # DW_CHILDREN_no
        .byte   17                              # DW_AT_low_pc
        .byte   1                               # DW_FORM_addr
        .byte   18                              # DW_AT_high_pc
        .byte   6                               # DW_FORM_data4
        .byte   64                              # DW_AT_frame_base
        .byte   24                              # DW_FORM_exprloc
        .byte   3                               # DW_AT_name
        .byte   14                              # DW_FORM_strp
        .byte   58                              # DW_AT_decl_file
        .byte   11                              # DW_FORM_data1
        .byte   59                              # DW_AT_decl_line
        .byte   11                              # DW_FORM_data1
        .byte   73                              # DW_AT_type
        .byte   19                              # DW_FORM_ref4
        .byte   63                              # DW_AT_external
        .byte   25                              # DW_FORM_flag_present
        .byte   0                               # EOM(1)
        .byte   0                               # EOM(2)
        .byte   3                               # Abbreviation Code
        .byte   36                              # DW_TAG_base_type
        .byte   0                               # DW_CHILDREN_no
        .byte   3                               # DW_AT_name
        .byte   14                              # DW_FORM_strp
        .byte   62                              # DW_AT_encoding
        .byte   11                              # DW_FORM_data1
        .byte   11                              # DW_AT_byte_size
        .byte   11                              # DW_FORM_data1
        .byte   0                               # EOM(1)
        .byte   0                               # EOM(2)
        .byte   0                               # EOM(3)
        .section        .debug_info,"",@progbits
.Lcu_begin0:
        .long   .Ldebug_info_end0-.Ldebug_info_start0 # Length of Unit
.Ldebug_info_start0:
        .short  4                               # DWARF version number
        .long   .debug_abbrev                   # Offset Into Abbrev. Section
        .byte   8                               # Address Size (in bytes)
        .byte   1                               # Abbrev [1] 0xb:0x40 DW_TAG_compile_unit
        .long   .Linfo_string0                  # DW_AT_producer
        .short  12                              # DW_AT_language
        .long   .Linfo_string1                  # DW_AT_name
        .long   .Lline_table_start0             # DW_AT_stmt_list
        .long   .Linfo_string2                  # DW_AT_comp_dir
        .quad   .Lfunc_begin0                   # DW_AT_low_pc
        .long   .Lfunc_end0-.Lfunc_begin0       # DW_AT_high_pc
        .byte   2                               # Abbrev [2] 0x2a:0x19 DW_TAG_subprogram
        .quad   .Lfunc_begin0                   # DW_AT_low_pc
        .long   .Lfunc_end0-.Lfunc_begin0       # DW_AT_high_pc
        .byte   1                               # DW_AT_frame_base
        .byte   86
        .long   .Linfo_string3                  # DW_AT_name
        .byte   1                               # DW_AT_decl_file
        .byte   1                               # DW_AT_decl_line
        .long   67                              # DW_AT_type
                                        # DW_AT_external
        .byte   3                               # Abbrev [3] 0x43:0x7 DW_TAG_base_type
        .long   .Linfo_string4                  # DW_AT_name
        .byte   5                               # DW_AT_encoding
        .byte   4                               # DW_AT_byte_size
        .byte   0                               # End Of Children Mark
.Ldebug_info_end0:
        .section        .debug_str,"MS",@progbits,1
.Linfo_string0:
        .asciz  "clang version 11.0.1git" # string offset=0
.Linfo_string1:
        .asciz  "main.c"                        # string offset=104
.Linfo_string2:
        .asciz  "/data02/home/huangjinjie/origin-llvm-project/llvm-project/build" # string offset=111
.Linfo_string3:
        .asciz  "main"                          # string offset=175
.Linfo_string4:
        .asciz  "int"                           # string offset=180
        .ident  "clang version 11.0.1git"
        .section        .GCC.command.line,"MS",@progbits,1
        .zero   1
        .ascii  "/usr/local/bin/clang-11 -g -S -gdwarf-4 main.c -o main_non_split.s"
        .zero   1
        .section        .debug_str,"MS",@progbits,1
        .section        ".note.GNU-stack","",@progbits
        .addrsig
        .section        .debug_line,"",@progbits
.Lline_table_start0:
