add_library(ZCompilerCodeGen
  CodeGen.cpp
)

llvm_update_compile_flags(ZCompilerCodeGen)

target_include_directories(ZCompilerCodeGen
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${LLVM_INCLUDE_DIRS}
)

target_compile_definitions(ZCompilerCodeGen
  PRIVATE
    ${LLVM_DEFINITIONS}
)

llvm_map_components_to_libnames(LLVM_CODEGEN_LIBS
  support
)

target_link_libraries(ZCompilerCodeGen
  PUBLIC
    ZCompilerAST
    ${LLVM_CODEGEN_LIBS}
)

