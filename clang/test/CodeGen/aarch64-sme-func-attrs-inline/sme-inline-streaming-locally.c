// RUN: %clang --target=aarch64-none-linux-gnu -march=armv9-a+sme -O3 -S -Xclang -verify %s

// Conflicting attributes when using always_inline
__attribute__((always_inline)) __arm_locally_streaming
void inlined_fn_local(void) {}
// expected-error@+1 {{always_inline function 'inlined_fn_local' and its caller 'inlined_fn_caller' have mismatched streaming attributes}}
void inlined_fn_caller(void) { inlined_fn_local(); }
__arm_locally_streaming
void inlined_fn_caller_local(void) { inlined_fn_local(); }
void inlined_fn_caller_streaming(void) __arm_streaming { inlined_fn_local(); }
