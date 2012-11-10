PLATFORM_INCLUDE_DIRECTORYS	:= $(PLATFORM_TOOL_DIRECTORY)/android-ndk-r4-crystax/build/platforms/android-3/arch-arm/usr/include
PLATFORM_LIBRARY_DIRECTORYS	:= $(PLATFORM_TOOL_DIRECTORY)/android-ndk-r4-crystax/build/platforms/android-3/arch-arm/usr/lib

PLATFORM_COMPILE_FLAGS		:= $(PLATFORM_COMPILE_FLAGS) -fpic
PLATFORM_COMPILE_FLAGS		:= $(PLATFORM_COMPILE_FLAGS) -mthumb-interwork
PLATFORM_COMPILE_FLAGS		:= $(PLATFORM_COMPILE_FLAGS) -ffunction-sections
PLATFORM_COMPILE_FLAGS		:= $(PLATFORM_COMPILE_FLAGS) -funwind-tables
PLATFORM_COMPILE_FLAGS		:= $(PLATFORM_COMPILE_FLAGS) -fstack-protector
PLATFORM_COMPILE_FLAGS		:= $(PLATFORM_COMPILE_FLAGS) -fno-short-enums 
PLATFORM_COMPILE_FLAGS		:= $(PLATFORM_COMPILE_FLAGS) -Wno-psabi
PLATFORM_COMPILE_FLAGS		:= $(PLATFORM_COMPILE_FLAGS) -march=armv5te
PLATFORM_COMPILE_FLAGS		:= $(PLATFORM_COMPILE_FLAGS) -mtune=xscale
PLATFORM_COMPILE_FLAGS		:= $(PLATFORM_COMPILE_FLAGS) -msoft-float
PLATFORM_COMPILE_FLAGS		:= $(PLATFORM_COMPILE_FLAGS) -mthumb
PLATFORM_COMPILE_FLAGS		:= $(PLATFORM_COMPILE_FLAGS) -fomit-frame-pointer
PLATFORM_COMPILE_FLAGS		:= $(PLATFORM_COMPILE_FLAGS) -fno-strict-aliasing
PLATFORM_COMPILE_FLAGS		:= $(PLATFORM_COMPILE_FLAGS) -finline-limit=64
PLATFORM_COMPILE_FLAGS		:= $(PLATFORM_COMPILE_FLAGS) -Wa,--noexecstack
PLATFORM_COMPILE_FLAGS		:= $(PLATFORM_COMPILE_FLAGS) -D__ARM_ARCH_5__
PLATFORM_COMPILE_FLAGS		:= $(PLATFORM_COMPILE_FLAGS) -D__ARM_ARCH_5T__
PLATFORM_COMPILE_FLAGS		:= $(PLATFORM_COMPILE_FLAGS) -D__ARM_ARCH_5E__
PLATFORM_COMPILE_FLAGS		:= $(PLATFORM_COMPILE_FLAGS) -D__ARM_ARCH_5TE__
PLATFORM_COMPILE_FLAGS		:= $(PLATFORM_COMPILE_FLAGS) -DANDROID 

PLATFORM_COMPILE_FLAGS		:= $(PLATFORM_COMPILE_FLAGS) -D__ANDROID__=1

PLATFORM_LINK_FLAGS		:= $(PLATFORM_LINK_FLAGS) -nostdlib
PLATFORM_LINK_FLAGS		:= $(PLATFORM_LINK_FLAGS) -Bdynamic
PLATFORM_LINK_FLAGS		:= $(PLATFORM_LINK_FLAGS) -Wl,-dynamic-linker,/system/bin/linker
PLATFORM_LINK_FLAGS		:= $(PLATFORM_LINK_FLAGS) -Wl,--gc-sections
PLATFORM_LINK_FLAGS		:= $(PLATFORM_LINK_FLAGS) -Wl,-z,nocopyreloc
PLATFORM_LINK_FLAGS		:= $(PLATFORM_LINK_FLAGS) -Wl,--no-undefined
PLATFORM_LINK_FLAGS		:= $(PLATFORM_LINK_FLAGS) -Wl,-z,noexecstack
PLATFORM_LINK_FLAGS		:= $(PLATFORM_LINK_FLAGS) -L$(PLATFORM_LIBRARY_DIRECTORYS)

PLATFORM_CRTBEGIN_STATICBIN	:= $(wildcard $(addsuffix /crtbegin_static.o,$(PLATFORM_LIBRARY_DIRECTORYS)))
PLATFORM_CRTEND_STATICBIN	:= $(wildcard $(addsuffix /crtend_android.o,$(PLATFORM_LIBRARY_DIRECTORYS)))
PLATFORM_CRTBEGIN_DYNAMICBIN	:= $(wildcard $(addsuffix /crtbegin_dynamic.o,$(PLATFORM_LIBRARY_DIRECTORYS)))
PLATFORM_CRTEND_DYNAMICBIN	:= $(wildcard $(addsuffix /crtend_android.o,$(PLATFORM_LIBRARY_DIRECTORYS)))
# 暂不用
#PLATFORM_CRTBEGIN_DYNAMIC	:= $(wildcard $(addsuffix /crtbegin_so.o,$(PLATFORM_LIBRARY_DIRECTORYS)))
#PLATFORM_CRTEND_DYNAMIC		:= $(wildcard $(addsuffix /crtend_so.o,$(PLATFORM_LIBRARY_DIRECTORYS)))
	
PROJECT_DEPEND_LIBRARYS	:= $(PROJECT_DEPEND_LIBRARYS) stdc++ supc++ c m gcc c dl

PLATFORM_DISABLE_FLAGS		:= -pthread