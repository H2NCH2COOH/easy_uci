include $(TOPDIR)/rules.mk

PKG_NAME:=libeasy_uci
PKG_VERSION:=0.1.0
PKG_RELEASE:=20160225

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
 SECTION:=libs
 CATEGORY:=Libraries
 TITLE:=Easy UCI
 DEPENDS:=+libuci
endef

define Package/$(PKG_NAME)/description
    An wrap around OpenWRT UCI C API
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/easy_uci.h $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libeasy_uci.so $(1)/usr/lib/
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libeasy_uci.so $(1)/usr/lib/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
