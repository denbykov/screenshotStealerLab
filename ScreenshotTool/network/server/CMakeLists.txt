add_executable (server "server.cpp" "Packet.h")

target_link_libraries(server
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET server PROPERTY CXX_STANDARD 20)
endif()
