#
# Copyright (C) 2007-2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=un-orchestrator
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/netgroup-polito/un-orchestrator.git
PKG_SOURCE_VERSION:=master
PKG_SOURCE_PROTO:=git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

#CMAKE_SOURCE_DIR:=un-orchestrator_build

define Package/un-orchestrator
  CATEGORY:=Utilities
  TITLE:=Name resolver 
  URL:==https://github.com/netgroup-polito/un-orchestrator
  MAINTAINER:=PoliTO
  DEPENDS:= +librofl +libjson-spirit +libxml2 +libmicrohttpd +libstdcpp +libsqlite3
# +libboost_system
endef

define Package/un-orchestrator/description
  da definire
endef

TARGET_CFLAGS:=$(TARGET_CFLAGS) -I$(STAGING_DIR)/usr/include/libxml2

define Build/Configure
#	$(CP) $(PKG_BUILD_DIR)/contrib/OpenWrt/name-resolver/CMakeLists.txt $(PKG_BUILD_DIR)/name-resolver
#	mv $(PKG_BUILD_DIR) $(PKG_BUILD_DIR)/un-orchestrator_build
	$(Build/Configure/Default)
endef

define Build/Install
endef

define Package/un-orchestrator/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/name-resolver/name-resolver $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/orchestrator/node-orchestrator $(1)/usr/sbin
	
	$(INSTALL_DIR) $(1)/cfg/name-resolver/config
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/name-resolver/config/example.xml $(1)/cfg/name-resolver/config
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/name-resolver/config/network-functions.xsd $(1)/cfg/name-resolver/config
	$(INSTALL_DIR) $(1)/cfg/orchestrator/config
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/orchestrator/config/default-config.ini $(1)/cfg/orchestrator/config
endef

$(eval $(call BuildPackage,un-orchestrator))

