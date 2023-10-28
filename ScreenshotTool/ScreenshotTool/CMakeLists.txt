add_executable (ScreenshotTool "ScreenshotTool.cpp" "ScreenshotTool.h")

target_link_libraries(ScreenshotTool
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ScreenshotTool PROPERTY CXX_STANDARD 20)
endif()
