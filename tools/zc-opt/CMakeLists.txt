add_llvm_executable(zc-opt
  zc-opt.cpp
)

llvm_update_compile_flags(zc-opt)

target_link_libraries(zc-opt
  PRIVATE
    ZCompilerZCToStandard
    ZCompilerZCDialect
    MLIRArithDialect
    MLIRFuncDialect
    MLIROptLib
    MLIRRegisterAllPasses
)

mlir_check_all_link_libraries(zc-opt)
