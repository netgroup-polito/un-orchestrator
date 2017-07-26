#
# Copyright (C) 2007-2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=czmq
PKG_VERSION:=3.0.2
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/zeromq/czmq
PKG_SOURCE_VERSION:=v3.0.2
PKG_SOURCE_PROTO:=git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
EXTRA_CFLAGS:=-std=gnu99


include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk


define Package/czmq
  CATEGORY:=Utility
  TITLE:=High-level C binding for ØMQ
  DEPENDS:= +libzmq
  PROVIDES:=libczmq
endef

define Package/czmq/description
  High-level C binding for ØMQ
endef

define Build/Configure
	(cd $(PKG_BUILD_DIR);  ./autogen.sh)
	$(Build/Configure/Default)
endef

define Build/Install
	$(call Build/Install/Default, install)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include
	
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libczmq.so $(1)/usr/lib
endef

define Package/czmq/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libczmq.so $(1)/usr/lib
endef

$(eval $(call BuildPackage,czmq))

