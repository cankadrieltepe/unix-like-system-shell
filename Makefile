TARGET_EXEC := stash

CC := gcc

SRC_DIR := ./src
MODULE_DIR := ./module
BUILD_DIR := ./build
DEP_DIR := $(BUILD_DIR)/.deps

MODULE_TARGET = $(MODULE_DIR)/mymodule.o

SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
DEPS := $(patsubst $(SRC_DIR)/%.c, $(DEP_DIR)/%.d, $(SRCS))

WARN_FLAGS += -Wall -Wno-comment -Werror -Wextra -Wpedantic
MAKE_FLAGS += -j
DEP_FLAGS = -MT $@ -MMD -MP -MF $(DEP_DIR)/$*.d
CFLAGS += $(WARN_FLAGS)

INC_DIRS := $(shell find $(SRC_DIR) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

.MAIN: $(TARGET_EXEC)

$(TARGET_EXEC): $(OBJS)
	@echo "COMP304 - Building $(TARGET_EXEC)"
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

all: $(TARGET_EXEC) $(OBJ) $(MODULE_TARGET)

$(MODULE_TARGET): $(MODULE_DIR)/mymodule.c
	@echo 'COMP304 - Building kernel module'
	cd $(MODULE_DIR) && $(MAKE)

$(OBJS) : $(BUILD_DIR)/%.o : $(SRC_DIR)/%.c $(DEP_DIR)/%.d | $(DEP_DIR)
	@echo 'COMP304 - Building object file for $(TARGET_EXEC)'
	@mkdir -p "$(dir $(DEP_DIR)/$*)"
	@mkdir -p $(@D)
	$(CC) $(INC_FLAGS) $(CFLAGS) $(DEP_FLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) $(TARGET_EXEC)
	$(RM) -rd $(BUILD_DIR)
	cd $(MODULE_DIR) && $(MAKE) clean

$(DEP_DIR):
	@mkdir -p $(DEP_DIR)

$(DEPS):

-include $(wildcard $(DEPS))

.PHONY: help
help:
	@echo  'COMP304 - Shell Project Makefile'
	@echo  '--------------------------------'
	@echo  'Targets:'
	@echo  "  $(TARGET_EXEC)         - Compiles the shell (default)"
	@echo  '  all             - Compiles the shell along with the kernel module'
	@echo  ''
	@echo  '  clean           - Removes build files'
