BUILD_DIR := build

LIBPNG_LIB := libpng16
LUA_LIB := lua

CPPFLAGS := -I$(BUILD_DIR)/include -Iinclude -I$(PREFIX)/include
CXXFLAGS := -std=c++20
LDFLAGS := -L$(BUILD_DIR)/lib -L$(PREFIX)/lib
LIBS := -l$(LUA_LIB)

MSYS_VERSION := $(if $(findstring Msys, $(shell uname -o)),$(word 1, $(subst ., ,$(shell uname -r))),0)
ifneq ($(MSYS_VERSION),0)
	SHARED_LIB_EXT := dll
	LIB_EXT := a
	LDFLAGS += -shared -static
	LUAJIT_LIB := lua51.$(SHARED_LIB_EXT)
	LIBS += -lpng16 -lzlibstatic -lgif
override DEVI_LDFLAGS := -shared -static
else
	LIBS += -lpng16 -lz -lgif
	ifeq ($(shell uname),Darwin)
		SHARED_LIB_EXT := dylib
		LIB_EXT := a
		LUAJIT_LIB := libluajit.$(LIB_EXT)

		ifeq ($(PLATFORM),IOS)
			SYSROOT=$(shell xcrun --sdk iphoneos --show-sdk-path)
			CFLAGS += -isysroot $(SYSROOT) -miphoneos-version-min=13.0
			CXXFLAGS += -arch arm64 -fPIC -isysroot $(SYSROOT) -miphoneos-version-min=13.0
			LDFLAGS += -arch arm64 -dynamiclib -all_load -fPIC -isysroot $(SYSROOT) -miphoneos-version-min=13.0
			CMAKE_OPTS := -DCMAKE_OSX_ARCHITECTURES="arm64" -DCMAKE_OSX_SYSROOT=$(SYSROOT) -DCMAKE_OSX_DEPLOYMENT_TARGET="13.0"
			CC := $(shell xcrun --sdk iphoneos --find clang)
			CXX := $(shell xcrun --sdk iphoneos --find clang++)
			LD := $(shell xcrun --sdk iphoneos --find ld)

# For LuaJIT
			ISDKP := $(shell xcrun --sdk iphoneos --show-sdk-path)
			ICC := $(shell xcrun --sdk iphoneos --find clang)
			ISDKF := -arch arm64 -isysroot $(ISDKP)
		else
			CFLAGS += -arch x86_64 -arch arm64
			CXXFLAGS += -arch x86_64 -arch arm64
			LDFLAGS += -arch x86_64 -arch arm64 -dynamiclib
			CMAKE_OPTS := -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
		endif
	else
		SHARED_LIB_EXT := so
		LIB_EXT := a
override DEVI_CXXFLAGS += -fPIC
override DEVI_LDFLAGS += -shared -fPIC
		LUAJIT_LIB := libluajit.$(SHARED_LIB_EXT)
	endif
endif

ifeq ($(BUILD),DEBUG)
override DEVI_CXXFLAGS += -g3 -O0
override DEVI_LDFLAGS += -g3
else
override DEVI_CXXFLAGS += -O3
endif

LIBPNG_VERSION := 1.6.40
ZLIB_VERSION := 1.3.1
GIFLIB_VERSION := 5.2.1

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build LuaJIT
$(BUILD_DIR)/LuaJIT/src/Makefile:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && git clone https://github.com/LuaJIT/LuaJIT

$(BUILD_DIR)/LuaJIT/src/$(LUAJIT_LIB): $(BUILD_DIR)/LuaJIT/src/Makefile
ifeq ($(shell uname),Darwin)
ifeq ($(PLATFORM),IOS)
	cd $(BUILD_DIR)/LuaJIT/src && make DEFAULT_CC=clang CROSS="$(shell dirname $(ICC))/" TARGET_FLAGS="$(ISDKF)" TARGET_SYS=iOS
else
	cd $(BUILD_DIR)/LuaJIT/src && make MACOSX_DEPLOYMENT_TARGET="11.00" clean && make TARGET_FLAGS="-arch arm64" MACOSX_DEPLOYMENT_TARGET="11.00" && mv $(LUAJIT_LIB) ../libluajit_arm64.$(LIB_EXT)
	cd $(BUILD_DIR)/LuaJIT/src && make MACOSX_DEPLOYMENT_TARGET="11.00" clean && make TARGET_FLAGS="-arch x86_64" MACOSX_DEPLOYMENT_TARGET="11.00" && mv $(LUAJIT_LIB) ../libluajit_intel.$(LIB_EXT)
	lipo -create $(BUILD_DIR)/LuaJIT/libluajit_arm64.$(LIB_EXT) $(BUILD_DIR)/LuaJIT/libluajit_intel.$(LIB_EXT) -output $(BUILD_DIR)/LuaJIT/src/$(LUAJIT_LIB)
endif
else
	cd $(BUILD_DIR)/LuaJIT/src && make
endif

$(BUILD_DIR)/lib/$(LUAJIT_LIB): $(BUILD_DIR)/LuaJIT/src/$(LUAJIT_LIB)
	mkdir -p $(BUILD_DIR)/lib
	cp $(BUILD_DIR)/LuaJIT/src/$(LUAJIT_LIB) $(BUILD_DIR)/lib/$(LUAJIT_LIB)

$(BUILD_DIR)/include/lua.h: $(BUILD_DIR)/LuaJIT/src/$(LUAJIT_LIB)
	mkdir -p $(BUILD_DIR)/include
	cp $(BUILD_DIR)/LuaJIT/src/lua.h $(BUILD_DIR)/include/lua.h

$(BUILD_DIR)/include/lualib.h: $(BUILD_DIR)/LuaJIT/src/$(LUAJIT_LIB)
	mkdir -p $(BUILD_DIR)/include
	cp $(BUILD_DIR)/LuaJIT/src/lualib.h $(BUILD_DIR)/include/lualib.h

$(BUILD_DIR)/include/lauxlib.h: $(BUILD_DIR)/LuaJIT/src/$(LUAJIT_LIB)
	mkdir -p $(BUILD_DIR)/include
	cp $(BUILD_DIR)/LuaJIT/src/lauxlib.h $(BUILD_DIR)/include/lauxlib.h

$(BUILD_DIR)/include/luaconf.h: $(BUILD_DIR)/LuaJIT/src/$(LUAJIT_LIB)
	mkdir -p $(BUILD_DIR)/include
	cp $(BUILD_DIR)/LuaJIT/src/luaconf.h $(BUILD_DIR)/include/luaconf.h

# Build zlib
override ZLIB_FILE := zlib-$(ZLIB_VERSION)

$(BUILD_DIR)/$(ZLIB_FILE).tar.gz:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && curl -Lo $(ZLIB_FILE).tar.gz https://www.zlib.net/fossils/zlib-$(ZLIB_VERSION).tar.gz

$(BUILD_DIR)/$(ZLIB_FILE)/CMakeLists.txt: $(BUILD_DIR)/$(ZLIB_FILE).tar.gz
	cd $(BUILD_DIR) && tar xzfm $(ZLIB_FILE).tar.gz

$(BUILD_DIR)/$(ZLIB_FILE)-build: $(BUILD_DIR)/$(ZLIB_FILE)/CMakeLists.txt
	cmake -B$(BUILD_DIR)/$(ZLIB_FILE)-build -H$(BUILD_DIR)/$(ZLIB_FILE) $(CMAKE_OPTS) -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(BUILD_DIR) -DBUILD_SHARED_LIBS=OFF

$(BUILD_DIR)/lib/libz.$(LIB_EXT): $(BUILD_DIR)/$(ZLIB_FILE)-build
	cmake --build $(BUILD_DIR)/$(ZLIB_FILE)-build --parallel
	cmake --install $(BUILD_DIR)/$(ZLIB_FILE)-build --prefix $(BUILD_DIR)
	find $(BUILD_DIR)/lib -iname 'libz*.$(SHARED_LIB_EXT)*' -delete
	touch $(BUILD_DIR)/lib/libz.$(LIB_EXT)

# Patch & build libpng
override LIBPNG_FILE := libpng-$(LIBPNG_VERSION)
override LIBPNG_PATCH := libpng-$(LIBPNG_VERSION)-apng.patch

$(BUILD_DIR)/$(LIBPNG_PATCH).gz:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && curl -Lo $(LIBPNG_PATCH).gz https://sourceforge.net/projects/libpng-apng/files/libpng16/$(LIBPNG_VERSION)/libpng-$(LIBPNG_VERSION)-apng.patch.gz/download

$(BUILD_DIR)/$(LIBPNG_PATCH): $(BUILD_DIR)/$(LIBPNG_PATCH).gz
	cd $(BUILD_DIR) && gzip -dk $(LIBPNG_PATCH)

$(BUILD_DIR)/$(LIBPNG_FILE).tar.gz:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && curl -Lo $(LIBPNG_FILE).tar.gz https://sourceforge.net/projects/libpng/files/libpng16/$(LIBPNG_VERSION)/libpng-$(LIBPNG_VERSION).tar.gz/download

$(BUILD_DIR)/$(LIBPNG_FILE)/CMakeLists.txt: $(BUILD_DIR)/$(LIBPNG_FILE).tar.gz $(BUILD_DIR)/$(LIBPNG_PATCH)
	cd $(BUILD_DIR) && tar xzfm $(LIBPNG_FILE).tar.gz
	cd $(BUILD_DIR)/$(LIBPNG_FILE) && git apply --reject --whitespace=fix ../$(LIBPNG_PATCH)

# libpng 1.6.40 can't be built as a universal binary with CMake on macOS (linker errors involving neon optimizations)
# So build each architecture separately then combine them using lipo
$(BUILD_DIR)/$(LIBPNG_FILE)-build-x86_64: $(BUILD_DIR)/$(LIBPNG_FILE)/CMakeLists.txt $(BUILD_DIR)/lib/libz.$(LIB_EXT)
	cmake -B$(BUILD_DIR)/$(LIBPNG_FILE)-build-x86_64 -H$(BUILD_DIR)/$(LIBPNG_FILE) -DCMAKE_OSX_ARCHITECTURES="x86_64" -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(BUILD_DIR) -DBUILD_SHARED_LIBS=OFF

$(BUILD_DIR)/$(LIBPNG_FILE)-build-arm64: $(BUILD_DIR)/$(LIBPNG_FILE)/CMakeLists.txt $(BUILD_DIR)/lib/libz.$(LIB_EXT)
	cmake -B$(BUILD_DIR)/$(LIBPNG_FILE)-build-arm64 -H$(BUILD_DIR)/$(LIBPNG_FILE) -DCMAKE_OSX_ARCHITECTURES="arm64" -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(BUILD_DIR) -DBUILD_SHARED_LIBS=OFF

$(BUILD_DIR)/$(LIBPNG_FILE)-build-x86_64/$(LIBPNG_LIB).$(LIB_EXT): $(BUILD_DIR)/$(LIBPNG_FILE)-build-x86_64
	cmake --build $(BUILD_DIR)/$(LIBPNG_FILE)-build-x86_64 --parallel
	touch $(BUILD_DIR)/$(LIBPNG_FILE)-build-x86_64/$(LIBPNG_LIB).$(LIB_EXT)

$(BUILD_DIR)/$(LIBPNG_FILE)-build-arm64/$(LIBPNG_LIB).$(LIB_EXT): $(BUILD_DIR)/$(LIBPNG_FILE)-build-arm64
	cmake --build $(BUILD_DIR)/$(LIBPNG_FILE)-build-arm64 --parallel
	touch $(BUILD_DIR)/$(LIBPNG_FILE)-build-arm64/$(LIBPNG_LIB).$(LIB_EXT)

$(BUILD_DIR)/libpng_universal.$(LIB_EXT): $(BUILD_DIR)/$(LIBPNG_FILE)-build-x86_64 $(BUILD_DIR)/$(LIBPNG_FILE)-build-arm64

$(BUILD_DIR)/$(LIBPNG_FILE)-build/$(LIBPNG_LIB).$(LIB_EXT): $(BUILD_DIR)/$(LIBPNG_FILE)/CMakeLists.txt $(BUILD_DIR)/lib/libz.$(LIB_EXT)
	cmake -B$(BUILD_DIR)/$(LIBPNG_FILE)-build -H$(BUILD_DIR)/$(LIBPNG_FILE) $(CMAKE_OPTS) -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX=$(BUILD_DIR) -DBUILD_SHARED_LIBS=OFF

ifeq ($(shell uname),Darwin)
ifeq ($(PLATFORM),IOS)
$(BUILD_DIR)/lib/$(LIBPNG_LIB).$(LIB_EXT): $(BUILD_DIR)/$(LIBPNG_FILE)-build/$(LIBPNG_LIB).$(LIB_EXT)
else
$(BUILD_DIR)/lib/$(LIBPNG_LIB).$(LIB_EXT): $(BUILD_DIR)/$(LIBPNG_FILE)-build-x86_64/$(LIBPNG_LIB).$(LIB_EXT) $(BUILD_DIR)/$(LIBPNG_FILE)-build-arm64/$(LIBPNG_LIB).$(LIB_EXT)
endif
else
$(BUILD_DIR)/lib/$(LIBPNG_LIB).$(LIB_EXT): $(BUILD_DIR)/$(LIBPNG_FILE)-build/$(LIBPNG_LIB).$(LIB_EXT)
endif

$(BUILD_DIR)/lib/$(LIBPNG_LIB).$(LIB_EXT):
ifeq ($(shell uname),Darwin)
ifeq ($(PLATFORM),IOS)
	cmake --build $(BUILD_DIR)/$(LIBPNG_FILE)-build --parallel
	cmake --install $(BUILD_DIR)/$(LIBPNG_FILE)-build --prefix $(BUILD_DIR)
	rm $(BUILD_DIR)/lib/$(LIBPNG_LIB)*$(SHARED_LIB_EXT)
else
	cmake --install $(BUILD_DIR)/$(LIBPNG_FILE)-build-arm64 --prefix $(BUILD_DIR)
	rm -rf $(BUILD_DIR)/lib/libpng*
	lipo -create $(BUILD_DIR)/$(LIBPNG_FILE)-build-x86_64/$(LIBPNG_LIB).$(LIB_EXT) $(BUILD_DIR)/$(LIBPNG_FILE)-build-arm64/$(LIBPNG_LIB).$(LIB_EXT) -output $(BUILD_DIR)/lib/$(LIBPNG_LIB).$(LIB_EXT)
endif
else
	cmake --build $(BUILD_DIR)/$(LIBPNG_FILE)-build --parallel
	cmake --install $(BUILD_DIR)/$(LIBPNG_FILE)-build --prefix $(BUILD_DIR)
	find $(BUILD_DIR)/lib -iname 'libpng*.$(SHARED_LIB_EXT)*' -delete
endif

# Build giflib
override GIFLIB_FILE := giflib-$(GIFLIB_VERSION)

$(BUILD_DIR)/$(GIFLIB_FILE).tar.gz:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && curl -Lo $(GIFLIB_FILE).tar.gz https://sourceforge.net/projects/giflib/files/giflib-$(GIFLIB_VERSION).tar.gz/download

$(BUILD_DIR)/$(GIFLIB_FILE)/Makefile: $(BUILD_DIR)/$(GIFLIB_FILE).tar.gz
	cd $(BUILD_DIR) && tar xzfm $(GIFLIB_FILE).tar.gz

$(BUILD_DIR)/$(GIFLIB_FILE)/libgif.$(LIB_EXT): $(BUILD_DIR)/$(GIFLIB_FILE)/Makefile
ifeq ($(shell uname),Darwin)
ifeq ($(PLATFORM),IOS)
	cd $(BUILD_DIR)/$(GIFLIB_FILE) && make CC="$(CC)" LD="$(LD)" CFLAGS="$(CFLAGS) -fPIC -miphoneos-version-min=13.0" LDFLAGS="$(LDFLAGS) -fPIC" clean libgif.$(LIB_EXT)
else
	cd $(BUILD_DIR)/$(GIFLIB_FILE) && make CFLAGS="$(CFLAGS)" clean libgif.$(LIB_EXT)
endif
else
	cd $(BUILD_DIR)/$(GIFLIB_FILE) && make clean libgif.$(LIB_EXT)
endif

$(BUILD_DIR)/lib/libgif.$(LIB_EXT): $(BUILD_DIR)/$(GIFLIB_FILE)/libgif.$(LIB_EXT) $(BUILD_DIR)/include/gif_lib.h
	mkdir -p $(BUILD_DIR)/lib
	mkdir -p $(BUILD_DIR)/include
	cp $(BUILD_DIR)/$(GIFLIB_FILE)/libgif.$(LIB_EXT) $(BUILD_DIR)/lib/libgif.$(LIB_EXT)

$(BUILD_DIR)/include/gif_lib.h: $(BUILD_DIR)/$(GIFLIB_FILE)/Makefile
	mkdir -p $(BUILD_DIR)/include
	cp $(BUILD_DIR)/$(GIFLIB_FILE)/gif_lib.h $(BUILD_DIR)/include/gif_lib.h

# Build devi
CPP = $(wildcard source/*.cpp)
OBJ = $(CPP:%.cpp=$(BUILD_DIR)/%.o)
DEP = $(OBJ:%.o=%.d)
BIN = devi.$(SHARED_LIB_EXT)

-include $(DEP)

$(BUILD_DIR)/%.o: %.cpp $(BUILD_DIR) $(BUILD_DIR)/lib/libgif.$(LIB_EXT) $(BUILD_DIR)/lib/$(LIBPNG_LIB).$(LIB_EXT)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEVI_CXXFLAGS) $(CPPFLAGS) -MMD -c $< -o $@
	touch $@

$(BUILD_DIR)/$(BIN): $(OBJ)
	$(CXX) $(LDFLAGS) $(DEVI_LDFLAGS) $^ -o $@ $(LIBS)

all: $(BUILD_DIR) $(BUILD_DIR)/$(BIN)

luajit: $(BUILD_DIR) $(BUILD_DIR)/lib/$(LUAJIT_LIB) $(BUILD_DIR)/include/lua.h $(BUILD_DIR)/include/lualib.h $(BUILD_DIR)/include/lauxlib.h $(BUILD_DIR)/include/luaconf.h

clean:
	rm -rf build
