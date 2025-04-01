TARGET_EXEC ?= gridemu

BUILD_DIR ?= ./build
SRC_DIRS ?= ./src
#SRC_DIRS ?= ./

SRCS := $(shell find $(SRC_DIRS) -name '*.cpp' -or -name '*.c' -or -name '*.s')
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
#INC_FLAGS := $(addprefix -I,$(INC_DIRS))
INC_FLAGS := $(addprefix -I,$(INC_DIRS)) -lpthread -lm `sdl2-config --cflags --libs`

CPPFLAGS ?= $(INC_FLAGS) -g -Wall -MMD -MP

LDFLAGS := -lpthread -lm `sdl2-config --cflags --libs`

#$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# assembly
$(BUILD_DIR)/%.s.o: %.s
	$(MKDIR_P) $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# c++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@


.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)
	$(RM) ./gridemu

-include $(DEPS)

MKDIR_P ?= mkdir -p
