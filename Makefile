PREFIX ?= $(shell pwd)/../prefix/$(CROSS:%-=%)
NAME   := lwip
TARGET :=arm-none-eabi
CROSS  :=$(TARGET)-
SOC    ?=soc-sim
CPU    :=arm
OS     :=hcos
IPV    :=ipv4
INCLUDE:= \
	-Isrc/include \
	-Isrc/include/$(IPV) \
	-Isrc/$(OS) \
	-I$(PREFIX)/include \
	-I$(PREFIX)/include/$(SOC)

COPTS  ?=-march=armv7-a -mthumb
AARCH  :=$(shell echo $(COPTS) | sed -e 's/.*armv\([0-9]\).*/\1/g')
MOPTS  :=$(COPTS) \
          -DCFG_AARCH=$(AARCH) \
          -msoft-float -fno-builtin -fno-common \
          -ffunction-sections -fdata-sections
CONFIG :=
ASFLAGS:=$(MOPTS) $(CONFIG) -O2 -g -Wall -Werror -D __ASSEMBLY__
CFLAGS :=$(MOPTS) $(CONFIG) -O2 -g -Wall -Werror -fno-builtin
LSCRIPT:=rom.ld
LDFLAGS:=$(MOPTS) -g -nostartfiles -nodefaultlibs -L $(PREFIX)/lib -T$(LSCRIPT)
MSCRIPT:=$(PREFIX)/share/mod.ld
LIB    :=lib$(NAME).a

ALL    := lib
CLEAN  :=

VPATH  := src/$(OS)/arch \
	src/core \
	src/core/ipv4 \
	src/api \
	src/netif
#	test/unit/udp \
#	test/unit \
#	test/unit/core \
#	test/unit/etharp \
#	test/unit/tcp
VOBJ   :=$(patsubst %.S,%.o, \
		$(patsubst %.c,%.o, \
		$(patsubst %.cpp, %.o, \
			$(notdir $(foreach DIR,$(VPATH),\
				$(wildcard $(DIR)/*.S)	\
				$(wildcard $(DIR)/*.c) 	\
				$(wildcard $(DIR)/*.cpp))))))
default:all

include $(PREFIX)/share/Makefile.rule

install:
	rm -rf $(PREFIX)/include/$(NAME)
	install -d $(PREFIX)/include/$(NAME) $(PREFIX)/include/$(NAME)/arch
	cp -r src/include/* $(PREFIX)/include/$(NAME)
	install -Dp src/$(OS)/*.h $(PREFIX)/include/$(NAME)
	install -Dp src/$(OS)/arch/*.h $(PREFIX)/include/$(NAME)/arch
	install -Dp $(LIB)  $(PREFIX)/lib/
	find -name "*.[hcS]" > $(PREFIX)/doc/$(NAME).files
	find -name "*.cpp"  >> $(PREFIX)/doc/$(NAME).files
	sed -e 's/^\./$(shell pwd|sed -e 's/\//\\\//g')/g' -i $(PREFIX)/doc/$(NAME).files
