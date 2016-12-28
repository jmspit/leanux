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
	insinto /etc/lard
	newins "${WORKDIR}"/"${P}"/tools/lard/etc/lard.conf lard.conf
	newinitd "${WORKDIR}"/"${P}"/tools/lard/gentoo/init.d/lard lard
	newconfd "${WORKDIR}"/"${P}"/tools/lard/gentoo/conf.d/lard lard
}

pkg_postinst() {
	enewgroup leanux
	enewuser leanux -1 -1 -1 leanux
	chown -R leanux:leanux /etc/lard
	chmod 775 /etc/lard
	chmod 644 /etc/lard/lard.conf
}

pkg_postrm() {
	rm -rf /var/run/lard  
	rm -rf /var/lib/lard
	rm -rf /etc/lard
	rm /etc/init.d/lard
	userdel leanux
  groupdel leanux
}
