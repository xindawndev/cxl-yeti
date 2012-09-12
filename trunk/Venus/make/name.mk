################################################################################
## @file:	name.mk
## @author	����ï <gcm.ustc.edu>
## @brief	�ļ�����Ŀ¼������
## @version	1.0
###############################################################################

ifeq ($(STATIC_NAME_PREFIX),)
        STATIC_NAME_PREFIX		:= lib
endif

ifeq ($(DYNAMIC_NAME_PREFIX),)
        DYNAMIC_NAME_PREFIX		:= $(STATIC_NAME_PREFIX)
endif

ifeq ($(STATIC_NAME_SUFFIX),)
        STATIC_NAME_SUFFIX		:= .a
endif

ifeq ($(DYNAMIC_NAME_SUFFIX),)
        DYNAMIC_NAME_SUFFIX		:= .so
endif

ifeq ($(SOURCE_SUFFIX),)
        SOURCE_SUFFIX			:= .cpp
endif

