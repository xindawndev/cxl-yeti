################################################################################
## @file: 	config.mk
## @author	����ï <gcm.ustc.edu>
## @brief	������Ŀ������.
## @version	1.0
###############################################################################

include $(ROOT_MAKE_DIRECTORY)/name.mk

CONFIG_COMPILE_LIST	:= debug release

CONFIG_THREAD_LIST	:= multi single

CONFIG_LIB_LIST		:= static dynamic

CONFIG_COMPILE		:= $(call get_config,$(CONFIG),$(CONFIG_COMPILE_LIST),debug)

CONFIG_THREAD		:= $(call get_config,$(CONFIG),$(CONFIG_THREAD_LIST),multi)

TARGET_DIRECTORY	:= $(CONFIG_COMPILE)

ifeq ($(CONFIG_COMPILE),debug)
	NAME_CONFIG		:= gd
	CONFIG_COMPILE_FLAGS	:= $(CONFIG_COMPILE_FLAGS) -D_DEBUG
else
	CONFIG_COMPILE_FLAGS	:= $(CONFIG_COMPILE_FLAGS) -D_NDEBUG
endif

ifeq ($(PROJECT_TYPE),lib)

	CONFIG_COMPILE_FLAGS	:= $(CONFIG_COMPILE_FLAGS) -D_LIB

	CONFIG_LIB		:= $(call get_config,$(CONFIG),$(CONFIG_LIB_LIST),static)

	TARGET_DIRECTORY	:= $(TARGET_DIRECTORY)/$(CONFIG_LIB)

        ifeq ($(CONFIG_LIB),static)

		NAME_CONFIG		:= s$(NAME_CONFIG)

		NAME_PREFIX		:= $(STATIC_NAME_PREFIX)

		NAME_SUFFIX		:= $(STATIC_NAME_SUFFIX)

		CONFIG_COMPILE_FLAGS	:= $(CONFIG_COMPILE_FLAGS) -D_STATIC

        else

		NAME_PREFIX		:= $(DYNAMIC_NAME_PREFIX)

		NAME_SUFFIX		:= $(DYNAMIC_NAME_SUFFIX)

		CONFIG_COMPILE_FLAGS	:= $(CONFIG_COMPILE_FLAGS) -D_DYNAMIC

        endif

else

	CONFIG_COMPILE_FLAGS	:= $(CONFIG_COMPILE_FLAGS) -D_BIN

	NAME_PREFIX	:= $(BIN_NAME_PREFIX)

	NAME_SUFFIX	:= $(BIN_NAME_SUFFIX)

endif

TARGET_DIRECTORY	:= $(TARGET_DIRECTORY)/$(CONFIG_THREAD)

ifneq ($(NAME_CONFIG),)
	NAME_CONFIG		:= -$(NAME_CONFIG)
endif

ifeq ($(CONFIG_THREAD),multi)
	NAME_CONFIG		:= -mt$(NAME_CONFIG)
	CONFIG_COMPILE_FLAGS	:= $(CONFIG_COMPILE_FLAGS) -D_MULTI
else
	CONFIG_COMPILE_FLAGS	:= $(CONFIG_COMPILE_FLAGS) -D_SINGLE
endif

DEPEND_DIRECTORY	:= $(TARGET_DIRECTORY)/.deps

OBJECT_DIRECTORY	:= $(TARGET_DIRECTORY)/.objs
