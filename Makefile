.PHONY: clean cleanall debug release

BUILD_DIR := ./build
OBJ_DIR := $(BUILD_DIR)/obj
SRC_DIR := ./src
INC_DIR := ./inc
LIBS := -lprotobuf -lpthread -lzmq -lenet 

DEPS_LIB := lib/payloadbuilder/build/libpayloadbuilder.a

PCH_FILENAME = $(SRC_DIR)/server.h
PCH = $(SRC_DIR)/$(PCH_FILENAME).gch

EXECUTABLE := server

HEADERS := $(shell find $(INC_DIR) -name '*.h' -or -name '*.hpp')
SRCS := $(shell find $(SRC_DIR) -name '*.cpp' -or -name '*.c' -or -name "*.cc" -or -name '*.s')
OBJS := $(SRCS:%=$(OBJ_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

LDFLAGS := -Llib/payloadbuilder/build -lpayloadbuilder

CPPFLAGS := $(INC_FLAGS) -MMD -MP
CXXFLAGS += -std=c++17 -w # Add -g flag for debug information

debug: CXXFLAGS += -g -ggdb3
debug: $(BUILD_DIR)/$(EXECUTABLE)

sanitize: LDFLAGS += -fsanitize=address
sanitize: CXXFLAGS += -g -ggdb3 -fsanitize=address
sanitize: $(BUILD_DIR)/$(EXECUTABLE)

release: CXXFLAGS += -O3 -fsanitize=address
release: $(DEPS_LIB) $(BUILD_DIR)/$(EXECUTABLE)

releasedb: CXXFLAGS += -g -ggdb3 -O3
releasedb: $(BUILD_DIR)/$(EXECUTABLE)

$(DEPS_LIB):
	cd lib/payloadbuilder && $(MAKE) release

$(BUILD_DIR)/$(EXECUTABLE): $(PCH) $(OBJS) $(DEPS_LIB) | $(BUILD_DIR)
	@echo "Linking..."
	clang++ $(OBJS) $(LIBS) -o $@ $(LDFLAGS) $(CXXFLAGS) -v

$(OBJ_DIR)/%.cpp.o: %.cpp
	@echo "Compiling sources..."
	mkdir -p $(dir $@)
	clang++ $(CPPFLAGS) $(CXXFLAGS) -I $(INC_DIR) -I $(SRC_DIR) -c $< -o $@

$(OBJ_DIR)/%.cc.o: %.cc
	@echo "Compiling sources..."
	mkdir -p $(dir $@)
	clang++ $(CPPFLAGS) $(CXXFLAGS) -I $(INC_DIR) -I $(SRC_DIR) -c $< -o $@

$(PCH): $(HEADERS)
	@echo "Building Precompiled Header..."
	clang++ $(CXXFLAGS) -I $(INC_DIR) -I $(SRC_DIR) -x c++-header $(PCH_FILENAME)


clean:
	rm -r $(BUILD_DIR)

cleanall:
	rm -r $(BUILD_DIR)
	cd lib/payloadbuilder && $(MAKE) clean
