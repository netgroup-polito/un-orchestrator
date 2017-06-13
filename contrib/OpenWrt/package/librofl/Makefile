#
# Copyright (C) 2007-2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=librofl
PKG_VERSION:=0.11.1
PKG_RELEASE:=1

PKG_SOURCE_URL:=https://github.com/bisdn/rofl-common.git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_VERSION:=03dcdef188d745ba8bc294ef4a8c913f0f07bdbc
PKG_SOURCE_PROTO:=git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)

PKG_FIXUP:=libtool

include $(INCLUDE_DIR)/package.mk

define Package/librofl
  CATEGORY:=Libraries
  TITLE:=OpenFlow C++ library 
  DEPENDS:=+libstdcpp +librt +libopenssl +libpthread +libglog
endef

define Package/librofl/description
  Revised OpenFlow Library (ROFL) is a C++ library that helps on writing OpenFlow endpoints (OF agents), for controllers, proxy controllers and datapaths
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
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/librofl_common.so* $(1)/usr/lib
endef

define Package/librofl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/librofl_common.so* $(1)/usr/lib
endef

$(eval $(call BuildPackage,librofl))
