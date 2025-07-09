#!/bin/bash
set -e

make_link() {
	mkdir -p "$(dirname "$2")"
	ln -f "$1" "$2"
	echo "Created hardlink $2 --> $1"
}

make_link "build/glitch-mint-logo" "deb/bin/glitch-mint-logo"

for lib in build/lib*.so; do
	make_link "$lib" "deb/lib/${lib#build/}"
done

size=$(du -ks ./deb --exclude=./deb/DEBIAN | cut -f 1)
sed -Ei "s/^Installed-Size:.*$/Installed-Size: $size/g" ./deb/DEBIAN/control
find ./deb/* -maxdepth 0 -not -name DEBIAN -print0 | xargs -0 md5deep -r > ./deb/DEBIAN/md5sums

package="$(sed -nE 's/^Package:\s*(.*)$/\1/p' ./deb/DEBIAN/control)"
version="$(sed -nE 's/^Version:\s*(.*)$/\1/p' ./deb/DEBIAN/control)"
architecture="$(sed -nE 's/^Architecture:\s*(.*)$/\1/p' ./deb/DEBIAN/control)"

fakeroot dpkg-deb --build ./deb

if [[ "$1" != '--no-rename' ]]; then
	deb_file="${package}_${version}_${architecture}.deb"
	mv deb.deb "$deb_file"
	echo "'./deb.deb' moved to './$deb_file'"
fi
