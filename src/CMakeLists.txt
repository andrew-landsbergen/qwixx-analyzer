# Define source files not defining "main" as a static library for linking
add_library(game STATIC game.cpp)

# Link compiler_flags (defined at top level)
target_link_libraries(game PUBLIC compiler_flags)

# Specify that anyone including this library should include
# the current source directory for header files
target_include_directories(game INTERFACE {CMAKE_CURRENT_SOURCE_DIR})