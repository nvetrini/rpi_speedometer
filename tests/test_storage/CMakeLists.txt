# Storage unit tests

# Include parent directory for common test configuration
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# Test source files
target_sources(app PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/test_storage.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mocks.c
)

# Include test headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link with ztest framework
zephyr_link_libraries(ztest)