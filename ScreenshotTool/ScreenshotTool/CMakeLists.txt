# CMakeList.txt : CMake project for ScreenshotTool, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (ScreenshotTool "ScreenshotTool.cpp" "ScreenshotTool.h")

target_link_libraries(ScreenshotTool
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ScreenshotTool PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
