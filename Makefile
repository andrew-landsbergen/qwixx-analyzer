VPATH = src:include

BUILD_DIR = build
INCLUDE_DIR = include
OBJ_DIR = obj

OBJ = $(OBJ_DIR)/qwixx.o $(OBJ_DIR)/game.o

CXX = g++
CXXFLAGS = -I$(INCLUDE_DIR) -Wall -Werror -Wextra -Wpedantic -std=c++20 -g3 -O3

$(BUILD_DIR)/qwixx : $(OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/%.o : %.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

.PHONY : clean
clean :
	rm -rf $(BUILD_DIR) $(OBJ_DIR)