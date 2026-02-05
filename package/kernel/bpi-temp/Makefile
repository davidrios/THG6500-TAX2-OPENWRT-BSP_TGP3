#
# Copyright (C) 2026 OpenWrt.org
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=bpi-temp
PKG_VERSION:=1.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/bpi-temp
  SUBMENU:=Hardware Monitoring Support
  TITLE:=BPI-Wifi6 Temperature Sensor Module
  FILES:=$(PKG_BUILD_DIR)/bpi_temp.ko
  AUTOLOAD:=$(call AutoLoad,95,bpi_temp)
endef

define KernelPackage/bpi-temp/description
  Kernel module to expose Triductor HAL temperature via /proc/bpi_temp
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(KERNEL_MAKE) M="$(PKG_BUILD_DIR)" KBUILD_MODPOST_WARN=1 modules
endef

$(eval $(call KernelPackage,bpi-temp))
