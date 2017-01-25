pkgname=leanux
pkgver=@leanux_VERSION_STR@
pkgrel=1
pkgdesc="@LEANUX_SHORT_DESC@"
arch=("i686" "x86_64" "mips" "sparc" "armv7h")
url="@LEANUX_WEBSITE@"
license=("GPLv3")
makedepends=("ncurses" "cmake" "zlib" "sqlite3")
source=("@LEANUX_SRC_URI_BASE@/$pkgname-$pkgver.tar.gz")
sha512sums=("@LEANUX_SOURCE_SHA512SUM@")
install=leanux

build() {
    mkdir $pkgname-$pkgver/release
    cd $pkgname-$pkgver/release
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make
}

check() {
    cd $pkgname-$pkgver/release
    make test
}

package() {
    cd $pkgname-$pkgver/release
    make DESTDIR="$pkgdir" install
    mv $pkgdir/usr/lib64/ $pkgdir/usr/lib/
}
