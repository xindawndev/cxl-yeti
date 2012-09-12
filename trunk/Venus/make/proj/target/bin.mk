################################################################################
## @file: 	bin.mk
## @author	����ï <gcm.ustc.edu>
## @brief	���ɿ�ִ���ļ��Ĺ���
## @version	1.0
###############################################################################

include $(TARGET_MAKE_DIRECTORY)/link.mk

$(TARGET_FILE_FULL): $(SOURCE_OBJECTS) $(DEPEND_FILES) $(MAKEFILE_LIST)
	@$(RM) $@
	$(LD) $(LINK_FLAGS) $(PLATFORM_CRTBEGIN_DYNAMICBIN) $(SOURCE_OBJECTS_FULL) $(LIB_PATHS) $(LIB_NAMES) $(PLATFORM_CRTEND_DYNAMICBIN) -o $@
