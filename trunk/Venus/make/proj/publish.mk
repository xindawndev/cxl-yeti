################################################################################
## @file:	publish.mk
## @author	����ï <gcm.ustc.edu>
## @brief	����Ŀ��
## @version	1.0
###############################################################################

include $(PROJ_MAKE_DIRECTORY)/version.mk

PUBLISH_MAKE_DIRECTORY		:= $(PROJ_MAKE_DIRECTORY)/publish

TARGET_FILE_PRIVATE		:= $(TARGET_FILE_VERSION)

TARGET_FILE_PUBLIC		:= $(TARGET_FILE_VERSION)

ifeq ($(PROJECT_TYPE),lib)
	PRIVATE_DIRECTORY	:= $(ROOT_LIBRARY_DIRECTORY)$(LOCAL_NAME)

        ifeq ($(CONFIG_LIB)-$(DYNAMIC_NAME_SUFFIX),dynamic-.dll)
		TARGET_FILE_PRIVATE	:= $(TARGET_FILE_VERSION:%.dll=%.a)
        endif

else
	PRIVATE_DIRECTORY	:= $(ROOT_BINARY_DIRECTORY)$(LOCAL_NAME)
endif


PUBLIC_DIRECTORY	:= $(ROOT_PUBLISH_DIRECTORY)/$(PLATFORM_STRATEGY_NAME)

TEST_DIRECTORY		:= $(ROOT_TEST_DIRECTORY)/$(PLATFORM_STRATEGY_NAME)

ifeq ($(findstring test,$(CONFIG_publish)),test)

        include $(ROOT_MAKE_DIRECTORY)/func/dirs.mk

	REVERT_TEST_DIRECTORY	:= $(strip $(call revert_directory,$(ROOT_TEST_DIRECTORY)/$(PLATFORM_STRATEGY_NAME)))

endif

.PHONY: publish
publish : $(addprefix publish-,$(CONFIG_publish))

.PHONY: publish-private
publish-private : $(TARGET_DIRECTORY)/$(TARGET_FILE_PRIVATE)
	@$(ECHO) "$(MKDIR) $(PRIVATE_DIRECTORY)"
	@$(MKDIR) $(PRIVATE_DIRECTORY)
	@$(ECHO) "$(LN) $(TARGET_DIRECTORY)/$(TARGET_FILE_PRIVATE) $(PRIVATE_DIRECTORY)/$(TARGET_FILE_PRIVATE)"
	@$(LN) $(TARGET_DIRECTORY)/$(TARGET_FILE_PRIVATE) $(PRIVATE_DIRECTORY)/$(TARGET_FILE_PRIVATE)

.PHONY: publish-public
publish-public : $(TARGET_DIRECTORY)/$(TARGET_FILE_PUBLIC)
	@$(ECHO) "$(MKDIR) $(PUBLIC_DIRECTORY)"
	@$(MKDIR) $(PUBLIC_DIRECTORY)
	@$(ECHO) ------$(TARGET_FILE_PUBLIC)
	@$(ECHO) "$(LN) $(TARGET_DIRECTORY)/$(TARGET_FILE_PUBLIC) $(PUBLIC_DIRECTORY)/$(TARGET_FILE_PUBLIC)"
	@$(LN) $(TARGET_DIRECTORY)/$(TARGET_FILE_PUBLIC) $(PUBLIC_DIRECTORY)/$(TARGET_FILE_PUBLIC)

.PHONY: publish-test
publish-test : $(TARGET_DIRECTORY)/$(TARGET_FILE_PUBLIC)
	@$(ECHO) "$(MKDIR) $(TEST_DIRECTORY)"
	@$(MKDIR) $(TEST_DIRECTORY) 
	@$(ECHO) "$(LN) -s $(REVERT_TEST_DIRECTORY)/$(TARGET_DIRECTORY)/$(TARGET_FILE_PUBLIC) $(TEST_DIRECTORY)/$(TARGET_FILE_PUBLIC)"
	@$(LN) -s $(REVERT_TEST_DIRECTORY)/$(TARGET_DIRECTORY)/$(TARGET_FILE_PUBLIC) $(TEST_DIRECTORY)/$(TARGET_FILE_PUBLIC)