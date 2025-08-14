# Add the test executable
#add_executable(Tests game_test.cpp)

# Find Catch2 for testing
#find_package(Catch2)

# Link internal "game" library (in src), compiler_flags (defined at top level), 
# and Catch2WithMain so we don't need to specify a "main" function
#target_link_libraries(Tests
#                      PUBLIC game
#                      PUBLIC compiler_flags
#                      PRIVATE Catch2::Catch2WithMain
#                      )