#
# Makefile
#
ifdef CROSS_COMPILE
CC 	= $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
CPP = $(CC) -E
AS 	= $(CROSS_COMPILE)as
LD	= $(CROSS_COMPILE)ld
AR	= $(CROSS_COMPILE)ar
NM	= $(CROSS_COMPILE)nm
STRIP 	= $(CROSS_COMPILE)strip
endif

LVGL_DIR_NAME 	?= lvgl
LVGL_DIR 		?= .

WARNINGS		:= -Wall -Wextra -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith \
					-fno-strict-aliasing -Wno-error=cpp -Wuninitialized -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess \
					-Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security -Wno-error=pedantic \
					-Wno-sign-compare -Wdouble-promotion -Wclobbered -Wempty-body -Wtype-limits -Wshift-negative-value \
					-Wno-unused-value -Wno-unused-parameter -Wno-missing-field-initializers -Wuninitialized -Wmaybe-uninitialized -Wall -Wextra -Wno-unused-parameter \
					-Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wpointer-arith -Wno-cast-qual \
					-Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security -Wno-sign-compare
CFLAGS 			?= -O3 -g0 -MD -MP -I$(LVGL_DIR)/ $(WARNINGS) 
ifdef CROSS_COMPILE
LDFLAGS 		?= -static -lm -Llibhv/lib -Lspdlog/build -l:libhv.a -latomic -lpthread -lstdc++fs -l:libspdlog.a
else
LDFLAGS 		?= -lm -Llibhv/lib -Lspdlog/build -l:libhv.a -latomic -lpthread -lstdc++fs -l:libspdlog.a
endif
BIN 			= fre3nderscreen
BUILD_DIR 		= ./build
BUILD_OBJ_DIR 	= $(BUILD_DIR)/obj
BUILD_BIN_DIR 	= $(BUILD_DIR)/bin
SPDLOG_DIR		= spdlog

prefix 			?= /usr
bindir 			?= $(prefix)/bin

#Collect the files to compile
MAINSRC = 		$(wildcard $(LVGL_DIR)/src/*.cpp)

include $(LVGL_DIR)/lvgl/lvgl.mk
include $(LVGL_DIR)/lv_drivers/lv_drivers.mk

CSRCS 			+= $(wildcard $(LVGL_DIR)/assets/*.c)
ifdef CROSS_COMPILE
CSRCS			+= $(wildcard $(LVGL_DIR)/lv_touch_calibration/*.c)
endif

ASSET_DIR		= material_46

CSRCS 			+= $(wildcard $(LVGL_DIR)/assets/$(ASSET_DIR)/*.c)

ifdef FRE3NDERSCREEN_VERSION
DEFINES			+= -D FRE3NDERSCREEN_VERSION="\"${FRE3NDERSCREEN_VERSION}\""
endif

OBJEXT 			?= .o

AOBJS 			= $(ASRCS:.S=$(OBJEXT))
COBJS 			= $(CSRCS:.c=$(OBJEXT))

MAINOBJ 		= $(MAINSRC:.cpp=$(OBJEXT))
DEPS                    = $(addprefix $(BUILD_OBJ_DIR)/, $(patsubst %.o, %.d, $(MAINOBJ)))

OBJS 			= $(AOBJS) $(COBJS) $(MAINOBJ)
TARGET 			= $(addprefix $(BUILD_OBJ_DIR)/, $(patsubst ./%, %, $(OBJS)))

INC 				:= -I./ -I./lvgl/ -I./lv_touch_calibration -I./spdlog/include -Ilibhv/include
LDLIBS	 			:= -lm

DEFINES				+= -D _GNU_SOURCE -DSPDLOG_COMPILED_LIB

# SIMULATION is enabled by default, need CROSS_COMPILE variable to do MIPS build
ifndef CROSS_COMPILE
DEFINES +=  -D LV_BUILD_TEST=0 -D SIMULATOR
LDLIBS += -lSDL2
endif

COMPILE_CC				= $(CC) $(CFLAGS) $(INC) $(DEFINES)
COMPILE_CXX				= $(CC) $(CFLAGS) $(INC) $(DEFINES)

## MAINOBJ -> OBJFILES

all: default

libhv.a:
	$(MAKE) -C libhv -j$(nproc) libhv

libspdlog.a:
	@mkdir -p $(SPDLOG_DIR)/build
	@cmake -B $(SPDLOG_DIR)/build -S $(SPDLOG_DIR)/ -DCMAKE_CXX_COMPILER=$(CXX)
	$(MAKE) -C $(SPDLOG_DIR)/build -j$(nproc)

$(BUILD_OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(COMPILE_CXX) -std=c++17 $(CFLAGS) -c $< -o $@
	@echo "CXX $<"

$(BUILD_OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@$(COMPILE_CC)  $(CFLAGS) -c $< -o $@
	@echo "CC $<"

default: $(TARGET)
	@mkdir -p $(dir $(BUILD_BIN_DIR)/)
	$(CXX) -o $(BUILD_BIN_DIR)/$(BIN) $(TARGET) $(LDFLAGS) $(LDLIBS)
	@echo "CXX $<"

spdlogclean:
	rm -rf $(SPDLOG_DIR)/build

libhvclean:
	$(MAKE) -C libhv clean

clean:
	rm -rf $(BUILD_DIR)

install:
	install -d $(DESTDIR)$(bindir)
	install $(BUILD_BIN_DIR)/$(BIN) $(DESTDIR)$(bindir)

uninstall:
	$(RM) -r $(addprefix $(DESTDIR)$(bindir)/,$(BIN))

build:
	$(MAKE) libhvclean
	$(MAKE) libhv.a
	$(MAKE) spdlogclean
	$(MAKE) libspdlog.a
	$(MAKE) clean
	$(MAKE) -j$(nproc)

-include			$(DEPS)
