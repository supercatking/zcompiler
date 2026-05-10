add_mlir_conversion_library(ZCompilerZCToStandard
  ZCToStandard.cpp

  ADDITIONAL_HEADER_DIRS
  ${PROJECT_SOURCE_DIR}/include/zcompiler/Conversion/ZCToStandard

  LINK_LIBS PUBLIC
  ZCompilerZCDialect
  MLIRArithDialect
  MLIRIR
  MLIRPass
)

