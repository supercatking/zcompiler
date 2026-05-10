add_library(ZCompilerRiscVBackend
  RiscVBackend.cpp
)

llvm_update_compile_flags(ZCompilerRiscVBackend)

target_include_directories(ZCompilerRiscVBackend
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
    ${LLVM_INCLUDE_DIRS}
    ${MLIR_INCLUDE_DIRS}
)

target_compile_definitions(ZCompilerRiscVBackend
  PRIVATE
    ${LLVM_DEFINITIONS}
)

llvm_map_components_to_libnames(LLVM_RISCV_BACKEND_LIBS
  support
)

target_link_libraries(ZCompilerRiscVBackend
  PUBLIC
    ZCompilerCodeGen
    ZCompilerMLIRGen
    ${LLVM_RISCV_BACKEND_LIBS}
)
