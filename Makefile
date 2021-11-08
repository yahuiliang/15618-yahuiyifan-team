CXX := g++ -m64 -std=c++11
CXXFLAGS := -I. -O3 -Wall
APP_NAME := BST
BUILD_DIR := build
OBJS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(wildcard *.cpp))

default: dirs $(APP_NAME)

$(APP_NAME): $(OBJS)
	$(CXX) -o $@ $^

$(BUILD_DIR)/%.o: %.cpp
	$(CXX) $< $(CXXFLAGS) -c -o $@

dirs:
	mkdir -p $(BUILD_DIR)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)/*.o $(APP_NAME)