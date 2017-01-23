# Copyright 1999-2015 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Id$

EAPI=5
inherit eutils cmake-utils flag-o-matic user

DESCRIPTION="@LEANUX_SHORT_DESC@"
HOMEPAGE="@LEANUX_WEBSITE@"
SRC_URI="@LEANUX_SRC_URI@"

LICENSE="GPL-3"
SLOT="0"
KEYWORDS="~arm ~mips ~sparc ~x86"

DEPEND="sys-libs/ncurses
  >=dev-util/cmake-2.8.12
  sys-libs/zlib
  sys-apps/hwids
  >=dev-db/sqlite-3.7.0"

src_prepare() {
  append-cflags -std=c99
  append-cxxflags -std=c++11
}

src_configure() {
  cmake-utils_src_configure
}

src_install() {
  cmake-utils_src_install
  insinto @LARD_SYSCONF_DIR@
  newins "${BUILD_DIR}"/lard.conf lard.conf
  newinitd "${BUILD_DIR}"/init.d/lard lard
  newconfd "${BUILD_DIR}"/conf.d/lard lard
}

pkg_postinst() {
  enewgroup @LARD_USER@
  enewuser @LARD_USER@ -1 -1 -1 @LARD_USER@
  chown -R @LARD_USER@:@LARD_USER@ @LARD_SYSCONF_DIR@
  chmod 775 @LARD_SYSCONF_DIR@
  chmod 644 @LARD_SYSCONF_FILE@
}

pkg_postrm() {
  rm -rf /var/run/lard
  rm -rf @LARD_SYSDB_PATH@
  rm -rf @LARD_SYSCONF_DIR@
  rm @LARD_SYSVINIT_FILE@
  userdel @LARD_USER@
  groupdel @LARD_USER@
}
