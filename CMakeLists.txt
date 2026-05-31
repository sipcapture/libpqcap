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

enable_testing()
add_test(NAME footer COMMAND test_footer)
add_test(NAME embed COMMAND test_embed)
add_test(NAME scan COMMAND test_scan)

install(TARGETS pqcap
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
install(FILES include/pqcap.h DESTINATION include)
