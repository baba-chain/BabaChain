
Debian
====================
This directory contains files used to package babachaind/babachain-qt
for Debian-based Linux systems. If you compile babachaind/babachain-qt yourself, there are some useful files here.

## babachain: URI support ##

babachain-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install babachain-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your babachain-qt binary to `/usr/bin`
and the `../../share/pixmaps/babachain128.png` to `/usr/share/pixmaps`

babachain-qt.protocol (KDE)

