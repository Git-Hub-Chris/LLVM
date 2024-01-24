; RUN: llc -mtriple=amdgcn-amd-amdpal -mcpu=gfx900 -mattr=+architected-sgprs -global-isel=0 -verify-machineinstrs < %s | FileCheck -check-prefix=GFX9 %s
; RUN: llc -mtriple=amdgcn-amd-amdpal -mcpu=gfx900 -mattr=+architected-sgprs -global-isel=1 -verify-machineinstrs < %s | FileCheck -check-prefix=GFX9 %s
; RUN: llc -mtriple=amdgcn-amd-amdpal -mcpu=gfx1200 -global-isel=0 -verify-machineinstrs < %s | FileCheck -check-prefix=GFX12 %s
; RUN: llc -mtriple=amdgcn-amd-amdpal -mcpu=gfx1200 -global-isel=1 -verify-machineinstrs < %s | FileCheck -check-prefix=GFX12 %s

define amdgpu_cs void @test_wave_id(ptr addrspace(1) %out) {
; GFX9-LABEL: test_wave_id:
; GFX9:       ; %bb.0:
; GFX9-NEXT:    s_bfe_u32 s0, ttmp8, 0x50019
; GFX9-NEXT:    v_mov_b32_e32 v2, s0
; GFX9-NEXT:    global_store_dword v[0:1], v2, off
; GFX9-NEXT:    s_endpgm
;
; GFX12-LABEL: test_wave_id:
; GFX12:       ; %bb.0:
; GFX12-NEXT:    s_bfe_u32 s0, ttmp8, 0x50019
; GFX12-NEXT:    s_delay_alu instid0(SALU_CYCLE_1)
; GFX12-NEXT:    v_mov_b32_e32 v2, s0
; GFX12-NEXT:    global_store_b32 v[0:1], v2, off
; GFX12-NEXT:    s_nop 0
; GFX12-NEXT:    s_sendmsg sendmsg(MSG_DEALLOC_VGPRS)
; GFX12-NEXT:    s_endpgm
  %waveid = call i32 @llvm.amdgcn.wave.id()
  store i32 %waveid, ptr addrspace(1) %out
  ret void
}

declare i32 @llvm.amdgcn.wave.id()
