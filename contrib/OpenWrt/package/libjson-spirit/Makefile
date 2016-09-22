#
# Copyright (C) 2007-2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libjson-spirit
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/sirikata/json-spirit.git
PKG_SOURCE_VERSION:=4f1a1023b5d14ec98887ead132d881b8a7acfd3c
PKG_SOURCE_PROTO:=git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)


include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

CMAKE_SOURCE_DIR:=build

define Package/libjson-spirit
  CATEGORY:=Libraries
  TITLE:=C++ JSON library
  DEPENDS:=+libstdcpp +boost-system +boost-thread
endef

define Package/libjson-spirit/description
  JSON Spirit is a C++ JSON library using Boost Spirit for parsing.
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/json_spirit
	$(CP) $(PKG_INSTALL_DIR)/usr/include/json_spirit/* $(1)/usr/include/json_spirit
	
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libjson_spirit.so $(1)/usr/lib
endef

define Package/libjson-spirit/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libjson_spirit.so $(1)/usr/lib
endef

$(eval $(call BuildPackage,libjson-spirit))

