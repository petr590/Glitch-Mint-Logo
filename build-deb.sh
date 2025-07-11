#!/bin/bash
set -e

make_link() {
	mkdir -p "$(dirname "$2")"
	ln -f "$1" "$2"
	echo "Created hardlink $2 --> $1"
}

link_executables() {
	make_link "release/glitch-mint-logo" "deb/bin/glitch-mint-logo"

	for lib in release/lib*.so; do
		make_link "$lib" "deb/lib/${lib#release/}"
	done
}

create_notifiers() {
	find deb/etc/systemd/system/ -type f -name 'gml-notifier-*.service' -exec rm {} \;
	
	template=resources/gml-notifier.template
	list=deb/etc/glitch-mint-logo/notifier-services.list
	
	> "$list" # Clear file
	
	while IFS= read -r service; do
		notifier="gml-notifier-$(cut -d. -f1 <<< "$service").service"
		echo "$notifier" >> "$list"
		sed "s/{service}/$service/g" "$template" > "deb/etc/systemd/system/$notifier"
	done < resources/services.list
}


calc_sizes_and_md5() {
	size=$(du -ks ./deb --exclude=./deb/DEBIAN | cut -f 1)
	sed -Ei "s/^Installed-Size:.*$/Installed-Size: $size/g" deb/DEBIAN/control
	find deb/* -maxdepth 0 -not -name DEBIAN -print0 | xargs -0 md5deep -r > deb/DEBIAN/md5sums
}


rename_deb_package() {
	package="$(sed -nE 's/^Package:\s*(.*)$/\1/p' ./deb/DEBIAN/control)"
	version="$(sed -nE 's/^Version:\s*(.*)$/\1/p' ./deb/DEBIAN/control)"
	architecture="$(sed -nE 's/^Architecture:\s*(.*)$/\1/p' ./deb/DEBIAN/control)"
	
	deb_file="${package}_${version}_${architecture}.deb"
	mv deb.deb "$deb_file"
	echo "'./deb.deb' moved to './$deb_file'"
}


link_executables
create_notifiers
calc_sizes_and_md5

fakeroot dpkg-deb --build deb/

[[ "$1" != '--no-rename' ]] && rename_deb_package

exit 0
