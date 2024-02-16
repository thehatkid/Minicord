ARCH := $(shell uname -m)

# Declare directories
PREFIX := $(PWD)/build-$(ARCH)
DLDIR  := $(PWD)/downloads
BINDIR := $(PREFIX)/bin
LIBDIR := $(PREFIX)/lib
INCDIR := $(PREFIX)/include
PKGDIR := $(PREFIX)/lib/pkgconfig
BDIR   := build-$(ARCH)

# Variables for compiling
CFLAGS  := -I$(INCDIR) -O3
LDFLAGS := -L$(LIBDIR)
NPROC   := $(shell nproc)

CONFIGURE_ENV  := PKG_CONFIG_LIBDIR="$(PKGDIR)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)"

CMAKE_ARGS := \
	-DCMAKE_PREFIX_PATH="$(PREFIX)" \
	-DCMAKE_INSTALL_PREFIX="$(PREFIX)" \
	-DCMAKE_C_FLAGS="$(CFLAGS)" \
	-DCMAKE_BUILD_TYPE=Release \
	-G "Unix Makefiles"

GIT   := git clone -q -c advice.detachedHead=false --single-branch --no-tags --depth 1
CMAKE := $(CONFIGURE_ENV) cmake -S . -B $(BDIR) $(CMAKE_ARGS)


# ============================ Dependencies options ============================


OPTS_RAPIDJSON := \
	-DCMAKE_INSTALL_DIR="$(LIBDIR)/cmake/RapidJSON" \
	-DRAPIDJSON_BUILD_DOC=OFF \
	-DRAPIDJSON_BUILD_EXAMPLES=OFF \
	-DRAPIDJSON_BUILD_TESTS=OFF \
	-DRAPIDJSON_BUILD_DOC=OFF \
	-DRAPIDJSON_HAS_STDSTRING=ON

OPTS_MBEDTLS := \
	-DENABLE_PROGRAMS=OFF \
	-DENABLE_TESTING=OFF

OPTS_IXWEBSOCKET := \
	-DBUILD_SHARED_LIBS=OFF \
	-DUSE_TLS=ON \
	-DUSE_ZLIB=ON \
	-DUSE_OPEN_SSL=OFF \
	-DUSE_MBED_TLS=ON


# ==============================================================================


all: download build


# Download target recipes
download: \
	$(DLDIR) \
	$(DLDIR)/$(ARCH)/zlib/configure \
	$(DLDIR)/rapidjson/CMakeLists.txt \
	$(DLDIR)/mbedtls/CMakeLists.txt \
	$(DLDIR)/ixwebsocket/CMakeLists.txt

# Build target recipes
build: \
	zlib \
	rapidjson \
	mbedtls \
	ixwebsocket


init: $(DLDIR) $(BINDIR) $(LIBDIR) $(INCDIR)

$(DLDIR):
	@mkdir -p $(DLDIR)

$(BINDIR):
	@mkdir -p $(BINDIR)

$(LIBDIR):
	@mkdir -p $(LIBDIR)

$(INCDIR):
	@mkdir -p $(INCDIR)


clean: clean-download clean-prefix

clean-download:
	@printf "\e[91m=>\e[0m \e[35mCleaning download folder...\e[0m\n"
	@rm -rf $(DLDIR)

clean-prefix:
	@printf "\e[91m=>\e[0m \e[35mCleaning prefix folder...\e[0m\n"
	@rm -rf $(PREFIX)


.PHONY: \
	all download build init clean clean-download clean-prefix \
	zlib rapidjson mbedtls ixwebsocket


# ================================ Dependencies ================================

# ------------------------------------ Zlib ------------------------------------
zlib: init $(LIBDIR)/libz.a

$(LIBDIR)/libz.a: $(DLDIR)/$(ARCH)/zlib/configure
	@printf "\e[94m=>\e[0m \e[36mBuilding Zlib...\e[0m\n"
	cd $(DLDIR)/$(ARCH)/zlib; \
	export $(CONFIGURE_ENV); \
	export CFLAGS="$$CFLAGS -fPIC"; \
	./configure --prefix="$(PREFIX)" --static; \
	make -j$(NPROC); \
	make install

$(DLDIR)/$(ARCH)/zlib/configure:
	@printf "\e[94m=>\e[0m \e[36mDownloading Zlib 1.3.1...\e[0m\n"
	@$(GIT) -b v1.3.1 https://github.com/madler/zlib $(DLDIR)/$(ARCH)/zlib

# ---------------------------------- RapidJSON ---------------------------------
rapidjson: init $(INCDIR)/rapidjson/rapidjson.h

$(INCDIR)/rapidjson/rapidjson.h: $(DLDIR)/rapidjson/$(BDIR)
	@printf "\e[94m=>\e[0m \e[36mInstalling RapidJSON...\e[0m\n"
	@cd $(DLDIR)/rapidjson; cmake --install $(BDIR)
	@touch $(INCDIR)/rapidjson/rapidjson.h

$(DLDIR)/rapidjson/$(BDIR): $(DLDIR)/rapidjson/CMakeLists.txt
	@printf "\e[94m=>\e[0m \e[36mConfiguring RapidJSON...\e[0m\n"
	@cd $(DLDIR)/rapidjson; $(CMAKE) $(OPTS_RAPIDJSON)

$(DLDIR)/rapidjson/CMakeLists.txt:
	@printf "\e[94m=>\e[0m \e[36mDownloading RapidJSON master...\e[0m\n"
	@$(GIT) -b master https://github.com/Tencent/rapidjson $(DLDIR)/rapidjson

# ----------------------------------- MbedTLS ----------------------------------
mbedtls: init $(LIBDIR)/libmbedtls.a

$(LIBDIR)/libmbedtls.a: $(DLDIR)/mbedtls/$(BDIR)/library/libmbedtls.a
	@printf "\e[94m=>\e[0m \e[36mInstalling MbedTLS...\e[0m\n"
	@cd $(DLDIR)/mbedtls; cmake --install $(BDIR)

$(DLDIR)/mbedtls/$(BDIR)/library/libmbedtls.a: $(DLDIR)/mbedtls/$(BDIR)/Makefile
	@printf "\e[94m=>\e[0m \e[36mBuilding MbedTLS...\e[0m\n"
	@cd $(DLDIR)/mbedtls; cmake --build $(BDIR) -j$(NPROC)

$(DLDIR)/mbedtls/$(BDIR)/Makefile: $(DLDIR)/mbedtls/CMakeLists.txt
	@printf "\e[94m=>\e[0m \e[36mConfiguring MbedTLS...\e[0m\n"
	@cd $(DLDIR)/mbedtls; $(CMAKE) $(OPTS_MBEDTLS)

$(DLDIR)/mbedtls/CMakeLists.txt:
	@printf "\e[94m=>\e[0m \e[36mDownloading MbedTLS 3.4.1...\e[0m\n"
	@$(GIT) -b v3.4.1 https://github.com/Mbed-TLS/mbedtls $(DLDIR)/mbedtls

# --------------------------------- IXWebSocket --------------------------------
ixwebsocket: init zlib mbedtls $(LIBDIR)/libixwebsocket.a

$(LIBDIR)/libixwebsocket.a: $(DLDIR)/ixwebsocket/$(BDIR)/libixwebsocket.a
	@printf "\e[94m=>\e[0m \e[36mInstalling IXWebSocket...\e[0m\n"
	@cd $(DLDIR)/ixwebsocket; cmake --install $(BDIR)

$(DLDIR)/ixwebsocket/$(BDIR)/libixwebsocket.a: $(DLDIR)/ixwebsocket/$(BDIR)/Makefile
	@printf "\e[94m=>\e[0m \e[36mBuilding IXWebSocket...\e[0m\n"
	@cd $(DLDIR)/ixwebsocket; cmake --build $(BDIR) -j$(NPROC)

$(DLDIR)/ixwebsocket/$(BDIR)/Makefile: $(DLDIR)/ixwebsocket/CMakeLists.txt
	@printf "\e[94m=>\e[0m \e[36mConfiguring IXWebSocket...\e[0m\n"
	@cd $(DLDIR)/ixwebsocket; $(CMAKE) $(OPTS_IXWEBSOCKET)

$(DLDIR)/ixwebsocket/CMakeLists.txt:
	@printf "\e[94m=>\e[0m \e[36mDownloading IXWebSocket master...\e[0m\n"
	@$(GIT) -b master https://github.com/machinezone/IXWebSocket $(DLDIR)/ixwebsocket
