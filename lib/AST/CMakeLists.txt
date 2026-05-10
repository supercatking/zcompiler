add_library(ZCompilerAST
  AST.cpp
)

llvm_update_compile_flags(ZCompilerAST)

target_include_directories(ZCompilerAST
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${LLVM_INCLUDE_DIRS}
)

target_compile_definitions(ZCompilerAST
  PRIVATE
    ${LLVM_DEFINITIONS}
)

llvm_map_components_to_libnames(LLVM_AST_LIBS
  support
)

target_link_libraries(ZCompilerAST
  PUBLIC
    ${LLVM_AST_LIBS}
)

