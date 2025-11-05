# BabaChain Installation Guide

For detailed build instructions, please see the platform-specific documentation:

- [Unix/Linux Build Guide](doc/build-unix.md)
- [macOS Build Guide](doc/build-osx.md)  
- [Windows Build Guide](doc/build-windows.md)
- [FreeBSD Build Guide](doc/build-freebsd.md)
- [NetBSD Build Guide](doc/build-netbsd.md)
- [OpenBSD Build Guide](doc/build-openbsd.md)

## Quick Start

### Prerequisites
- Git
- C++ compiler (GCC 8+ or Clang 7+)
- Build tools (make, autotools)
- Dependencies (see platform-specific guides)

### Basic Build Steps

```bash
git clone https://github.com/baba-chain/babachain.git
cd babachain
./autogen.sh
./configure
make
make install  # optional
```

### Running BabaChain

After building, you can run:
- `babachaind` - BabaChain daemon
- `babachain-cli` - Command line interface
- `babachain-qt` - GUI wallet (if built with Qt support)

For more detailed instructions and troubleshooting, see the platform-specific build guides in the [doc/](/doc) directory.
