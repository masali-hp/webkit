set(DLLLauncher_SOURCES
    DLLLauncherMain.cpp
)

set(DLLLauncher_LIBRARIES
    shlwapi
)

add_definitions(-DUSE_CONSOLE_ENTRY_POINT=1)

add_executable(DumpRenderTreeLauncher ${DLLLauncher_SOURCES})
target_link_libraries(DumpRenderTreeLauncher ${DLLLauncher_LIBRARIES})
set_target_properties(DumpRenderTreeLauncher PROPERTIES FOLDER "Tools")
set_target_properties(DumpRenderTreeLauncher PROPERTIES OUTPUT_NAME "DumpRenderTree")

add_executable(ImageDiffLauncher ${DLLLauncher_SOURCES})
target_link_libraries(ImageDiffLauncher ${DLLLauncher_LIBRARIES})
set_target_properties(ImageDiffLauncher PROPERTIES FOLDER "Tools")
set_target_properties(ImageDiffLauncher PROPERTIES OUTPUT_NAME "ImageDiff")
