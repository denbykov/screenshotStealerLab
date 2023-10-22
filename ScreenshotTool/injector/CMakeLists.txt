set (SOURCES
	main.cpp
)

add_executable (injector ${SOURCES})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET injector PROPERTY CXX_STANDARD 20)
endif()
