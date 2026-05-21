# Maintainer: ginluv21 <gabikg21@gmail.com>

pkgname=typecode-git
pkgver=r0.placeholder
pkgrel=1
pkgdesc="Terminal typing trainer for programmers - learn to type real code"
arch=('x86_64' 'aarch64' 'armv7h')
url="https://github.com/ginluv21/typecode"
license=('MIT')
depends=('ncurses')
makedepends=('git' 'gcc' 'make')
provides=('typecode')
conflicts=('typecode')
source=("$pkgname::git+https://github.com/ginluv21/typecode.git")
sha256sums=('SKIP')

pkgver() {
    cd "$pkgname"
    printf "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

build() {
    cd "$pkgname"
    make
}

package() {
    cd "$pkgname"

    # бинарник в /usr/share/typecode/ (нужны уроки рядом)
    install -dm755 "$pkgdir/usr/share/typecode"
    install -Dm755 typecode "$pkgdir/usr/share/typecode/typecode"

    # уроки
    cp -r lessons "$pkgdir/usr/share/typecode/lessons"

    # обёртка в PATH - делает cd перед запуском
    install -dm755 "$pkgdir/usr/bin"
    cat > "$pkgdir/usr/bin/typecode" <<'EOF'
#!/bin/sh
cd /usr/share/typecode
exec /usr/share/typecode/typecode "$@"
EOF
    chmod 755 "$pkgdir/usr/bin/typecode"
}
