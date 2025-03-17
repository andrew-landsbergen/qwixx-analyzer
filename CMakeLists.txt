# Specify minimum required CMake version
cmake_minimum_required(VERSION 3.28)

# Set project name and version
project(QwixxAnalyzer VERSION 1.0)

# Specify the C++ standard
add_library(compiler_flags INTERFACE)
target_compile_features(compiler_flags INTERFACE cxx_std_20)

# Add compiler flags only when building this project
set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")
target_compile_options(compiler_flags INTERFACE
    "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wall;-Wextra;-Wpedantic;-Werror;-g3;-O3>>"
    "$<${msvc_cxx}:$<BUILD_INTERFACE:-W3>>"
)

# Add the executable
add_executable(QwixxAnalyzer src/qwixx.cpp src/game.cpp)

target_link_libraries(QwixxAnalyzer PUBLIC compiler_flags)

target_include_directories(QwixxAnalyzer PUBLIC include)
    