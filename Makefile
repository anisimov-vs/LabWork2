# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Werror -Wpedantic -Wall -std=c++17 -I$(SRC_DIR)

# Linker flags
LDFLAGS = -lgtest -lgtest_main -pthread

# Directories
SRC_DIR = src
OBJ_DIR = tmp
BIN_DIR = .

# Common source files (excluding main.cpp and test.cpp)
COMMON_SRCS = $(wildcard $(SRC_DIR)/*.cpp)
COMMON_SRCS := $(filter-out $(SRC_DIR)/main.cpp $(SRC_DIR)/test.cpp, $(COMMON_SRCS))

# Specific source files
MAIN_SRCS = $(SRC_DIR)/main.cpp
TEST_SRCS = $(SRC_DIR)/test.cpp

# Generate object files for common sources
COMMON_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(COMMON_SRCS))

# Object files for main and test
MAIN_OBJ = $(OBJ_DIR)/main.o
TEST_OBJ = $(OBJ_DIR)/test.o

# Target binaries
MAIN_TARGET = $(BIN_DIR)/main
TEST_TARGET = $(BIN_DIR)/test

# Default target
all: $(MAIN_TARGET)

# Test target
test: $(TEST_TARGET)

# Build main binary
$(MAIN_TARGET): $(COMMON_OBJS) $(MAIN_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Build test binary
$(TEST_TARGET): $(COMMON_OBJS) $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile common source files
$(COMMON_OBJS): $(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Compile main.cpp
$(MAIN_OBJ): $(MAIN_SRCS)
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Compile test.cpp
$(TEST_OBJ): $(TEST_SRCS)
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Clean up
clean:
	rm -rf $(OBJ_DIR)

cleanall: clean
	rm -rf $(MAIN_TARGET) $(TEST_TARGET)

# Phony targets
.PHONY: all clean cleanall