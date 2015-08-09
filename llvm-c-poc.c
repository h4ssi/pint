#include <llvm-c/Core.h>

int main() {
  LLVMBuilderRef b = LLVMCreateBuilder();
  LLVMDisposeBuilder(b);
  return 0;
}
