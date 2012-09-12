################################################################################
## @file:	global.mk
## @author	郭春茂 <gcm.ustc.edu>
## @brief	全局设置.
## @version	1.0
###############################################################################

include $(ROOT_MAKE_DIRECTORY)/func/dirs.mk

GLOBAL_COMPILE_FLAGS		:= $(GLOBAL_COMPILE_FLAGS) -DBOOST_USER_CONFIG=\<boost_config.h\>
GLOBAL_COMPILE_FLAGS		:= $(GLOBAL_COMPILE_FLAGS) -DFRAMEWORK_USER_CONFIG=\<framework_config.h\>

PLATFORM_LOCAL_NAME		:= $(patsubst $(shell cd $(ROOT_BUILD_DIRECTORY) ; pwd)%,%,$(shell pwd))

PLATFORM_STRATEGY_NAME		:= $(firstword $(subst /, ,$(PLATFORM_LOCAL_NAME)))

LOCAL_NAME			:= $(patsubst /$(PLATFORM_STRATEGY_NAME)%,%,$(PLATFORM_LOCAL_NAME))

PLATFORM_NAME			:= $(subst ., ,$(PLATFORM_STRATEGY_NAME))

STRATEGY_NAME			:= $(word 2,$(PLATFORM_NAME))

PLATFORM_NAME			:= $(firstword $(PLATFORM_NAME))

PLATFORM_DIRECTORY		:= $(ROOT_PLATFORM_DIRECTORY)/$(PLATFORM_NAME)

PLATFORM_BUILD_DIRECTORY		:= $(ROOT_BUILD_DIRECTORY)/$(PLATFORM_STRATEGY_NAME)
        
PLATFORM_TOOL_DIRECTORY		:= $(ROOT_TOOL_DIRECTORY)/$(PLATFORM_NAME)

include $(wildcard $(PLATFORM_DIRECTORY)/make/*.mk)

PLATFORM_TOOL_PATH		:= $(addprefix $(PLATFORM_TOOL_DIRECTORY),$(PLATFORM_TOOL_PATH))
PLATFORM_TOOL_PATH		:= $(foreach path,$(PLATFORM_TOOL_PATH),$(shell cd $(path) ; pwd))
$(foreach path,$(PLATFORM_TOOL_PATH),$(eval PATH:=$(path):$(PATH)))
export PATH

GLOBAL_COMPILE_FLAGS		:= $(GLOBAL_COMPILE_FLAGS) -DPLATFORM_NAME=$(PLATFORM_NAME)
GLOBAL_COMPILE_FLAGS		:= $(GLOBAL_COMPILE_FLAGS) -DTOOL_NAME=$(PLATFORM_TOOL_NAME)
GLOBAL_COMPILE_FLAGS		:= $(GLOBAL_COMPILE_FLAGS) -DSTRATEGY_NAME=$(STRATEGY_NAME)

GLOBAL_COMPILE_FLAGS		:= $(GLOBAL_COMPILE_FLAGS) -fvisibility=hidden

GLOBAL_LINK_FLAGS		:= $(GLOBAL_LINK_FLAGS) -Wl,--exclude-libs,ALL

PLATFORM_INCLUDE_DIRECTORYS	:= $(PLATFORM_INCLUDE_DIRECTORYS) $(PLATFORM_DIRECTORY)

ifneq ($(LOCAL_NAME),/cex)
        ifneq ($(wildcard $(PLATFORM_DIRECTORY)/cex),)
		PLATFORM_INCLUDE_DIRECTORYS	:= $(PLATFORM_INCLUDE_DIRECTORYS) $(PLATFORM_DIRECTORY)/cex
		PLATFORM_DEPENDS		:= $(PLATFORM_DEPENDS) /cex
        endif
endif

COMMON_MAKE_FILES		:= $(addprefix $(PLATFORM_BUILD_DIRECTORY),$(addsuffix /Common.mk,$(LOCAL_NAME) $(call root_directories,$(LOCAL_NAME))) /Common.mk)

-include $(COMMON_MAKE_FILES)

ifneq ($(LOCAL_NAME),/cex)
        include $(ROOT_PROJECT_DIRECTORY)$(LOCAL_NAME)/Makefile.in
else
        include $(ROOT_MAKE_DIRECTORY)$(LOCAL_NAME)/Makefile.in
endif

COMMON_MAKE_FILES		:= $(addprefix $(ROOT_PROJECT_DIRECTORY),$(addsuffix /Common.mk,$(LOCAL_NAME) $(call root_directories,$(LOCAL_NAME))) /Common.mk)

-include $(COMMON_MAKE_FILES)

ifneq ($(STRATEGY_NAME),)
        include $(ROOT_STRATEGY_DIRECTORY)/$(STRATEGY_NAME).mk
endif
