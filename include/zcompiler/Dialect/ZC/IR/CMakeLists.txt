set(LLVM_TARGET_DEFINITIONS ZCOps.td)
mlir_tablegen(ZCOps.h.inc -gen-op-decls)
mlir_tablegen(ZCOps.cpp.inc -gen-op-defs)
mlir_tablegen(ZCOpsDialect.h.inc -gen-dialect-decls)
mlir_tablegen(ZCOpsDialect.cpp.inc -gen-dialect-defs)
add_public_tablegen_target(ZCompilerZCOpsIncGen)

