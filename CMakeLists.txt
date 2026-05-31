cmake_minimum_required(VERSION 3.16)
project(libpqcap VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(pqcap
  src/pqcap_error.c
  src/pqcap_util.c
  src/pqcap_footer.c
  src/pqcap_extract.c
  src/pqcap_embed.c
  src/pqcap_scan.c
  src/pqcap_io.c
)

target_include_directories(pqcap PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_include_directories(pqcap PRIVATE src)

add_executable(test_footer tests/test_footer.c)
target_link_libraries(test_footer PRIVATE pqcap)

add_executable(test_embed tests/test_embed_roundtrip.c)
target_link_libraries(test_embed PRIVATE pqcap)

add_executable(test_scan tests/test_scan.c)
target_link_libraries(test_scan PRIVATE pqcap)

add_executable(test_parquet_flexibility tests/test_parquet_flexibility.c)
target_link_libraries(test_parquet_flexibility PRIVATE pqcap)

add_executable(test_fixture_embed tests/test_fixture_embed.c)
target_link_libraries(test_fixture_embed PRIVATE pqcap)

add_executable(pqcap_embed_cli tests/pqcap_embed_cli.c)
target_link_libraries(pqcap_embed_cli PRIVATE pqcap)

enable_testing()
add_test(NAME footer COMMAND test_footer)
add_test(NAME embed COMMAND test_embed)
add_test(NAME scan COMMAND test_scan)
add_test(NAME parquet_flexibility COMMAND test_parquet_flexibility)
add_test(NAME fixture_embed COMMAND test_fixture_embed
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME integration_pqcap_validate
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration_pqcap_validate.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set_tests_properties(integration_pqcap_validate PROPERTIES
  ENVIRONMENT "LIBPQCAP_BUILD_DIR=${CMAKE_BINARY_DIR}")

install(TARGETS pqcap
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
install(FILES include/pqcap.h DESTINATION include)
