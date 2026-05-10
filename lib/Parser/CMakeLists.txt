add_library(ZCompilerParser
  Parser.cpp
)

llvm_update_compile_flags(ZCompilerParser)

target_include_directories(ZCompilerParser
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${LLVM_INCLUDE_DIRS}
)

target_compile_definitions(ZCompilerParser
  PRIVATE
    ${LLVM_DEFINITIONS}
)

llvm_map_components_to_libnames(LLVM_PARSER_LIBS
  support
)

target_link_libraries(ZCompilerParser
  PUBLIC
    ZCompilerAST
    ZCompilerLexer
    ${LLVM_PARSER_LIBS}
)

