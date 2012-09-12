################################################################################
## @file:	distclean.mk
## @author	����ï <gcm.ustc.edu>
## @brief	ɾ�����������ļ�����������Ŀ��
## @version	1.0
###############################################################################

empty_parents		= $(if $(filter-out $(1),$(wildcard $(dir $(1))*)),$(1),$(1) $(call empty_parents,$(patsubst %/,%,$(dir $(1)))))

EMPTY_DIRECTORYS	:= $(call empty_parents,$(TARGET_DIRECTORY))

.PHONY: distclean
distclean:
	@$(ECHO) clean $(EMPTY_DIRECTORYS)
	@$(RM) $(EMPTY_DIRECTORYS)
