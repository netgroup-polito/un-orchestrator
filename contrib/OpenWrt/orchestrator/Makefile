# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
#
# Copyright (C) 2016 Patrick Tomassone
#
# This is free software, licensed under the GNU General Public License v2.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=node-orchestrator
PKG_VERSION:=0.0.1
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/node-orchestrator
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=CMake OpenWrt cross compile un-orchestrator-frog4
  DEPENDS:=+libmicrohttpd +boost-system +libstdcpp +libxml2 +boost-thread
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./* $(PKG_BUILD_DIR)/
	rm -f $(PKG_BUILD_DIR)/CMakeCache.txt
	rm -fR $(PKG_BUILD_DIR)/CMakeFiles
	rm -f $(PKG_BUILD_DIR)/Makefile ]
	rm -f $(PKG_BUILD_DIR)/cmake_install.cmake
	rm -f $(PKG_BUILD_DIR)/progress.make
endef

define Build/Configure
    IN_OPENWRT=1 \
    AR="$(TARGET_CROSS)ar" \
    AS="$(TARGET_CC) -c $(TARGET_CFLAGS)" \
    LD="$(TARGET_CROSS)ld" \
    NM="$(TARGET_CROSS)nm" \
    CC="$(TARGET_CC)" \
    GCC="$(TARGET_CC)" \
    CXX="$(TARGET_CROSS)g++" \
    RANLIB="$(TARGET_CROSS)ranlib" \
    STRIP="$(TARGET_CROSS)strip" \
    OBJCOPY="$(TARGET_CROSS)objcopy" \
    OBJDUMP="$(TARGET_CROSS)objdump" \
    TARGET_CPPFLAGS="$(TARGET_CPPFLAGS)" \
    TARGET_CFLAGS="$(TARGET_CFLAGS)" \
    TARGET_LDFLAGS="$(TARGET_LDFLAGS)" \
    cmake $(PKG_BUILD_DIR)/CMakeLists.txt
endef

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/node-orchestrator $(STAGING_DIR)/usr/bin/
endef

define Build/UninstallDev
	rm -rf \
	$(STAGING_DIR)/usr/include/node-orchestrator \
	$(STAGING_DIR)/usr/bin/node-orchestrator
endef

define Build/Compile
	LDFLAGS="$(EXTRA_LDFLAGS) " \
	$(MAKE) -C $(PKG_BUILD_DIR)
endef

define Package/node-orchestrator/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/node-orchestrator $(1)/bin/
endef

$(eval $(call BuildPackage,node-orchestrator))
