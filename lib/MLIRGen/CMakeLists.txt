add_library(ZCompilerMLIRGen
  MLIRGen.cpp
)

llvm_update_compile_flags(ZCompilerMLIRGen)

target_include_directories(ZCompilerMLIRGen
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${LLVM_INCLUDE_DIRS}
    ${MLIR_INCLUDE_DIRS}
)

target_compile_definitions(ZCompilerMLIRGen
  PRIVATE
    ${LLVM_DEFINITIONS}
)

target_link_libraries(ZCompilerMLIRGen
  PUBLIC
    ZCompilerAST
    MLIRArithDialect
    MLIRFuncDialect
    MLIRMemRefDialect
    MLIRIR
)
