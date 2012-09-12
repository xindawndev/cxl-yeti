################################################################################
## @file: 	proj.mk
## @author	����ï <gcm.ustc.edu>
## @brief	�����̵Ĺ����ļ�
## @version	1.0
###############################################################################

include $(ROOT_MAKE_DIRECTORY)/cmd.mk

PROJ_MAKE_DIRECTORY	:= $(ROOT_MAKE_DIRECTORY)/proj

include $(PROJ_MAKE_DIRECTORY)/config.mk

include $(PROJ_MAKE_DIRECTORY)/directs.mk

TARGETS			:= target slink clean distclean info publish

ifeq ($(MAKECMDGOALS),)
	MAKECMDGOALS		:= target
endif

ifeq ($(BUILDABLE),no)
	MAKECMDGOALS		:= $(subst target,slink,$(MAKECMDGOALS))
endif
include $(patsubst %,$(PROJ_MAKE_DIRECTORY)/%.mk,$(filter $(TARGETS),$(MAKECMDGOALS)))
