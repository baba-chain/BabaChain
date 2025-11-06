build_linux_SHA256SUM = sha256sum
build_linux_DOWNLOAD = curl --location --fail --connect-timeout $(DOWNLOAD_CONNECT_TIMEOUT) --retry $(DOWNLOAD_RETRIES) -o

# Cross-compilation toolchain detection
ifneq ($(shell which aarch64-linux-gnu-gcc 2>/dev/null),)
aarch64_linux_CC = aarch64-linux-gnu-gcc
aarch64_linux_CXX = aarch64-linux-gnu-g++
aarch64_linux_AR = aarch64-linux-gnu-ar
aarch64_linux_RANLIB = aarch64-linux-gnu-ranlib
aarch64_linux_NM = aarch64-linux-gnu-nm
aarch64_linux_STRIP = aarch64-linux-gnu-strip
aarch64_linux_OBJDUMP = aarch64-linux-gnu-objdump
endif

ifneq ($(shell which arm-linux-gnueabihf-gcc 2>/dev/null),)
arm_linux_CC = arm-linux-gnueabihf-gcc
arm_linux_CXX = arm-linux-gnueabihf-g++
arm_linux_AR = arm-linux-gnueabihf-ar
arm_linux_RANLIB = arm-linux-gnueabihf-ranlib
arm_linux_NM = arm-linux-gnueabihf-nm
arm_linux_STRIP = arm-linux-gnueabihf-strip
arm_linux_OBJDUMP = arm-linux-gnueabihf-objdump
endif
