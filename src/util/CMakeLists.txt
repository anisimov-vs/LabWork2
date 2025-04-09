# Create a library for utility components
add_library(deckstiny_util STATIC logger.cpp)

# Include directories
target_include_directories(deckstiny_util PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Link any required libraries
target_link_libraries(deckstiny_util 
    PUBLIC nlohmann_json::nlohmann_json
)

# Set compiler flags
if(MSVC)
    target_compile_options(deckstiny_util PRIVATE /W4)
else()
    target_compile_options(deckstiny_util PRIVATE -Wall -Wextra -Wpedantic)
endif() 