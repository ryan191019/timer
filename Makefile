#
# Copyright (C) 2016 Montage Inc.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=zmtc

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/zmtc
  SECTION:=base
  CATEGORY:=Administration
  SUBMENU:=Montage configuration items
  TITLE:= Montage Controller
  VERSION:=1
  DEPENDS:=+libutils +wdk +libmpdclient
endef

define Package/zmtc/description
  Montage Controller
endef

define Package/zmtc/config
source "$(SOURCE)/Config.in"
endef

TARGET_CFLAGS += \
	 -include $(STAGING_DIR)/usr/include/mon_config.h \
	 -include $(STAGING_DIR)/usr/include/mon_kconfig.h \
	 -I$(PKG_BUILD_DIR) \
	 -I$(STAGING_DIR)/usr/include \
	 -I$(STAGING_DIR)/usr/include/libutils \
	 -I$(STAGING_DIR)/usr/include/wdk \
	 -Werror

TARGET_LDFLAGS += \
	-Wl,-rpath-link=$(STAGING_DIR)/usr/lib -lpthread -lcdb -lutils -lmpdclient

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(call TSYNC,$(PKG_BUILD_DIR))
endef

define Build/Compile
	CC="$(TARGET_CC)" \
	CFLAGS="$(TARGET_CFLAGS)" \
	LDFLAGS="$(TARGET_LDFLAGS)" \
	$(MAKE) -C $(PKG_BUILD_DIR) \
		all
endef

define Package/zmtc/install
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_DATA) ./files/etc/inittab $(1)/etc/
	$(INSTALL_BIN) ./files/etc/dhcpcd.sh $(1)/etc/
	$(INSTALL_DIR) $(1)/etc/ifplugd
	$(INSTALL_BIN) ./files/etc/ifplugd/ifplugd.action $(1)/etc/ifplugd/ifplugd.action
	$(INSTALL_DIR) $(1)/etc/ip-up.d
	$(INSTALL_BIN) ./files/etc/ip-up.d/wan $(1)/etc/ip-up.d/wan
	$(INSTALL_DIR) $(1)/etc/ip-down.d
	$(INSTALL_BIN) ./files/etc/ip-down.d/wan $(1)/etc/ip-down.d/wan
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/musb $(1)/etc/init.d/musb
	$(INSTALL_DIR) $(1)/lib/wdk
#	$(INSTALL_BIN) ./files/lib/wdk/commit $(1)/lib/wdk/commit
#	$(INSTALL_BIN) ./files/lib/wdk/save $(1)/lib/wdk/save
	$(INSTALL_BIN) ./files/lib/wdk/omnicfg $(1)/lib/wdk/omnicfg
	$(INSTALL_BIN) ./files/lib/wdk/omnicfg_apply $(1)/lib/wdk/omnicfg_apply
	$(INSTALL_BIN) ./files/lib/wdk/omnicfg_ctrl $(1)/lib/wdk/omnicfg_ctrl
	$(INSTALL_BIN) ./files/lib/wdk/smrtcfg $(1)/lib/wdk/smrtcfg

	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mtc $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mtc_cli $(1)/usr/bin/
	
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/wdk/wdk-bin $(1)/lib/wdk/wdk-bin
	ln -sf wdk-bin $(1)/lib/wdk/sleep
	ln -sf wdk-bin $(1)/lib/wdk/ap
	ln -sf wdk-bin $(1)/lib/wdk/cdbreset
	ln -sf wdk-bin $(1)/lib/wdk/debug
	ln -sf wdk-bin $(1)/lib/wdk/dhcps
	ln -sf wdk-bin $(1)/lib/wdk/get_channel
	ln -sf wdk-bin $(1)/lib/wdk/logout
	ln -sf wdk-bin $(1)/lib/wdk/ping
	ln -sf wdk-bin $(1)/lib/wdk/reboot
	ln -sf wdk-bin $(1)/lib/wdk/route
	ln -sf wdk-bin $(1)/lib/wdk/wan
	ln -sf wdk-bin $(1)/lib/wdk/mangment
	ln -sf wdk-bin $(1)/lib/wdk/sta
	ln -sf wdk-bin $(1)/lib/wdk/stor
	ln -sf wdk-bin $(1)/lib/wdk/stapoll
	ln -sf wdk-bin $(1)/lib/wdk/status
	ln -sf wdk-bin $(1)/lib/wdk/system
	ln -sf wdk-bin $(1)/lib/wdk/http
	ln -sf wdk-bin $(1)/lib/wdk/sysupgrade
	ln -sf wdk-bin $(1)/lib/wdk/time
	ln -sf wdk-bin $(1)/lib/wdk/upnp
	ln -sf wdk-bin $(1)/lib/wdk/cdbupload
	ln -sf wdk-bin $(1)/lib/wdk/log
	ln -sf wdk-bin $(1)/lib/wdk/logconf
	ln -sf wdk-bin $(1)/lib/wdk/upload
	ln -sf wdk-bin $(1)/lib/wdk/wps
	ln -sf wdk-bin $(1)/lib/wdk/smbc
	ln -sf wdk-bin $(1)/lib/wdk/commit
	ln -sf wdk-bin $(1)/lib/wdk/save
	ln -sf wdk-bin $(1)/lib/wdk/rakey

endef

$(eval $(call BuildPackage,zmtc))
