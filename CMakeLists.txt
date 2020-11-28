cmake_minimum_required(VERSION 3.5)
project(aicup2020)

# OS and compiler checks.
if(WIN32)
    add_definitions(-DWIN32)
    SET(PROJECT_LIBS Ws2_32.lib)
endif()

file(GLOB HEADERS "*.hpp" ".h" "model/*.hpp")
SET_SOURCE_FILES_PROPERTIES(${HEADERS} PROPERTIES HEADER_FILE_ONLY TRUE)
file(GLOB SRC "*.cpp" "model/*.cpp")
add_executable(aicup2020 ${HEADERS} ${SRC})
set_property(TARGET aicup2020 PROPERTY CXX_STANDARD 17)
TARGET_LINK_LIBRARIES(aicup2020 ${PROJECT_LIBS})