################################################################################
## @file: 	clean.mk
## @author	����ï <gcm.ustc.edu>
## @brief	ɾ����ʱ�ļ����м��ļ�
## @version	1.0
###############################################################################
.PHONY: clean 
clean:
	@$(ECHO) clean $(OBJECT_DIRECTORY) $(DEPEND_DIRECTORY) $(TARGET_FILE_FULL)
	@$(RM) $(TARGET_FILE_FULL) $(OBJECT_DIRECTORY) $(DEPEND_DIRECTORY)
