#!/bin/sh
set -e

BIN_SRC_PATH="build/glitch-mint-logo"
BIN_DST_PATH="deb/bin/glitch-mint-logo"

[ -e "$BIN_DST_PATH" ] && rm "$BIN_DST_PATH"
ln "$BIN_SRC_PATH" "$BIN_DST_PATH"

size=$(du -ks ./deb --exclude=./deb/DEBIAN | cut -f 1)
sed -Ei "s/^Installed-Size:.*$/Installed-Size: $size/g" ./deb/DEBIAN/control

package="$(sed -nE 's/^Package:\s*(.*)$/\1/p' ./deb/DEBIAN/control)"
version="$(sed -nE 's/^Version:\s*(.*)$/\1/p' ./deb/DEBIAN/control)"
architecture="$(sed -nE 's/^Architecture:\s*(.*)$/\1/p' ./deb/DEBIAN/control)"

fakeroot dpkg-deb --build ./deb

mv deb.deb "${package}_${version}_${architecture}.deb"
