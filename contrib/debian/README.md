
# Debian Packaging for BabaChain Core

This directory contains files used to package babachaind/babachain-qt for Debian-based Linux systems.

## Package Contents

- **babachaind** - BabaChain Core daemon
- **babachain-qt** - BabaChain Core GUI wallet  
- **babachain-tx** - Transaction creation utility
- **babachain-cli** - Command line interface

## Installation Methods

### Method 1: Automated Installation Script

After building BabaChain Core, use the provided installation script:

```bash
./contrib/install.sh
```

This script will:
- Install all binaries to `/usr/local/bin`
- Set up desktop integration
- Create configuration directory
- Install icons and protocol handlers

### Method 2: Manual Installation

#### Desktop Integration (Gnome/XFCE/Unity)

```bash
sudo desktop-file-install babachain-qt.desktop
sudo update-desktop-database
```

#### KDE Protocol Handler

```bash
sudo cp babachain-qt.protocol /usr/share/kde4/services/
```

#### Icons

```bash
sudo cp ../../share/pixmaps/babachain*.png /usr/share/pixmaps/
sudo cp ../../share/pixmaps/babachain*.xpm /usr/share/pixmaps/
```

### Method 3: Package Building

Build Debian packages using the provided control files:

```bash
# Install build dependencies
sudo apt-get install debhelper devscripts

# Build packages
debuild -us -uc
```

## URI Support

The desktop file and protocol handler enable `babachain:` URI support, allowing users to:
- Click babachain: links in web browsers
- Open payment requests directly in BabaChain Core

## Uninstallation

To remove BabaChain Core:

```bash
./contrib/uninstall.sh
```

This preserves wallet and configuration files while removing binaries and desktop integration.

## Files Description

- `control` - Package metadata and dependencies
- `babachain-qt.desktop` - Desktop entry for GUI application
- `babachain-qt.protocol` - KDE protocol handler
- `*.install` - File installation mappings
- `*.manpages` - Manual page installations
- `copyright` - License and copyright information
- `rules` - Build rules for package creation

