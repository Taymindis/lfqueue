cmake_minimum_required(VERSION 2.8)
project(lfqueue)

set(PROJNAME lfqueue)

IF (NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)
SET(CMAKE_INSTALL_INCLUDEDIR /usr/local/include)
ENDIF(NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)

file(GLOB_RECURSE sources lfqueue.c lfqueue.h)
file(GLOB_RECURSE sources_test test.cpp)
file(GLOB_RECURSE example_test lfqueue.c lfqueue.h example.c)

enable_testing()
add_executable(lfqueue-example ${example_test})
target_link_libraries(lfqueue-example PUBLIC pthread)

include_directories(src /usr/local/include)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall -Wstrict-prototypes -Wmissing-prototypes")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-declarations -Wshadow -Wpointer-arith -Wcast-qual")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wsign-compare -std=c11 -pedantic")


###############################################################################
## packaging ##################################################################
###############################################################################
add_library(${PROJNAME}.static STATIC ${sources})
target_link_libraries(${PROJNAME}.static)
set_target_properties(${PROJNAME}.static PROPERTIES OUTPUT_NAME ${PROJNAME})
add_library(${PROJNAME} SHARED ${sources})
target_link_libraries(${PROJNAME})
add_test (NAME lfqueue_test COMMAND lfqueue-example)


# link_directories(/usr/local/lib /usr/lib)

## For .a library
install(TARGETS ${PROJNAME}.static EXPORT ${PROJNAME}
    ARCHIVE DESTINATION lib${LIB_SUFFIX}
    LIBRARY DESTINATION lib${LIB_SUFFIX}
    # RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

## For .so library
install(TARGETS ${PROJNAME} EXPORT ${PROJNAME}
    ARCHIVE DESTINATION lib${LIB_SUFFIX}
    LIBRARY DESTINATION lib${LIB_SUFFIX}
    # RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
install(FILES lfqueue.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/lfsaq)

install(CODE "execute_process(COMMAND ldconfig)")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

set(CPACK_PACKAGE_NAME "lfqueue")
set(CPACK_PACKAGE_VERSION "1.0.0")

# we don't want to split our program up into several things
set(CPACK_MONOLITHIC_INSTALL 1)

# This must be last
include(CPack)
