# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Werror -Wpedantic -Wall -std=c++17 -I$(SRC_DIR) -I$(TEST_DIR)

# Linker flags (note that tests need gtest libraries)
LDFLAGS = -lgtest -lgtest_main -pthread

# Directories
SRC_DIR = src
TEST_DIR = tests
OBJ_DIR = tmp
BIN_DIR = .

# Source file sets
# - Common sources: all .cpp files in src/ except main.cpp
COMMON_SRCS = $(filter-out $(SRC_DIR)/main.cpp, $(wildcard $(SRC_DIR)/*.cpp))
# - Test sources: all .cpp files in tests/
TEST_SRCS = $(wildcard $(TEST_DIR)/*.cpp)
# - Main source
MAIN_SRCS = $(SRC_DIR)/main.cpp

# Object file output paths
COMMON_OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(COMMON_SRCS))
TEST_OBJS   = $(patsubst $(TEST_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(TEST_SRCS))
MAIN_OBJ    = $(OBJ_DIR)/main.o

# Final targets
MAIN_TARGET = $(BIN_DIR)/main
TEST_TARGET = $(BIN_DIR)/test

# Default target builds main binary
all: $(MAIN_TARGET)

# Build test binary when requested
test: $(TEST_TARGET)

# Link main binary from common objects and main.o
$(MAIN_TARGET): $(COMMON_OBJS) $(MAIN_OBJ)
	@echo "Linking main binary..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Link test binary from common objects and test objects
$(TEST_TARGET): $(COMMON_OBJS) $(TEST_OBJS)
	@echo "Linking test binary..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile common source files (*.cpp in src/ except main.cpp)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	@echo "Compiling common source $<..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Compile main source file
$(MAIN_OBJ): $(MAIN_SRCS)
	@mkdir -p $(OBJ_DIR)
	@echo "Compiling main source $<..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Compile test source files from tests/
$(OBJ_DIR)/%.o: $(TEST_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	@echo "Compiling test source $<..."
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Cleanup object files and binaries
clean:
	rm -rf $(OBJ_DIR)

cleanall: clean
	rm -f $(MAIN_TARGET) $(TEST_TARGET)

.PHONY: all test clean cleanall
