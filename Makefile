VPATH = src:include

BUILD_DIR = build
INCLUDE_DIR = include
OBJ_DIR = obj

OBJ = $(OBJ_DIR)/qwixx.o

CXX = g++
CXXFLAGS = -I$(INCLUDE_DIR) -Wall -Werror -std=c++20

$(BUILD_DIR)/qwixx : $(OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/%.o : %.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

.PHONY : clean
clean :
	rm -rf $(BUILD_DIR) $(OBJ_DIR)