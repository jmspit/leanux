cmake_minimum_required(VERSION 3.18)

# base names
set( PROJECT "leanux" )
string(TOUPPER ${PROJECT} PROJECTUC)

project( ${PROJECT} )

execute_process(
    COMMAND git describe --tags --always
    OUTPUT_VARIABLE GIT_TAG
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
message( STATUS "GIT_TAG              : ${GIT_TAG}")

include( cmake/MunkeiVersionFromGit.cmake )

version_from_git(
  LOG       ON
  TIMESTAMP "%Y%m%d%H%M%S"
)

set( ${PROJECT}_VERSION_MAJOR "${VERSION_MAJOR}" )
set( ${PROJECT}_VERSION_MINOR "${VERSION_MINOR}" )
set( ${PROJECT}_VERSION_PATCH "${VERSION_PATCH}" )

# options
option(EXAMPLE_TEST "set to ON to build examples and tests" "OFF")

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
message( STATUS "CMAKE_VERSION          : ${CMAKE_VERSION}")

set(${PROJECTUC}_VERSION_STR "${SEMVER}")
set(${PROJECT}_VERSION_STR "${SEMVER}")
message( STATUS "${PROJECTUC} BUILD VERSION   : ${${PROJECTUC}_VERSION_STR}")
set(${PROJECT}_LIB_NAME "${PROJECT}-${${PROJECT}_VERSION_STR}")
message( STATUS "examples and tests     : ${EXAMPLE_TEST}")

#global constants
set( ${PROJECTUC}_MAINTAINER_NAME "Jan-Marten Spit" )
set( ${PROJECTUC}_MAINTAINER_EMAIL "spitjm@xs4all.nl" )
string(TIMESTAMP ${PROJECTUC}_CURRENT_YEAR "%Y")
set( ${PROJECTUC}_COPYRIGHT "copyright GPL v3, ${${PROJECTUC}_MAINTAINER_NAME} 2015-${${PROJECTUC}_CURRENT_YEAR} <${${PROJECTUC}_MAINTAINER_EMAIL}>" )
set( ${PROJECTUC}_SOFTWARE_REPO "https://github.com/jmspit/${PROJECT}" )
set( ${PROJECTUC}_BUG_URL "${${PROJECTUC}_SOFTWARE_REPO}/issues" )
set( ${PROJECTUC}_SHORT_DESC "${PROJECT} - GNU/Linux performance tools and C++ API" )
set( ${PROJECTUC}_WEBSITE "https://jmspit.github.io/${PROJECT}" )
set( ${PROJECTUC}_SRC_URI_BASE "${${PROJECTUC}_SOFTWARE_REPO}/archive" )
set( ${PROJECTUC}_SRC_URI "${${PROJECTUC}_SRC_URI_BASE}/v${${PROJECT}_VERSION_STR}.tar.gz")

message( STATUS "${PROJECTUC}_MAINTAINER_NAME : ${${PROJECTUC}_MAINTAINER_NAME}")
message( STATUS "${PROJECTUC}_MAINTAINER_EMAIL: ${${PROJECTUC}_MAINTAINER_EMAIL}")
message( STATUS "${PROJECTUC}_SOFTWARE_REPO   : ${${PROJECTUC}_SOFTWARE_REPO}")
message( STATUS "${PROJECTUC}_BUG_URL         : ${${PROJECTUC}_BUG_URL}")
message( STATUS "${PROJECTUC}_WEBSITE         : ${${PROJECTUC}_WEBSITE}")

# configure README
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/INSTALL ${CMAKE_CURRENT_BINARY_DIR}/INSTALL @ONLY)

# include per tool constants
include( tools/lard/lard.cmake )
include( tools/lmon/lmon.cmake )
include( tools/lblk/lblk.cmake )
include( tools/lsys/lsys.cmake )
include( tools/labbix/labbix.cmake )

# for reuse
function(deb_monoinstall)
  if ( ${CMAKE_VERSION} STRLESS "3.3.2" AND ${CPACK_GENERATOR} STREQUAL "DEB" )
    set( ${PROJECTUC}_DEB_MONOINSTALL "1" PARENT_SCOPE)
  else()
    set( ${PROJECTUC}_DEB_MONOINSTALL "0" PARENT_SCOPE)
  endif()
endfunction(deb_monoinstall)

# detect distros and set distro specifics
include(cmake/FindLinuxDistro.cmake)
deb_monoinstall()

message( STATUS "${PROJECTUC}_DEB_MONOINSTALL : ${${PROJECTUC}_DEB_MONOINSTALL}")

# detect sqlite
set( SQLITE3_FIND_QUIETLY 1 )
include(cmake/FindSqlite3.cmake)
if (SQLITE3_FOUND)
  execute_process(
    COMMAND sh -c "grep '#define SQLITE_VERSION_NUMBER' /usr/include/sqlite3.h | awk '{print $3}'"
    OUTPUT_VARIABLE SQLITE_VERSION_NUMBER
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if ( SQLITE_VERSION_NUMBER LESS 3007006 )
    message( FATAL_ERROR " ${PROJECT} requires at least sqlite 3.7.6 (3007006 whilst ${SQLITE_VERSION_NUMBER} provided)")
  endif()
  message( STATUS "SQLITE VERSION NUMBER  : ${SQLITE_VERSION_NUMBER}")
else()
  message( FATAL_ERROR " ${PROJECT} requires sqlite3.")
endif()

# C and C++ compile and link flags.
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D ${PROJECTUC}_DEBUG -std=c99 -Wall")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -std=c99 -fPIC -Wall -fstack-protector-all -Wpointer-sign -Wformat -Wformat-security")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D ${PROJECTUC}_DEBUG -Wall")
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

# shared library
set(${PROJECT}_objects lib/block.cpp
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
add_library (${${PROJECT}_LIB_NAME} SHARED ${${PROJECT}_objects})
target_link_libraries (${${PROJECT}_LIB_NAME} ${SQLITE3_LIBRARY})

# lmon tool
add_executable( lmon tools/lmon/lmon.cpp tools/lmon/history.cpp tools/lmon/lmon_curses.cpp tools/lmon/xdata.cpp tools/lmon/realtime.cpp )
target_link_libraries (lmon ${${PROJECT}_LIB_NAME} )

# lblk tool
add_executable( lblk tools/lblk/lblk.cpp tools/lib/tabular.cpp )
target_link_libraries (lblk ${${PROJECT}_LIB_NAME})

# lsys tool
add_executable( lsys tools/lsys/lsys.cpp tools/lib/tabular.cpp )
target_link_libraries (lsys ${${PROJECT}_LIB_NAME})

# lard tool
include( tools/lard/lard.cmake )
add_executable( lard tools/lard/lar.cpp tools/lard/lar_snap.cpp tools/lard/lar_schema.cpp )
target_link_libraries (lard ${${PROJECT}_LIB_NAME})

# lrep tool
include( tools/lrep/lrep.cmake )
add_executable( lrep tools/lrep/lrep.cpp )
target_link_libraries (lrep ${${PROJECT}_LIB_NAME})

# labbix tool
add_executable( labbix tools/labbix/labbix.cpp )
target_link_libraries (labbix ${${PROJECT}_LIB_NAME})

# setup include directories for compiling
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/lib ${CMAKE_CURRENT_SOURCE_DIR}/tools/lib ${CMAKE_CURRENT_BINARY_DIR} ${SQLITE3_INCLUDE_DIR})

# examples
if ( ${EXAMPLE_TEST} STREQUAL "ON" )
  # generate make test code
  enable_testing()
  set(example-cpu_EXE_NAME "example-cpu-${${PROJECT}_VERSION_STR}")
  add_executable( ${example-cpu_EXE_NAME} examples/example_cpu.cpp  )
  target_link_libraries (${example-cpu_EXE_NAME} ${${PROJECT}_LIB_NAME})
  add_test( ${example-cpu_EXE_NAME} ${example-cpu_EXE_NAME} )

  set(example-process_EXE_NAME "example-process-${${PROJECT}_VERSION_STR}")
  add_executable( ${example-process_EXE_NAME} examples/example_process.cpp  )
  target_link_libraries (${example-process_EXE_NAME} ${${PROJECT}_LIB_NAME})
  add_test( ${example-process_EXE_NAME} ${example-process_EXE_NAME} )

  set(example-process2_EXE_NAME "example-process2-${${PROJECT}_VERSION_STR}")
  add_executable( ${example-process2_EXE_NAME} examples/example_process2.cpp  )
  target_link_libraries (${example-process2_EXE_NAME} ${${PROJECT}_LIB_NAME})
  #add_test( ${example-process2_EXE_NAME} ${example-process2_EXE_NAME} )

  set(example-system_EXE_NAME "example-system-${${PROJECT}_VERSION_STR}")
  add_executable( ${example-system_EXE_NAME} examples/example_system.cpp  )
  target_link_libraries (${example-system_EXE_NAME} ${${PROJECT}_LIB_NAME})
  add_test( ${example-system_EXE_NAME} ${example-system_EXE_NAME} )

  set(example-net_EXE_NAME "example-net-${${PROJECT}_VERSION_STR}")
  add_executable( ${example-net_EXE_NAME} examples/example_net.cpp  )
  target_link_libraries (${example-net_EXE_NAME} ${${PROJECT}_LIB_NAME})
  add_test( ${example-net_EXE_NAME} ${example-net_EXE_NAME} )

  set(example-block_EXE_NAME "example-block-${${PROJECT}_VERSION_STR}")
  add_executable( ${example-block_EXE_NAME} examples/example_block.cpp tools/lib/tabular.cpp  )
  target_link_libraries (${example-block_EXE_NAME} ${${PROJECT}_LIB_NAME})
  add_test( ${example-block_EXE_NAME} ${example-block_EXE_NAME} )

  set(example-pci_EXE_NAME "example-pci-${${PROJECT}_VERSION_STR}")
  add_executable( ${example-pci_EXE_NAME} examples/example_pci.cpp  )
  target_link_libraries (${example-pci_EXE_NAME} ${${PROJECT}_LIB_NAME})
  add_test( ${example-pci_EXE_NAME} ${example-pci_EXE_NAME} )

  set(example-usb_EXE_NAME "example-usb-${${PROJECT}_VERSION_STR}")
  add_executable( ${example-usb_EXE_NAME} examples/example_usb.cpp  )
  target_link_libraries (${example-usb_EXE_NAME} ${${PROJECT}_LIB_NAME})
  add_test( ${example-usb_EXE_NAME} ${example-usb_EXE_NAME} )

  set(example-persist_EXE_NAME "example-persist-${${PROJECT}_VERSION_STR}")
  add_executable( ${example-persist_EXE_NAME} examples/example_persist.cpp  )
  target_link_libraries (${example-persist_EXE_NAME} ${${PROJECT}_LIB_NAME})
  add_test( ${example-persist_EXE_NAME} ${example-persist_EXE_NAME} )
endif()

# we need zlib
find_package(ZLIB REQUIRED)
if (ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
    if ( ${EXAMPLE_TEST} STREQUAL "ON" )
      target_link_libraries(${example-cpu_EXE_NAME} ${ZLIB_LIBRARIES})
      target_link_libraries(${example-process_EXE_NAME} ${ZLIB_LIBRARIES})
      target_link_libraries(${example-process2_EXE_NAME} ${ZLIB_LIBRARIES})
      target_link_libraries(${example-system_EXE_NAME} ${ZLIB_LIBRARIES})
      target_link_libraries(${example-net_EXE_NAME} ${ZLIB_LIBRARIES})
      target_link_libraries(${example-block_EXE_NAME} ${ZLIB_LIBRARIES})
      target_link_libraries(${example-pci_EXE_NAME} ${ZLIB_LIBRARIES})
      target_link_libraries(${example-usb_EXE_NAME} ${ZLIB_LIBRARIES})
      target_link_libraries(${example-persist_EXE_NAME} ${ZLIB_LIBRARIES})
    endif()
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
  #target_link_libraries(lmon ${CURSES_LIBRARIES})
  target_link_libraries(lmon ncurses tinfo )
endif (CURSES_FOUND)

# install binaries
if ( ${${PROJECTUC}_DEB_MONOINSTALL} STREQUAL "1" )
  install(TARGETS lmon DESTINATION ${INSTALL_BIN_PATH})
  install(TARGETS lblk DESTINATION ${INSTALL_BIN_PATH})
  install(TARGETS lsys DESTINATION ${INSTALL_BIN_PATH})
  install(TARGETS lard DESTINATION ${INSTALL_BIN_PATH})
  install(TARGETS lrep DESTINATION ${INSTALL_BIN_PATH})
  install(TARGETS labbix DESTINATION ${INSTALL_BIN_PATH})
  install(TARGETS ${${PROJECT}_LIB_NAME} DESTINATION ${INSTALL_LIB_PATH})
else()
  install(TARGETS lmon DESTINATION ${INSTALL_BIN_PATH} COMPONENT lmon)
  install(TARGETS lblk DESTINATION ${INSTALL_BIN_PATH} COMPONENT lblk)
  install(TARGETS lsys DESTINATION ${INSTALL_BIN_PATH} COMPONENT lsys)
  install(TARGETS lard DESTINATION ${INSTALL_BIN_PATH} COMPONENT lard)
  install(TARGETS lrep DESTINATION ${INSTALL_BIN_PATH} COMPONENT lrep)
  install(TARGETS labbix DESTINATION ${INSTALL_BIN_PATH} COMPONENT labbix)
  install(TARGETS ${${PROJECT}_LIB_NAME} DESTINATION ${INSTALL_LIB_PATH} COMPONENT lib${PROJECT})
endif()

# development headers
if ( ${${PROJECTUC}_DEB_MONOINSTALL} STREQUAL "1" )
  install(FILES lib/block.hpp DESTINATION ${INSTALL_INCLUDE_PATH}/${PROJECT})
  install(FILES lib/cpu.hpp DESTINATION ${INSTALL_INCLUDE_PATH}/${PROJECT})
  install(FILES lib/net.hpp DESTINATION ${INSTALL_INCLUDE_PATH}/${PROJECT})
  install(FILES lib/oops.hpp DESTINATION ${INSTALL_INCLUDE_PATH}/${PROJECT})
  install(FILES lib/pci.hpp DESTINATION ${INSTALL_INCLUDE_PATH}/${PROJECT})
  install(FILES lib/process.hpp DESTINATION ${INSTALL_INCLUDE_PATH}/${PROJECT})
  install(FILES lib/system.hpp DESTINATION ${INSTALL_INCLUDE_PATH}/${PROJECT})
  install(FILES lib/util.hpp DESTINATION ${INSTALL_INCLUDE_PATH}/${PROJECT})
else()
  install(FILES lib/block.hpp DESTINATION ${INSTALL_INCLUDE_PATH}/${PROJECT} COMPONENT lib${PROJECT}-devel)
  install(FILES lib/cpu.hpp DESTINATION ${INSTALL_INCLUDE_PATH}/${PROJECT} COMPONENT lib${PROJECT}-devel)
  install(FILES lib/net.hpp DESTINATION ${INSTALL_INCLUDE_PATH}/${PROJECT} COMPONENT lib${PROJECT}-devel)
  install(FILES lib/oops.hpp DESTINATION ${INSTALL_INCLUDE_PATH}/${PROJECT} COMPONENT lib${PROJECT}-devel)
  install(FILES lib/pci.hpp DESTINATION ${INSTALL_INCLUDE_PATH}/${PROJECT} COMPONENT lib${PROJECT}-devel)
  install(FILES lib/process.hpp DESTINATION ${INSTALL_INCLUDE_PATH}/${PROJECT} COMPONENT lib${PROJECT}-devel)
  install(FILES lib/system.hpp DESTINATION ${INSTALL_INCLUDE_PATH}/${PROJECT} COMPONENT lib${PROJECT}-devel)
  install(FILES lib/util.hpp DESTINATION ${INSTALL_INCLUDE_PATH}/${PROJECT} COMPONENT lib${PROJECT}-devel)
endif()

# lard.conf
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/etc/lard.conf ${CMAKE_CURRENT_BINARY_DIR}/lard.conf @ONLY)

# man pages
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/lmon/lmon.1 ${CMAKE_CURRENT_BINARY_DIR}/lmon.1 @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/lblk/lblk.1 ${CMAKE_CURRENT_BINARY_DIR}/lblk.1 @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/lsys/lsys.1 ${CMAKE_CURRENT_BINARY_DIR}/lsys.1 @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/lard.1 ${CMAKE_CURRENT_BINARY_DIR}/lard.1 @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/lrep/lrep.1 ${CMAKE_CURRENT_BINARY_DIR}/lrep.1 @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/labbix/labbix.1 ${CMAKE_CURRENT_BINARY_DIR}/labbix.1 @ONLY)
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
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/labbix.1.gz
                   COMMAND cat ${CMAKE_CURRENT_BINARY_DIR}/labbix.1 | gzip > ${CMAKE_BINARY_DIR}/labbix.1.gz
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tools/labbix/labbix.1 ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                   COMMENT "preparing man page for labbix"
                   VERBATIM )
add_custom_target(man ALL DEPENDS ${CMAKE_BINARY_DIR}/lmon.1.gz
                                  ${CMAKE_BINARY_DIR}/lblk.1.gz
                                  ${CMAKE_BINARY_DIR}/lsys.1.gz
                                  ${CMAKE_BINARY_DIR}/lard.1.gz
                                  ${CMAKE_BINARY_DIR}/lrep.1.gz
                                  ${CMAKE_BINARY_DIR}/labbix.1.gz)

if ( ${${PROJECTUC}_DEB_MONOINSTALL} STREQUAL "1" )
  install(FILES ${CMAKE_BINARY_DIR}/lmon.1.gz DESTINATION ${MAN_INSTALL_DIR}/man1)
  install(FILES ${CMAKE_BINARY_DIR}/lblk.1.gz DESTINATION ${MAN_INSTALL_DIR}/man1)
  install(FILES ${CMAKE_BINARY_DIR}/lsys.1.gz DESTINATION ${MAN_INSTALL_DIR}/man1)
  install(FILES ${CMAKE_BINARY_DIR}/lard.1.gz DESTINATION ${MAN_INSTALL_DIR}/man1)
  install(FILES ${CMAKE_BINARY_DIR}/lrep.1.gz DESTINATION ${MAN_INSTALL_DIR}/man1)
  install(FILES ${CMAKE_BINARY_DIR}/labbix.1.gz DESTINATION ${MAN_INSTALL_DIR}/man1)
else()
  install(FILES ${CMAKE_BINARY_DIR}/lmon.1.gz DESTINATION ${MAN_INSTALL_DIR}/man1 COMPONENT lmon)
  install(FILES ${CMAKE_BINARY_DIR}/lblk.1.gz DESTINATION ${MAN_INSTALL_DIR}/man1 COMPONENT lblk)
  install(FILES ${CMAKE_BINARY_DIR}/lsys.1.gz DESTINATION ${MAN_INSTALL_DIR}/man1 COMPONENT lsys)
  install(FILES ${CMAKE_BINARY_DIR}/lard.1.gz DESTINATION ${MAN_INSTALL_DIR}/man1 COMPONENT lard)
  install(FILES ${CMAKE_BINARY_DIR}/lrep.1.gz DESTINATION ${MAN_INSTALL_DIR}/man1 COMPONENT lrep)
  install(FILES ${CMAKE_BINARY_DIR}/labbix.1.gz DESTINATION ${MAN_INSTALL_DIR}/man1 COMPONENT labbix)
endif()

set(CPACK_PACKAGE_VERSION ${${PROJECTUC}_VERSION_STR})
set(CPACK_PACKAGE_VERSION_MAJOR ${${PROJECT}_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${${PROJECT}_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${${PROJECT}_VERSION_PATCH})
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT}-${${PROJECTUC}_VERSION_STR}")
set(CPACK_SOURCE_IGNORE_FILES "/build/" "/release/" "/debug/" "/.svn/" "${PROJECT}.geany")
set(CPACK_PACKAGE_CONTACT "${${PROJECTUC}_MAINTAINER_EMAIL}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${${PROJECTUC}_SHORT_DESC})
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_BINARY_DIR}/README.md")
set(CPACK_PACKAGE_VENDOR "${${PROJECTUC}_MAINTAINER_EMAIL}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} )
set( CPACK_RPM_PACKAGE_LICENSE "GPLv3" )
message( STATUS "make package produces ${CPACK_PACKAGE_FILE_NAME}" )
message( STATUS "make package_source produces ${CPACK_SOURCE_PACKAGE_FILE_NAME}" )
set(CPACK_COMPONENTS_ALL lib${PROJECT} lib${PROJECT}-devel lmon lblk lsys lard lrep labbix )
set(CPACK_COMPONENT_INSTALL ON)
set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)

# DEB support in cmake < 3.3.2 lacks per-component postinst en prerm actions
if ( ${${PROJECTUC}_DEB_MONOINSTALL} STREQUAL "1" )
  set(CPACK_DEB_COMPONENT_INSTALL OFF)
else()
  set(CPACK_DEB_COMPONENT_INSTALL ON)
endif()
include(CPack)


# Gentoo ebuild
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT}.ebuild ${CMAKE_CURRENT_BINARY_DIR}/${${PROJECT}_LIB_NAME}.ebuild @ONLY)
message( STATUS "generated ebuild for Gentoo GNU/Linux: ${CMAKE_CURRENT_BINARY_DIR}/${${PROJECT}_LIB_NAME}.ebuild")

# Arch pkg
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT}.PKGBUILD ${CMAKE_CURRENT_BINARY_DIR}/${${PROJECT}_LIB_NAME}.PKGBUILD @ONLY)
message( STATUS "generated PKGBUILD for Arch GNU/Linux: ${CMAKE_CURRENT_BINARY_DIR}/${${PROJECT}_LIB_NAME}.PKGBUILD")

# generate configure header
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT}-config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT}-config.hpp @ONLY)

# Doxygen source code documentation
find_package(Doxygen)
if(DOXYGEN_FOUND)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT}.doxygen ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mainpage.dox.in ${CMAKE_CURRENT_BINARY_DIR}/mainpage.dox @ONLY)
add_custom_target(doc
${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
COMMENT "Generating API documentation with Doxygen" VERBATIM
)
endif(DOXYGEN_FOUND)

# write the osrelease pretty name to a file
file( WRITE osrelease.txt "${OS_RELEASE_PRETTY_NAME}" )
