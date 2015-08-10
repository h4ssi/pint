#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>

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
  LLVMVerifyFunction(f, LLVMPrintMessageAction);
  LLVMVerifyModule(m, LLVMPrintMessageAction, 0);
  LLVMDisposeBuilder(b);

  LLVMDumpModule(m);
  return 0;
}
