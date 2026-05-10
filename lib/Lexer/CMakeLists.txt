add_library(ZCompilerLexer
  Lexer.cpp
  Token.cpp
)

llvm_update_compile_flags(ZCompilerLexer)

target_include_directories(ZCompilerLexer
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${LLVM_INCLUDE_DIRS}
)

target_compile_definitions(ZCompilerLexer
  PRIVATE
    ${LLVM_DEFINITIONS}
)

llvm_map_components_to_libnames(LLVM_LEXER_LIBS
  support
)

target_link_libraries(ZCompilerLexer
  PUBLIC
    ${LLVM_LEXER_LIBS}
)

