CXX := g++ -m64 -std=c++11
CXXFLAGS := -g -Wall
APP_NAME := main
OBJS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))

default: $(APP_NAME)

$(APP_NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ 

%.o: %.cpp *.h
	$(CXX) $< $(CXXFLAGS) -c -o $@

.PHONY: clean
clean:
	rm -rf *.o $(APP_NAME)