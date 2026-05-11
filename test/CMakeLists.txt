find_program(BASH_PROGRAM bash REQUIRED)

add_test(
  NAME lexer
  COMMAND ${BASH_PROGRAM}
          ${CMAKE_CURRENT_SOURCE_DIR}/lexer/run.sh
          $<TARGET_FILE:zc>
          ${CMAKE_SOURCE_DIR}
)

add_test(
  NAME parser
  COMMAND ${BASH_PROGRAM}
          ${CMAKE_CURRENT_SOURCE_DIR}/parser/run.sh
          $<TARGET_FILE:zc>
          ${CMAKE_SOURCE_DIR}
)

add_test(
  NAME codegen
  COMMAND ${BASH_PROGRAM}
          ${CMAKE_CURRENT_SOURCE_DIR}/codegen/run.sh
          $<TARGET_FILE:zc>
          ${CMAKE_SOURCE_DIR}
)

add_test(
  NAME dialect
  COMMAND ${BASH_PROGRAM}
          ${CMAKE_CURRENT_SOURCE_DIR}/dialect/run.sh
          $<TARGET_FILE:zc-opt>
          ${CMAKE_SOURCE_DIR}
)

add_test(
  NAME correctness
  COMMAND ${BASH_PROGRAM}
          ${CMAKE_CURRENT_SOURCE_DIR}/correctness/run.sh
          $<TARGET_FILE:zc>
          ${CMAKE_SOURCE_DIR}
)
