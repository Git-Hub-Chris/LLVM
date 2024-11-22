// RUN: %clang_cc1 -triple dxil-pc-shadermodel6.6-compute -S -finclude-default-header -o - %s | FileCheck %s

// The purpose of this test is to ensure that the target
// extension type associated with the structured buffer only
// contains anonymous struct types, rather than named
// struct types

// note that "{ <4 x float> }" in the check below is a struct type, but only the
// body is emitted on the RHS because we are already in the context of a
// target extension type definition (class.hlsl::StructuredBuffer)

// CHECK: %"class.hlsl::StructuredBuffer" = type { target("dx.RawBuffer", %struct.mystruct, 0, 0), %struct.mystruct }
// CHECK: %struct.mystruct = type { <4 x float> }
// CHECK: %dx.types.Handle = type { ptr }
// CHECK: %dx.types.ResBind = type { i32, i32, i32, i8 }
// CHECK: %dx.types.ResourceProperties = type { i32, i32 }

struct mystruct
{
    float4 Color;
};

StructuredBuffer<mystruct> my_buffer : register(t2, space4);

export float4 test()
{
    return my_buffer[0].Color;
}

[numthreads(1,1,1)]
void main() {}
