add_executable (client "main.cpp" "Packet.h")

target_link_libraries(client
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET client PROPERTY CXX_STANDARD 20)
endif()
