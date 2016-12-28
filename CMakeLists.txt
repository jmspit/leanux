cmake_minimum_required (VERSION 2.8)
project( leanux )

set( leanux_VERSION_MAJOR "0" )
set( leanux_VERSION_MINOR "0" )
set( leanux_VERSION_PATCH "8" )

if ( ${CMAKE_SIZEOF_VOID_P} STREQUAL "8" )
execute_process(
    COMMAND sh -c "expr ${CMAKE_SIZEOF_VOID_P} \\* 8"
    OUTPUT_VARIABLE BITNESS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
else()
  set( BITNESS "" )
endif()

site_name( SITE_NAME )
message( STATUS "SITE_NAME              : ${SITE_NAME}")
message( STATUS "CMAKE_SYSTEM           : ${CMAKE_SYSTEM}")
message( STATUS "CMAKE_SYSTEM_NAME      : ${CMAKE_SYSTEM_NAME}")
message( STATUS "CMAKE_SYSTEM_PROCESSOR : ${CMAKE_SYSTEM_PROCESSOR}")
message( STATUS "CMAKE_CXX_COMPILER     : ${CMAKE_CXX_COMPILER}")
message( STATUS "CMAKE_HOST_SYSTEM      : ${CMAKE_HOST_SYSTEM}")

set(leanux_VERSION_STR "${leanux_VERSION_MAJOR}.${leanux_VERSION_MINOR}.${leanux_VERSION_PATCH}")
message( STATUS "LEANUX BUILD VERSION   : ${leanux_VERSION_STR}")
set(leanux_LIB_NAME "leanux-${leanux_VERSION_STR}")

#global constants
set( LEANUX_MAINTAINER_NAME "Jan-Marten Spit" )
set( LEANUX_MAINTAINER_EMAIL "spitjm@xs4all.nl" )
set( LEANUX_BUG_URL "https://bugs.o-rho.com" )
set( LEANUX_SHORT_DESC "leanux - GNU/Linux performance tools and C++ API" )
set( LEANUX_WEBSITE "https://www.o-rho.com/leanux")
set( LEANUX_SRC_URI_BASE "https://www.o-rho.com/sites/default/files/project_files")
set( LEANUX_SRC_URI "${LEANUX_SRC_URI_BASE}/leanux-${leanux_VERSION_STR}.tar.gz")

# detect distros and set distro specifics
include( tools/lard/lard.cmake )
include(FindLinuxDistro.cmake)
# detect sqlite
set( SQLITE3_FIND_QUIETLY 1 )
include(FindSqlite3.cmake)
if (SQLITE3_FOUND)
  execute_process(
    COMMAND sh -c "grep '#define SQLITE_VERSION_NUMBER' /usr/include/sqlite3.h | awk '{print $3}'"
    OUTPUT_VARIABLE SQLITE_VERSION_NUMBER
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if ( SQLITE_VERSION_NUMBER LESS 3007006 )
    message( FATAL_ERROR "Leanux requires at least sqlite 3.7.6 (3007006 whilst ${SQLITE_VERSION_NUMBER} provided)")
  endif()
  message( STATUS "SQLITE VERSION NUMBER  : ${SQLITE_VERSION_NUMBER}")
else()
  message( FATAL_ERROR "Leanux requires sqlite3.")
endif()

# C and C++ compile and link flags.
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DLEANUX_DEBUG -std=c99 -Wall")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -std=c99 -fPIC -Wall -fstack-protector-all -Wpointer-sign -Wformat -Wformat-security")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DLEANUX_DEBUG -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -pie -fPIC -Wall -fstack-protector-all -Wformat -Wformat-security")
set(CMAKE_C_FLAGS "-std=c99 -fPIC -Wall -fstack-protector-all -Wpointer-sign -Wformat -Wformat-security")
add_compile_options( -Wall )

# print summary
message( STATUS "CMAKE_BUILD_TYPE       : ${CMAKE_BUILD_TYPE}")
message( STATUS "INSTALL_INCLUDE_PATH   : ${INSTALL_INCLUDE_PATH}")
message( STATUS "INSTALL_LIB_PATH       : ${INSTALL_LIB_PATH}")
message( STATUS "INSTALL_BIN_PATH       : ${INSTALL_BIN_PATH}")
message( STATUS "CMAKE_INSTALL_PREFIX   : ${CMAKE_INSTALL_PREFIX}")
message( STATUS "MAN_INSTALL_DIR        : ${MAN_INSTALL_DIR}")

# generate make test code
enable_testing()

# leanux shared library
set(leanux_objects lib/block.cpp
                   lib/configfile.cpp
                   lib/cpu.cpp
                   lib/device.cpp
                   lib/gzstream.cpp
                   lib/natsort.cpp
                   lib/net.cpp
                   lib/oops.cpp
                   lib/pci.cpp
                   lib/persist.cpp
                   lib/process.cpp
                   lib/system.cpp
                   lib/usb.cpp
                   lib/util.cpp
                   lib/vmem.cpp)
add_library (${leanux_LIB_NAME} SHARED ${leanux_objects})
target_link_libraries (${leanux_LIB_NAME} ${SQLITE3_LIBRARY})

# lmon tool
add_executable( lmon tools/lmon/lmon.cpp tools/lmon/history.cpp tools/lmon/lmon_curses.cpp tools/lmon/xdata.cpp tools/lmon/realtime.cpp libtermkey/driver-csi.c libtermkey/driver-ti.c libtermkey/termkey.c )
target_link_libraries (lmon ${leanux_LIB_NAME})

# lblk tool
add_executable( lblk tools/lblk/lblk.cpp tools/lib/tabular.cpp )
target_link_libraries (lblk ${leanux_LIB_NAME})

# lsys tool
add_executable( lsys tools/lsys/lsys.cpp tools/lib/tabular.cpp )
target_link_libraries (lsys ${leanux_LIB_NAME})

# lard tool
include( tools/lard/lard.cmake )
add_executable( lard tools/lard/lar.cpp tools/lard/lar_snap.cpp tools/lard/lar_schema.cpp )
target_link_libraries (lard ${leanux_LIB_NAME})

# lrep tool
include( tools/lrep/lrep.cmake )
add_executable( lrep tools/lrep/lrep.cpp )
target_link_libraries (lrep ${leanux_LIB_NAME})

# labbix tool
add_executable( labbix tools/labbix/labbix.cpp )
target_link_libraries (labbix ${leanux_LIB_NAME})

# setup include directories for compiling
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/lib ${CMAKE_CURRENT_SOURCE_DIR}/libtermkey ${CMAKE_CURRENT_SOURCE_DIR}/tools/lib ${CMAKE_CURRENT_BINARY_DIR} ${SQLITE3_INCLUDE_DIR})

# examples
set(example-cpu_EXE_NAME "example-cpu-${leanux_VERSION_STR}")
add_executable( ${example-cpu_EXE_NAME} examples/example_cpu.cpp  )
target_link_libraries (${example-cpu_EXE_NAME} ${leanux_LIB_NAME})
add_test( ${example-cpu_EXE_NAME} ${example-cpu_EXE_NAME} )

set(example-process_EXE_NAME "example-process-${leanux_VERSION_STR}")
add_executable( ${example-process_EXE_NAME} examples/example_process.cpp  )
target_link_libraries (${example-process_EXE_NAME} ${leanux_LIB_NAME})
add_test( ${example-process_EXE_NAME} ${example-process_EXE_NAME} )

set(example-process2_EXE_NAME "example-process2-${leanux_VERSION_STR}")
add_executable( ${example-process2_EXE_NAME} examples/example_process2.cpp  )
target_link_libraries (${example-process2_EXE_NAME} ${leanux_LIB_NAME})
#add_test( ${example-process2_EXE_NAME} ${example-process2_EXE_NAME} )

set(example-system_EXE_NAME "example-system-${leanux_VERSION_STR}")
add_executable( ${example-system_EXE_NAME} examples/example_system.cpp  )
target_link_libraries (${example-system_EXE_NAME} ${leanux_LIB_NAME})
add_test( ${example-system_EXE_NAME} ${example-system_EXE_NAME} )

set(example-net_EXE_NAME "example-net-${leanux_VERSION_STR}")
add_executable( ${example-net_EXE_NAME} examples/example_net.cpp  )
target_link_libraries (${example-net_EXE_NAME} ${leanux_LIB_NAME})
add_test( ${example-net_EXE_NAME} ${example-net_EXE_NAME} )

set(example-block_EXE_NAME "example-block-${leanux_VERSION_STR}")
add_executable( ${example-block_EXE_NAME} examples/example_block.cpp tools/lib/tabular.cpp  )
target_link_libraries (${example-block_EXE_NAME} ${leanux_LIB_NAME})
add_test( ${example-block_EXE_NAME} ${example-block_EXE_NAME} )

set(example-pci_EXE_NAME "example-pci-${leanux_VERSION_STR}")
add_executable( ${example-pci_EXE_NAME} examples/example_pci.cpp  )
target_link_libraries (${example-pci_EXE_NAME} ${leanux_LIB_NAME})
add_test( ${example-pci_EXE_NAME} ${example-pci_EXE_NAME} )

set(example-usb_EXE_NAME "example-usb-${leanux_VERSION_STR}")
add_executable( ${example-usb_EXE_NAME} examples/example_usb.cpp  )
target_link_libraries (${example-usb_EXE_NAME} ${leanux_LIB_NAME})
add_test( ${example-usb_EXE_NAME} ${example-usb_EXE_NAME} )

set(example-persist_EXE_NAME "example-persist-${leanux_VERSION_STR}")
add_executable( ${example-persist_EXE_NAME} examples/example_persist.cpp  )
target_link_libraries (${example-persist_EXE_NAME} ${leanux_LIB_NAME})
add_test( ${example-persist_EXE_NAME} ${example-persist_EXE_NAME} )

# we need zlib
find_package(ZLIB REQUIRED)
if (ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
    target_link_libraries(${example-cpu_EXE_NAME} ${ZLIB_LIBRARIES})
    target_link_libraries(${example-process_EXE_NAME} ${ZLIB_LIBRARIES})
    target_link_libraries(${example-process2_EXE_NAME} ${ZLIB_LIBRARIES})
    target_link_libraries(${example-system_EXE_NAME} ${ZLIB_LIBRARIES})
    target_link_libraries(${example-net_EXE_NAME} ${ZLIB_LIBRARIES})
    target_link_libraries(${example-block_EXE_NAME} ${ZLIB_LIBRARIES})
    target_link_libraries(${example-pci_EXE_NAME} ${ZLIB_LIBRARIES})
    target_link_libraries(${example-usb_EXE_NAME} ${ZLIB_LIBRARIES})
    target_link_libraries(${example-persist_EXE_NAME} ${ZLIB_LIBRARIES})
    target_link_libraries(lmon ${ZLIB_LIBRARIES})
    target_link_libraries(lblk ${ZLIB_LIBRARIES})
    target_link_libraries(lsys ${ZLIB_LIBRARIES})
    target_link_libraries(lard ${ZLIB_LIBRARIES})
    target_link_libraries(lrep ${ZLIB_LIBRARIES})
    target_link_libraries(labbix ${ZLIB_LIBRARIES})
endif()

# we need ncurses
find_package(Curses REQUIRED)
if (CURSES_FOUND)
  #message( STATUS "curses is " ${CURSES_LIBRARY})
  target_link_libraries(lmon ${CURSES_LIBRARIES})
endif (CURSES_FOUND)

# install binaries
install(TARGETS lmon DESTINATION ${INSTALL_BIN_PATH})
install(TARGETS lblk DESTINATION ${INSTALL_BIN_PATH})
install(TARGETS lsys DESTINATION ${INSTALL_BIN_PATH})
install(TARGETS lard DESTINATION ${INSTALL_BIN_PATH})
install(TARGETS lrep DESTINATION ${INSTALL_BIN_PATH})
install(TARGETS labbix DESTINATION ${INSTALL_BIN_PATH})
install(TARGETS ${leanux_LIB_NAME} DESTINATION ${INSTALL_LIB_PATH})

# will be part of a separate devl package
#install(FILES lib/block.hpp DESTINATION ${INSTALL_INCLUDE_PATH}/leanux)
#install(FILES lib/cpu.hpp DESTINATION ${INSTALL_INCLUDE_PATH}/leanux)
#install(FILES lib/net.hpp DESTINATION ${INSTALL_INCLUDE_PATH}/leanux)
#install(FILES lib/oops.hpp DESTINATION ${INSTALL_INCLUDE_PATH}/leanux)
#install(FILES lib/pci.hpp DESTINATION ${INSTALL_INCLUDE_PATH}/leanux)
#install(FILES lib/process.hpp DESTINATION ${INSTALL_INCLUDE_PATH}/leanux)
#install(FILES lib/system.hpp DESTINATION ${INSTALL_INCLUDE_PATH}/leanux)
#install(FILES lib/util.hpp DESTINATION ${INSTALL_INCLUDE_PATH}/leanux)

# lard.conf
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/etc/lard.conf ${CMAKE_CURRENT_BINARY_DIR}/lard.conf @ONLY)

# man pages
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/lmon/lmon.1 ${CMAKE_CURRENT_BINARY_DIR}/lmon.1 @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/lblk/lblk.1 ${CMAKE_CURRENT_BINARY_DIR}/lblk.1 @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/lsys/lsys.1 ${CMAKE_CURRENT_BINARY_DIR}/lsys.1 @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/lard.1 ${CMAKE_CURRENT_BINARY_DIR}/lard.1 @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/lrep/lrep.1 ${CMAKE_CURRENT_BINARY_DIR}/lrep.1 @ONLY)
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/lmon.1.gz
                   COMMAND cat ${CMAKE_CURRENT_BINARY_DIR}/lmon.1 | gzip > ${CMAKE_BINARY_DIR}/lmon.1.gz
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/lmon/lmon.1 ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                   COMMENT "preparing man page for lmon"
                   VERBATIM )
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/lblk.1.gz
                   COMMAND cat ${CMAKE_CURRENT_BINARY_DIR}/lblk.1 | gzip > ${CMAKE_BINARY_DIR}/lblk.1.gz
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/lblk/lblk.1 ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                   COMMENT "preparing man page for lblk"
                   VERBATIM )
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/lsys.1.gz
                   COMMAND cat ${CMAKE_CURRENT_BINARY_DIR}/lsys.1 | gzip > ${CMAKE_BINARY_DIR}/lsys.1.gz
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/lsys/lsys.1 ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                   COMMENT "preparing man page for lsys"
                   VERBATIM )
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/lard.1.gz
                   COMMAND cat ${CMAKE_CURRENT_BINARY_DIR}/lard.1 | gzip > ${CMAKE_BINARY_DIR}/lard.1.gz
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/lard.1 ${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/lard.cmake ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                   COMMENT "preparing man page for lard"
                   VERBATIM )
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/lrep.1.gz
                   COMMAND cat ${CMAKE_CURRENT_BINARY_DIR}/lrep.1 | gzip > ${CMAKE_BINARY_DIR}/lrep.1.gz
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/lrep/lrep.1 ${CMAKE_CURRENT_SOURCE_DIR}/tools/lrep/lrep.cmake ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                   COMMENT "preparing man page for lrep"
                   VERBATIM )
add_custom_target(man ALL DEPENDS ${CMAKE_BINARY_DIR}/lmon.1.gz ${CMAKE_BINARY_DIR}/lblk.1.gz ${CMAKE_BINARY_DIR}/lsys.1.gz ${CMAKE_BINARY_DIR}/lard.1.gz ${CMAKE_BINARY_DIR}/lrep.1.gz)
install(FILES ${CMAKE_BINARY_DIR}/lmon.1.gz DESTINATION ${MAN_INSTALL_DIR}/man1)
install(FILES ${CMAKE_BINARY_DIR}/lblk.1.gz DESTINATION ${MAN_INSTALL_DIR}/man1)
install(FILES ${CMAKE_BINARY_DIR}/lsys.1.gz DESTINATION ${MAN_INSTALL_DIR}/man1)
install(FILES ${CMAKE_BINARY_DIR}/lard.1.gz DESTINATION ${MAN_INSTALL_DIR}/man1)
install(FILES ${CMAKE_BINARY_DIR}/lrep.1.gz DESTINATION ${MAN_INSTALL_DIR}/man1)

#set(CPACK_PACKAGE_VERSION ${leanux_VERSION_STR})
set(CPACK_PACKAGE_VERSION_MAJOR ${leanux_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${leanux_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${leanux_VERSION_PATCH})
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "leanux-${leanux_VERSION_STR}")
set(CPACK_SOURCE_IGNORE_FILES "/build/" "/release/" "/debug/" "/.svn/" "leanux.geany")
set(CPACK_PACKAGE_CONTACT "${LEANUX_MAINTAINER_EMAIL}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${LEANUX_SHORT_DESC})
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
set(CPACK_PACKAGE_VENDOR "${LEANUX_MAINTAINER_EMAIL}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} )
set( CPACK_RPM_PACKAGE_LICENSE "GPLv3" )
message( STATUS "make package produces ${CPACK_PACKAGE_FILE_NAME}" )
include(CPack)


# Gentoo ebuild
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/leanux.ebuild ${CMAKE_CURRENT_BINARY_DIR}/${leanux_LIB_NAME}.ebuild @ONLY)
message( STATUS "generated ebuild for Gentoo GNU/Linux: ${CMAKE_CURRENT_BINARY_DIR}/${leanux_LIB_NAME}.ebuild")

# Arch pkg
execute_process(
  COMMAND sh -c "sha512sum ${CPACK_SOURCE_PACKAGE_FILE_NAME}.tar.gz|cut -d\\  -f1"
  OUTPUT_VARIABLE LEANUX_SOURCE_SHA512SUM
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/leanux.PKGBUILD ${CMAKE_CURRENT_BINARY_DIR}/${leanux_LIB_NAME}.PKGBUILD @ONLY)
message( STATUS "generated PKGBUILD for Arch GNU/Linux: ${CMAKE_CURRENT_BINARY_DIR}/${leanux_LIB_NAME}.PKGBUILD")

# generate configure header
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/leanux-config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/leanux-config.hpp @ONLY)

# Doxygen source code documentation
find_package(Doxygen)
if(DOXYGEN_FOUND)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/leanux.doxygen ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mainpage.dox.in ${CMAKE_CURRENT_BINARY_DIR}/mainpage.dox @ONLY)
add_custom_target(doc
${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
COMMENT "Generating API documentation with Doxygen" VERBATIM
)
endif(DOXYGEN_FOUND)

