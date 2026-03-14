################################################################################
#
# hydra-ui
#
################################################################################

HYDRA_UI_VERSION = 0.1.0
HYDRA_UI_SITE = $(BR2_EXTERNAL_HYDRA_PATH)/../../ui/dist
HYDRA_UI_SITE_METHOD = local
HYDRA_UI_INSTALL_STAGING = NO
HYDRA_UI_INSTALL_TARGET = YES

# No build step — UI is pre-built on the host (npm run build)
define HYDRA_UI_BUILD_CMDS
endef

define HYDRA_UI_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/var/www/hydra
	cp -a $(@D)/* $(TARGET_DIR)/var/www/hydra/
endef

$(eval $(generic-package))
