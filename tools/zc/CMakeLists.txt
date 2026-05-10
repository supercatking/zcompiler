add_executable(zc
  zc.cpp
)

llvm_update_compile_flags(zc)

llvm_map_components_to_libnames(LLVM_LIBS
  support
)

target_include_directories(zc
  PRIVATE
    ${LLVM_INCLUDE_DIRS}
    ${MLIR_INCLUDE_DIRS}
)

target_compile_definitions(zc
  PRIVATE
    ${LLVM_DEFINITIONS}
)

target_link_libraries(zc
  PRIVATE
    ZCompilerCodeGen
    ZCompilerMLIRGen
    ZCompilerParser
    ZCompilerLexer
    ${LLVM_LIBS}
)
