#sane defaults
set( CMAKE_INSTALL_PREFIX "/usr" )
set( INSTALL_INCLUDE_PATH "include" )
set( INSTALL_LIB_PATH "lib${BITNESS}" )
set( INSTALL_BIN_PATH "bin" )
set( MAN_INSTALL_DIR "/usr/share/man")

set( CPACK_GENERATOR "TGZ" )
set( CPACK_RPM_PACKAGE_GROUP "Applications/System" )
set( CPACK_DEBIAN_PACKAGE_SECTION "Utilities" )

set( DIST_PKG_ZLIB "zlib" )
set( DIST_PKG_NCURSES "ncurses" )
set( DIST_PKG_SQLITE3 "sqlite3" )
set( DIST_PKG_PCIIDS "pciutils" )
set( DIST_PKG_USBIDS "usbutils" )
set( DIST_PKG_OUI "hwdata" )

if ( EXISTS "/etc/os-release" )
  execute_process(
    COMMAND sh -c "cat /etc/os-release | grep '^ID=' | cut -d= -f2 | sed 's,\",,g'"
    OUTPUT_VARIABLE OS_RELEASE_ID
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  ###############################
  # Gentoo
  ###############################
  if ( ${OS_RELEASE_ID} STREQUAL "gentoo" )
    execute_process(
      COMMAND sh -c "cat /etc/gentoo-release | sed 's,\",,g'"
      OUTPUT_VARIABLE OS_RELEASE_PRETTY_NAME
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/gentoo/init.d/lard ${CMAKE_CURRENT_BINARY_DIR}/init.d/lard @ONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/gentoo/conf.d/lard ${CMAKE_CURRENT_BINARY_DIR}/conf.d/lard @ONLY)
    set( CPACK_GENERATOR "TGZ" )
    set( MAN_INSTALL_DIR "share/man" )
    set( OS_RELEASE_TAG "${OS_RELEASE_ID}" )

  ###############################
  # RHEL (7 supports /etc/os-release)
  ###############################
  elseif( ${OS_RELEASE_ID} STREQUAL "rhel" )
    execute_process(
      COMMAND sh -c "cat /etc/os-release | grep 'PRETTY_NAME=' | cut -d= -f2 | sed 's,\",,g' | sed 's,(,,g' | sed 's,),,g' | sed 's, ,_,g' "
      OUTPUT_VARIABLE OS_RELEASE_PRETTY_NAME
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
      COMMAND sh -c "cat /etc/os-release | grep 'VERSION_ID=' | cut -d= -f2 | sed 's,\",,g'"
      OUTPUT_VARIABLE OS_RELEASE_VERSION_ID
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/centos/init.d/lard ${CMAKE_CURRENT_BINARY_DIR}/init.d/lard @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lard.conf DESTINATION "/etc/lard" COMPONENT lard)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/init.d/lard DESTINATION "/etc/init.d" COMPONENT lard)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/centos/postinstall.sh" ${CMAKE_CURRENT_BINARY_DIR}/postinstall.sh @ONLY)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/centos/preuninstall.sh" ${CMAKE_CURRENT_BINARY_DIR}/preuninstall.sh @ONLY)
    set( CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION /usr /usr/bin /usr/include /usr/lib64 /usr/share /usr/share/man /usr/share/man/man1 )
    set( CPACK_GENERATOR "RPM" )
    set( OS_RELEASE_TAG "el${OS_RELEASE_VERSION_ID}" )
    set(DIST_PKG_SQLITE3 "sqlite")

  ###############################
  # openSUSE
  ###############################
  elseif( ${OS_RELEASE_ID} STREQUAL "opensuse" )
    execute_process(
      COMMAND sh -c "cat /etc/os-release | grep 'PRETTY_NAME=' | cut -d= -f2 | sed 's,\",,g' | sed 's,(,,g' | sed 's,),,g' | sed 's, ,_,g' "
      OUTPUT_VARIABLE OS_RELEASE_PRETTY_NAME
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
      COMMAND sh -c "cat /etc/os-release | grep 'VERSION_ID=' | cut -d= -f2 | sed 's,\",,g'"
      OUTPUT_VARIABLE OS_RELEASE_VERSION_ID
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/debian/lard.service ${CMAKE_CURRENT_BINARY_DIR}/lard.service @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lard.conf DESTINATION "/etc/lard" COMPONENT lard)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lard.service DESTINATION "/etc/systemd/system" COMPONENT lard)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/opensuse/postinstall.sh" ${CMAKE_CURRENT_BINARY_DIR}/postinstall.sh @ONLY)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/opensuse/preuninstall.sh" ${CMAKE_CURRENT_BINARY_DIR}/preuninstall.sh @ONLY)
    set( CPACK_RPM_lard_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_BINARY_DIR}/postinstall.sh" )
    set( CPACK_RPM_lard_PRE_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_BINARY_DIR}/preuninstall.sh" )

    set( OS_RELEASE_TAG "${OS_RELEASE_ID}_${OS_RELEASE_VERSION_ID}" )
    set(CPACK_GENERATOR "RPM")
    set( DIST_PKG_ZLIB "libz1" )
    set( DIST_PKG_NCURSES "libncurses5" )

  ###############################
  # Ubuntu
  ###############################
  elseif( ${OS_RELEASE_ID} STREQUAL "ubuntu" )
    execute_process(
      COMMAND sh -c "cat /etc/os-release | grep 'PRETTY_NAME=' | cut -d= -f2 | sed 's,\",,g' | sed 's,(,,g' | sed 's,),,g' | sed 's, ,_,g' "
      OUTPUT_VARIABLE OS_RELEASE_PRETTY_NAME
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
      COMMAND sh -c "cat /etc/os-release | grep 'VERSION_ID=' | cut -d= -f2 | sed 's,\",,g'"
      OUTPUT_VARIABLE OS_RELEASE_VERSION_ID
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set( CPACK_GENERATOR "DEB")
    deb_monoinstall()
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/ubuntu/init.d/lard ${CMAKE_CURRENT_BINARY_DIR}/init.d/lard @ONLY)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/ubuntu/postinst" ${CMAKE_CURRENT_BINARY_DIR}/postinst @ONLY)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/ubuntu/prerm" ${CMAKE_CURRENT_BINARY_DIR}/prerm @ONLY)
    if ( ${${PROJECTUC}_DEB_MONOINSTALL} STREQUAL "1" )
      install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lard.conf DESTINATION "/etc/lard")
      install(FILES ${CMAKE_CURRENT_BINARY_DIR}/init.d/lard DESTINATION "/etc/init.d")
      set( CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_BINARY_DIR}/postinst;${CMAKE_CURRENT_BINARY_DIR}/prerm;")
    else()
      install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lard.conf DESTINATION "/etc/lard" COMPONENT lard)
      install(FILES ${CMAKE_CURRENT_BINARY_DIR}/init.d/lard DESTINATION "/etc/init.d" COMPONENT lard)
      set( CPACK_DEBIAN_LARD_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_BINARY_DIR}/postinst;${CMAKE_CURRENT_BINARY_DIR}/prerm;")
    endif()
    set( OS_RELEASE_TAG "${OS_RELEASE_ID}_${OS_RELEASE_VERSION_ID}" )
    set( INSTALL_LIB_PATH "lib" )
    set( DIST_PKG_ZLIB "zlib1g" )
    set( DIST_PKG_NCURSES "libncurses5" )
    set( DIST_PKG_OUI "ieee-data" )

  ###############################
  # Debian
  ###############################
  elseif( ${OS_RELEASE_ID} STREQUAL "debian" )
    execute_process(
      COMMAND sh -c "cat /etc/os-release | grep 'PRETTY_NAME=' | cut -d= -f2 | sed 's,\",,g' | sed 's,(,,g' | sed 's,),,g' | sed 's, ,_,g' "
      OUTPUT_VARIABLE OS_RELEASE_PRETTY_NAME
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
      COMMAND sh -c "cat /etc/os-release | grep 'VERSION_ID=' | cut -d= -f2 | sed 's,\",,g'"
      OUTPUT_VARIABLE OS_RELEASE_VERSION_ID
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set( CPACK_GENERATOR "DEB")
    deb_monoinstall()
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/debian/lard.service ${CMAKE_CURRENT_BINARY_DIR}/lard.service @ONLY)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/debian/postinst" ${CMAKE_CURRENT_BINARY_DIR}/postinst @ONLY)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/debian/prerm" ${CMAKE_CURRENT_BINARY_DIR}/prerm @ONLY)
    if ( ${${PROJECTUC}_DEB_MONOINSTALL} STREQUAL "1" )
      install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lard.conf DESTINATION "/etc/lard")
      install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lard.service DESTINATION "/etc/systemd/system")
      set( CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_BINARY_DIR}/postinst;${CMAKE_CURRENT_BINARY_DIR}/prerm;")
    else()
      install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lard.conf DESTINATION "/etc/lard" COMPONENT lard)
      install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lard.service DESTINATION "/etc/systemd/system" COMPONENT lard)
      set( CPACK_DEBIAN_LARD_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_BINARY_DIR}/postinst;${CMAKE_CURRENT_BINARY_DIR}/prerm;")
    endif()
    set( OS_RELEASE_TAG "${OS_RELEASE_ID}_${OS_RELEASE_VERSION_ID}" )
    set( INSTALL_LIB_PATH "lib" )
    set( DIST_PKG_ZLIB "zlib1g" )
    set( DIST_PKG_NCURSES "libncurses5" )
    set( DIST_PKG_OUI "ieee-data" )

  ###############################
  # CENTOS
  ###############################
  elseif( ${OS_RELEASE_ID} STREQUAL "centos" )
    execute_process(
      COMMAND sh -c "cat /etc/os-release | grep 'PRETTY_NAME=' | cut -d= -f2 | sed 's,\",,g' | sed 's,(,,g' | sed 's,),,g' | sed 's, ,_,g' "
      OUTPUT_VARIABLE OS_RELEASE_PRETTY_NAME
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
      COMMAND sh -c "cat /etc/os-release | grep 'VERSION_ID=' | cut -d= -f2 | sed 's,\",,g'"
      OUTPUT_VARIABLE OS_RELEASE_VERSION_ID
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/centos/lard.service ${CMAKE_CURRENT_BINARY_DIR}/lard.service @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lard.conf DESTINATION "/etc/lard" COMPONENT lard)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lard.service DESTINATION "/etc/systemd/system" COMPONENT lard)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/centos/postinstall.sh" ${CMAKE_CURRENT_BINARY_DIR}/postinstall.sh @ONLY)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/centos/preuninstall.sh" ${CMAKE_CURRENT_BINARY_DIR}/preuninstall.sh @ONLY)
    set( CPACK_RPM_lard_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_BINARY_DIR}/postinstall.sh" )
    set( CPACK_RPM_lard_PRE_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_BINARY_DIR}/preuninstall.sh" )
    set( CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION /usr /usr/bin /usr/include /usr/lib64 /usr/share /usr/share/man /usr/share/man/man1 /etc/systemd /etc/systemd/system )
    set( CPACK_GENERATOR "RPM" )
    set( OS_RELEASE_TAG "el${OS_RELEASE_VERSION_ID}" )
    set(DIST_PKG_SQLITE3 "sqlite")
  ###############################
  # FEDORA
  ###############################
  elseif( ${OS_RELEASE_ID} STREQUAL "fedora" )
    execute_process(
      COMMAND sh -c "cat /etc/os-release | grep 'PRETTY_NAME=' | cut -d= -f2 | sed 's,\",,g' | sed 's,(,,g' | sed 's,),,g' | sed 's, ,_,g' "
      OUTPUT_VARIABLE OS_RELEASE_PRETTY_NAME
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
      COMMAND sh -c "cat /etc/os-release | grep 'VERSION_ID=' | cut -d= -f2 | sed 's,\",,g'"
      OUTPUT_VARIABLE OS_RELEASE_VERSION_ID
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/fedora/lard.service ${CMAKE_CURRENT_BINARY_DIR}/lard.service @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lard.conf DESTINATION "/etc/lard" COMPONENT lard)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lard.service DESTINATION "/etc/systemd/system" COMPONENT lard)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/fedora/postinstall.sh" ${CMAKE_CURRENT_BINARY_DIR}/postinstall.sh @ONLY)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/fedora/preuninstall.sh" ${CMAKE_CURRENT_BINARY_DIR}/preuninstall.sh @ONLY)
    set( CPACK_RPM_lard_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_BINARY_DIR}/postinstall.sh" )
    set( CPACK_RPM_lard_PRE_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_BINARY_DIR}/preuninstall.sh" )
    set( CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION /usr /usr/bin /usr/include /usr/lib64 /usr/share /usr/share/man /usr/share/man/man1 /etc/systemd /etc/systemd/system )
    set( OS_RELEASE_TAG "${OS_RELEASE_ID}_${OS_RELEASE_VERSION_ID}" )
    set(CPACK_GENERATOR "RPM")
    set(DIST_PKG_SQLITE3 "sqlite-libs")
  ###############################
  # Arch Linux
  ###############################
  elseif( ${OS_RELEASE_ID} STREQUAL "arch" )
    execute_process(
      COMMAND sh -c "cat /etc/os-release | grep 'PRETTY_NAME=' | cut -d= -f2 | sed 's,\",,g'"
      OUTPUT_VARIABLE OS_RELEASE_PRETTY_NAME
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set( OS_RELEASE_TAG "${OS_RELEASE_ID}_${OS_RELEASE_VERSION_ID}" )
    set(CPACK_GENERATOR "TGZ")
    set( MAN_INSTALL_DIR "man" )
    set( OS_RELEASE_TAG "Arch" )
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/archlinux/leanux.install ${CMAKE_CURRENT_BINARY_DIR}/leanux.install @ONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tools/lard/archlinux/lard.service ${CMAKE_CURRENT_BINARY_DIR}/lard.service @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lard.service DESTINATION "/etc/systemd/system")
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lard.conf DESTINATION "/etc/lard")
  else()
    execute_process(
      COMMAND sh -c "cat /etc/os-release | grep 'PRETTY_NAME=' | cut -d= -f2 | sed 's,\",,g'"
      OUTPUT_VARIABLE OS_RELEASE_PRETTY_NAME
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  endif()

###############################
# SLES
###############################
elseif ( EXISTS "/etc/SuSE-release" )
  set( OS_RELEASE_ID "sles" )
  execute_process(
    COMMAND sh -c "head -1 /etc/SuSE-release | awk -F\\( '{print $1}'"
    OUTPUT_VARIABLE OS_RELEASE_PRETTY_NAME
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  execute_process(
    COMMAND sh -c "cat /etc/SuSE-release | grep 'VERSION' | awk '{print $3}'"
    OUTPUT_VARIABLE OS_RELEASE_MAJOR
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  execute_process(
    COMMAND sh -c "cat /etc/SuSE-release | grep 'PATCHLEVEL' | awk '{print $3}'"
    OUTPUT_VARIABLE OS_RELEASE_MINOR
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  set(CPACK_SET_DESTDIR "ON")
  set(OS_RELEASE_TAG "SLES_${OS_RELEASE_MAJOR}_${OS_RELEASE_MINOR}_${CMAKE_SYSTEM_PROCESSOR}" )
  set(CPACK_GENERATOR "RPM")

###############################
# RED HAT
###############################
elseif ( EXISTS "/etc/redhat-release" )
  set( OS_RELEASE_ID "rhel" )
  execute_process(
    COMMAND sh -c "cat /etc/redhat-release | awk '{print $1\" \"$2\" \"$3\" \"$4\" \"$5}'"
    OUTPUT_VARIABLE OS_RELEASE_PRETTY_NAME
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  execute_process(
    COMMAND sh -c "cat /etc/redhat-release | awk '{print $7}' | cut -d\. -f1"
    OUTPUT_VARIABLE RHEL_MAJOR
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  set( CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION /usr /usr/bin /usr/include /usr/lib64 /usr/share /usr/share/man /usr/share/man/man1 )
  set( CPACK_GENERATOR "RPM" )
  set( OS_RELEASE_TAG "el${RHEL_MAJOR}" )
else()
  message(STATUS "GNU/Linux distribution not recognized")
endif()

set(CPACK_PACKAGE_FILE_NAME "${PROJECT}-${${PROJECT}_VERSION_STR}.${OS_RELEASE_TAG}.${CMAKE_SYSTEM_PROCESSOR}")

if ( ${CPACK_GENERATOR} STREQUAL "RPM" )
  set( CPACK_RPM_PACKAGE_REQUIRES "${DIST_PKG_NCURSES}, ${DIST_PKG_ZLIB}, ${DIST_PKG_SQLITE3} >= 3.7.0, ${DIST_PKG_PCIIDS}, ${DIST_PKG_USBIDS}, ${DIST_PKG_OUI}" )
  set( CPACK_RPM_lard_PACKAGE_REQUIRES "${PROJECT}-lib${PROJECT}" )
  set( CPACK_RPM_lmon_PACKAGE_REQUIRES "${PROJECT}-lib${PROJECT}" )
  set( CPACK_RPM_lsys_PACKAGE_REQUIRES "${PROJECT}-lib${PROJECT}" )
  set( CPACK_RPM_lblk_PACKAGE_REQUIRES "${PROJECT}-lib${PROJECT}" )
  set( CPACK_RPM_lrep_PACKAGE_REQUIRES "${PROJECT}-lib${PROJECT}" )
  set( CPACK_RPM_labbix_PACKAGE_REQUIRES "${PROJECT}-lib${PROJECT}" )
elseif ( ${CPACK_GENERATOR} STREQUAL "DEB" )
  set( CPACK_DEBIAN_PACKAGE_DEPENDS "${DIST_PKG_NCURSES}, ${DIST_PKG_ZLIB}, ${DIST_PKG_SQLITE3} >= 3.7.0, ${DIST_PKG_PCIIDS}, ${DIST_PKG_USBIDS}, ${DIST_PKG_OUI}" )
  if ( ${${PROJECTUC}_DEB_MONOINSTALL} STREQUAL "1" )
    # install everything, so no interdependencies
  else()
    set( CPACK_DEBIAN_lard_PACKAGE_DEPENDS "${PROJECT}-lib${PROJECT}" )
    set( CPACK_DEBIAN_lmon_PACKAGE_DEPENDS "${PROJECT}-lib${PROJECT}" )
    set( CPACK_DEBIAN_lsys_PACKAGE_DEPENDS "${PROJECT}-lib${PROJECT}" )
    set( CPACK_DEBIAN_lblk_PACKAGE_DEPENDS "${PROJECT}-lib${PROJECT}" )
    set( CPACK_DEBIAN_lrep_PACKAGE_DEPENDS "${PROJECT}-lib${PROJECT}" )
    set( CPACK_DEBIAN_labbix_PACKAGE_DEPENDS "${PROJECT}-lib${PROJECT}" )
  endif()
endif()

message(STATUS "OS_RELEASE_ID          : ${OS_RELEASE_ID}")
message(STATUS "OS_RELEASE_PRETTY_NAME : ${OS_RELEASE_PRETTY_NAME}")
message(STATUS "OS_RELEASE_TAG         : ${OS_RELEASE_TAG}")
message(STATUS "CPACK_GENERATOR        : ${CPACK_GENERATOR}")
