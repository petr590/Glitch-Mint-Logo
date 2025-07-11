#!/bin/sh
# Только для дебага
# Увеличивает версию билда на 1, собирает deb-пакет и устанавливает его

make -C build/ &&
sed -Ei 's/^Version: ([0-9]+\.[0-9]+\.[0-9]+)-([0-9]+)$/echo "Version: \1-$((\2+1))"/ge' deb/DEBIAN/control &&
./build-deb.sh --no-rename &&
sudo apt install ./deb.deb
