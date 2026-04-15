#
# Makefile
#
CC = arm-linux-gcc
LVGL_DIR_NAME ?= lvgl
LVGL_DIR ?= ${shell pwd}
CFLAGS ?= -O3 -g0 -I$(LVGL_DIR)/ -I$(LVGL_DIR)/include/ -Wall -Wshadow -Wundef -Wmissing-prototypes -Wno-discarded-qualifiers -Wall -Wextra -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith -fno-strict-aliasing -Wno-error=cpp -Wuninitialized -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security -Wno-ignored-qualifiers -Wno-error=pedantic -Wno-sign-compare -Wno-error=missing-prototypes -Wdouble-promotion -Wclobbered -Wdeprecated -Wempty-body -Wtype-limits -Wstack-usage=2048 -Wno-unused-value -Wno-unused-parameter -Wno-missing-field-initializers -Wuninitialized -Wmaybe-uninitialized -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wpointer-arith -Wno-cast-qual -Wmissing-prototypes -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wno-discarded-qualifiers -Wformat-security -Wno-ignored-qualifiers -Wno-sign-compare
LDFLAGS ?= -lm -lpthread
BIN = demo


#Collect the files to compile
MAINSRC = ./main.c

include $(LVGL_DIR)/lvgl/lvgl.mk
include $(LVGL_DIR)/lv_drivers/lv_drivers.mk

CSRCS +=$(LVGL_DIR)/mouse_cursor_icon.c 
CSRCS +=$(LVGL_DIR)/src/ui_pages.c
CSRCS +=$(LVGL_DIR)/src/ui_home.c
CSRCS +=$(LVGL_DIR)/src/ui_light.c
CSRCS +=$(LVGL_DIR)/src/ui_setting.c
CSRCS +=$(LVGL_DIR)/src/ui_robot.c
CSRCS +=$(LVGL_DIR)/src/ui_add_device.c
CSRCS +=$(LVGL_DIR)/src/ui_time_client.c
CSRCS +=$(wildcard img/*.c)

OBJEXT ?= .o

# 设置输出目录
BUILD_DIR = ./build
BIN_DIR = ./bin

# 确保目录存在
$(shell mkdir -p $(BUILD_DIR) $(BIN_DIR))

# 定义目标文件
AOBJS = $(patsubst %.S,$(BUILD_DIR)/%.o,$(ASRCS))
COBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(CSRCS))
MAINOBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(MAINSRC))

SRCS = $(ASRCS) $(CSRCS) $(MAINSRC)
OBJS = $(AOBJS) $(COBJS)

## MAINOBJ -> OBJFILES

all: default

# 通用编译规则
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@$(CC)  $(CFLAGS) -c $< -o $@
	@echo "CC $<"

$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	@$(CC)  $(CFLAGS) -c $< -o $@
	@echo "AS $<"
    
default: $(AOBJS) $(COBJS) $(MAINOBJ)
	$(CC) -o $(BIN_DIR)/$(BIN) $(MAINOBJ) $(AOBJS) $(COBJS) $(LDFLAGS)

clean: 
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "Clean done"

