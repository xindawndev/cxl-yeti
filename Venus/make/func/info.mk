################################################################################
## @file:	info.mk
## @author	郭春茂 <gcm.ustc.edu>
## @brief	提取项目信息的函数
## @version	1.0
###############################################################################

# 通用提取项目信息
# argment1:	项目名称
# argment2: 	提取的信息项
# return:	该信息项的值

define get_item_info
$(strip \
	$(shell $(MAKE) -C $(PLATFORM_BUILD_DIRECTORY) LOCAL_NAME=$(1) config="$(strip $(config))" info | \
		awk -F : '{ if ($$1 == "$(2)") print $$2 }') \
)
endef

# 通用提取项目信息到变量中（可以同时多项）
# argment1:	项目名称
# argment2: 	提取的信息项（多项）
# argment3: 	变量名后缀（默认为项目名，/转换为_）
# return:	变量名称列表

define get_item_info_as_var
$(eval get_item_info_as_var_ret:=$(strip \
	$(shell $(MAKE) -C $(PLATFORM_BUILD_DIRECTORY) LOCAL_NAME=$(1) config="$(strip $(config))" info | \
		awk -F : ' 
			BEGIN { $(foreach info,$(2),infos["$(info)"];) suffix="$(if $(3),$(3),$(subst /,_,$(1)))"; } 
			{ if ($$1 in infos) printf "%s%s $$(eval %s%s:=%s)", $$1, suffix, $$1, suffix, $$2; } 
		' \
	) \
))$(get_item_info_as_var_ret)
endef


# 提取项目类型
# argment1:	项目名称
# return:	项目类型

define get_item_type
$(call get_item_info,$(1),Type)
endef

# 提取工程直接依赖项
# argment1:	项目名称
# return:	项目直接依赖项

define get_item_depends
$(call get_item_info,$(1),Depends)
endef
