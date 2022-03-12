ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

include $(YAUL_INSTALL_ROOT)/share/pre.common.mk

# Each asset follows the format: <path>;<symbol>. Duplicates are removed
BUILTIN_ASSETS=

INCLUDEPATHS:= \
	-I. \
	-I../../fixed

SOURCES:= . asm ../../fixed

SOURCEFILES_AS:= $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.sx))
SOURCEFILES_CC:= $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.c))
SOURCEFILES_XX:= $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.cpp))

SH_PROGRAM:= openlara
SH_SRCS:= \
	$(SOURCEFILES_AS) \
	$(SOURCEFILES_CC) \
	$(SOURCEFILES_XX)

SH_LIBRARIES:=
SH_AFLAGS+= \
	$(INCLUDEPATHS)
SH_CFLAGS+= \
	-O2 \
	-ggdb3 \
	$(INCLUDEPATHS) \
	-D__SATURN__ \
	-Wno-error=unused-parameter \
	-Wno-error=extra \
	-save-temps=obj

IP_VERSION:= V1.000
IP_RELEASE_DATE:= 20160101
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= OpenLara
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06001E00
IP_1ST_READ_ADDR:= 0x06004000

include $(YAUL_INSTALL_ROOT)/share/post.common.mk
