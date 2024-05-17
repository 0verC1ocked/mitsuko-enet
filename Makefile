.PHONY: clean debug release

BUILD_DIR := ./build
OBJ_DIR := $(BUILD_DIR)/obj
SRC_DIR := ./src
INC_DIR := ./inc
DEP_DIR := ./../payload-builder
LIBS := -lprotobuf -lpthread -lzmq -lenet #-lcpr

PCH_FILENAME = $(SRC_DIR)/server.h
PCH = $(SRC_DIR)/$(PCH_FILENAME).gch

EXECUTABLE := server

HEADERS := $(shell find $(INC_DIR) -name '*.h' -or -name '*.hpp')
SRCS := $(shell find $(SRC_DIR) -name '*.cpp' -or -name '*.c' -or -name "*.cc" -or -name '*.s')
OBJS := $(SRCS:%=$(OBJ_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

CPPFLAGS := $(INC_FLAGS) -MMD -MP
CXXFLAGS += -std=c++17 -w # Add -g flag for debug information

debug: CXXFLAGS += -g -ggdb3
debug: $(BUILD_DIR)/$(EXECUTABLE)

sanitize: LDFLAGS += -fsanitize=address
sanitize: CXXFLAGS += -g -ggdb3 -fsanitize=address
sanitize: $(BUILD_DIR)/$(EXECUTABLE)

release: CXXFLAGS += -O3 -fsanitize=address
release: $(BUILD_DIR)/$(EXECUTABLE)

releasedb: CXXFLAGS += -g -ggdb3 -O3
releasedb: $(BUILD_DIR)/$(EXECUTABLE)

$(BUILD_DIR)/$(EXECUTABLE): $(PCH) $(OBJS)
	@echo "Linking..."
	clang++ $(OBJS) $(LIBS) -o $@ $(LDFLAGS) $(CXXFLAGS) -v

$(OBJ_DIR)/%.cpp.o: %.cpp
	@echo "Compiling sources..."
	mkdir -p $(dir $@)
	clang++ $(CPPFLAGS) $(CXXFLAGS) -I $(INC_DIR) -I $(SRC_DIR) -I $(DEP_DIR) -c $< -o $@

$(OBJ_DIR)/%.cc.o: %.cc
	@echo "Compiling sources..."
	mkdir -p $(dir $@)
	clang++ $(CPPFLAGS) $(CXXFLAGS) -I $(INC_DIR) -I $(SRC_DIR) -I $(DEP_DIR) -c $< -o $@

$(PCH): $(HEADERS)
	@echo "Building Precompiled Header..."
	clang++ $(CXXFLAGS) -I $(INC_DIR) -I $(SRC_DIR) -I $(DEP_DIR) -x c++-header $(PCH_FILENAME)


clean:
	rm -r $(BUILD_DIR)
