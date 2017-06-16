#
# Copyright (C) 2007-2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=DoubleDecker
PKG_VERSION:=0.4
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/Acreo/DoubleDecker
PKG_SOURCE_VERSION:=c113129870a36aed74c8d1a8fe77c497bf98aea2#master
PKG_SOURCE_PROTO:=git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)


include $(INCLUDE_DIR)/package.mk


define Package/DoubleDecker
  CATEGORY:=Utility
  TITLE:=DoubleDecker messaging system
  DEPENDS:= +libczmq +libsodium +libjson-c +liburcu
  PROVIDES:=libdd
endef

define Package/DoubleDecker/description
  DoubleDecker is a hierarchical distributed message system based on ZeroMQ which can be used to provide messaging between processes running on a single machine and between processes running on multiple machines. It is hierarchical in the sense that message brokers are connected to each-other in a tree topology and route messages upwards in case they don't have the destination client beneath themselves.
endef

define Build/Configure
	(cd $(PKG_BUILD_DIR);  ./boot.sh)
	$(Build/Configure/Default)
endef

define Build/Install
	$(call Build/Install/Default, install)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include
	
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libdd*.{a,so*}	$(1)/usr/lib/

	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin/
endef

define Package/DoubleDecker/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libdd.so.0.0.3 $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libdd.so.0 $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libdd.so $(1)/usr/lib


	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/ddbroker $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/ddclient $(1)/usr/bin/

	$(INSTALL_DIR) $(1)/cfg/dd/keys
	$(CP) $(PKG_BUILD_DIR)/keys/* $(1)/cfg/dd/keys
endef

$(eval $(call BuildPackage,DoubleDecker))

