#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>
#include <llvm-c/ExecutionEngine.h>

#include <stdio.h>

int main() {
  LLVMContextRef c = LLVMGetGlobalContext();
  LLVMModuleRef m = LLVMModuleCreateWithNameInContext("test", c);
  LLVMTypeRef i = LLVMInt32TypeInContext(c);
  LLVMTypeRef ps[] = {i, i};
  LLVMTypeRef ft = LLVMFunctionType(i, ps, 2, 0);
  LLVMValueRef f = LLVMAddFunction(m, "plus", ft);
  LLVMBasicBlockRef bb = LLVMAppendBasicBlockInContext(c, f, "entry");
  LLVMBuilderRef b = LLVMCreateBuilder();
  LLVMPositionBuilderAtEnd(b, bb);
  LLVMValueRef r =
      LLVMBuildNSWAdd(b, LLVMGetParam(f, 0), LLVMGetParam(f, 1), "");
  LLVMBuildRet(b, r);
  if (LLVMVerifyFunction(f, LLVMPrintMessageAction)) {
    return 1;
  }
  char *err;
  if (LLVMVerifyModule(m, LLVMPrintMessageAction, &err)) {
    puts(err);
    return 2;
  }
  LLVMDisposeBuilder(b);

  LLVMDumpModule(m);

  LLVMLinkInMCJIT();
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMExecutionEngineRef ee;
  if (LLVMCreateJITCompilerForModule(&ee, m, 0, &err)) {
    puts(err);
    return 3;
  }
  int (*plus)(int, int) = (int (*)(int, int))LLVMGetPointerToGlobal(ee, f);
  printf("%d\n", plus(3, 4));
  return 0;
}
