add_mlir_dialect_library(ZCompilerZCDialect
  ZCDialect.cpp
  ZCOps.cpp

  ADDITIONAL_HEADER_DIRS
  ${PROJECT_SOURCE_DIR}/include/zcompiler/Dialect/ZC/IR

  DEPENDS
  ZCompilerZCOpsIncGen

  LINK_LIBS PUBLIC
  MLIRIR
  MLIRSideEffectInterfaces
)

