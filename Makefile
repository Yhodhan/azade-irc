.PHONY: all

BUILD = build
TARGET = $(BUILD)/azade-irc
SRC_DIR = src

SRCS = $(shell find $(SRC_DIR) -name '*.cpp')
OBJS = $(patsubst %.cpp, %.o, $(SRCS)) 

CXXFLAGS = --std=c++17 -g

all: $(TARGET) 
	@$(BUILD)/azade-irc

$(TARGET): $(OBJS) 
	@ mkdir -p $(BUILD)
	@$(CXX) -o $@ $^

%.o: %.cpp
	@$(CXX) $(CXXFLAGS) -c $^ -o $@

clean:
	rm -rf $(BUILD) $(OBJS)
