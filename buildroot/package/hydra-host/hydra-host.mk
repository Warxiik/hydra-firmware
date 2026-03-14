################################################################################
#
# hydra-host
#
################################################################################

HYDRA_HOST_VERSION = 0.1.0
HYDRA_HOST_SITE = $(BR2_EXTERNAL_HYDRA_PATH)/../../host
HYDRA_HOST_SITE_METHOD = local
HYDRA_HOST_INSTALL_STAGING = NO
HYDRA_HOST_INSTALL_TARGET = YES
HYDRA_HOST_DEPENDENCIES = spdlog nlohmann-json

HYDRA_HOST_CONF_OPTS = \
	-DBUILD_TESTS=OFF \
	-DCMAKE_BUILD_TYPE=Release

define HYDRA_HOST_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/hydra-host $(TARGET_DIR)/usr/bin/hydra-host
endef

define HYDRA_HOST_INSTALL_INIT_SYSTEMD
	$(INSTALL) -D -m 0644 $(BR2_EXTERNAL_HYDRA_PATH)/rootfs-overlay/etc/systemd/system/hydra-host.service \
		$(TARGET_DIR)/etc/systemd/system/hydra-host.service
endef

$(eval $(cmake-package))
