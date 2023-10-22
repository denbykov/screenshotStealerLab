set (SOURCES
	main.cpp
)

add_library (offender SHARED ${SOURCES})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET offender PROPERTY CXX_STANDARD 20)
endif()
