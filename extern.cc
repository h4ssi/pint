
extern "C" {
extern void ExternLLVMInitializeNativeTarget(void);
extern void ExternLLVMInitializeNativeAsmPrinter(void);
}

#include <llvm-c/Target.h>

void ExternLLVMInitializeNativeTarget(void) { LLVMInitializeNativeTarget(); }

void ExternLLVMInitializeNativeAsmPrinter(void) {
  LLVMInitializeNativeAsmPrinter();
}
