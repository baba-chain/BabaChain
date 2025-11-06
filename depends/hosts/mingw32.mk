ifneq ($(shell $(SHELL) $(.SHELLFLAGS) "command -v $(host)-g++-posix"),)
mingw32_CXX := $(host)-g++-posix
endif

mingw32_CFLAGS=-pipe -std=$(C_STANDARD)
mingw32_CXXFLAGS=-pipe -std=$(CXX_STANDARD)

# Windows-specific flags
mingw32_CPPFLAGS=-DWIN32 -D_WIN32_WINNT=0x0601 -D_WIN32_IE=0x0501 -DWIN32_LEAN_AND_MEAN -DNOMINMAX
mingw32_LDFLAGS=-Wl,--major-subsystem-version -Wl,6 -Wl,--minor-subsystem-version -Wl,1

# Static linking for Windows
mingw32_LDFLAGS += -static-libgcc -static-libstdc++

ifneq ($(LTO),)
mingw32_AR = $(host_toolchain)gcc-ar
mingw32_NM = $(host_toolchain)gcc-nm
mingw32_RANLIB = $(host_toolchain)gcc-ranlib
endif

mingw32_release_CFLAGS=-O2
mingw32_release_CXXFLAGS=$(mingw32_release_CFLAGS)

mingw32_debug_CFLAGS=-O1 -g
mingw32_debug_CXXFLAGS=$(mingw32_debug_CFLAGS)

mingw32_debug_CPPFLAGS=-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC

mingw32_cmake_system=Windows
