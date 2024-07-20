cmake_minimum_required(VERSION 3.26)
project("gdext-relay_peer")

set (${PROJECT_NAME}_ADDITIONAL_SRCS "relay.gdextension")

# Just so this shows in the IDE
add_custom_target(${PROJECT_NAME}_EXTRAS SOURCES ${${PROJECT_NAME}_ADDITIONAL_SRCS})

# The actual extension. Only half the files are even new
add_library(${PROJECT_NAME} SHARED
        "src/register_types.h"
        "src/relay_peer.h"
        "src/register_types.cpp"
        "src/relay_peer.cpp"
)