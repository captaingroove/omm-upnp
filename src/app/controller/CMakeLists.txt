if(LIB_OMMGUI_PLATFORM)
add_executable(ommcontroller WIN32
ControllerApplication.cpp
)

target_link_libraries(ommcontroller
${OMM}
${OMM_UTIL}
${OMM_AV}
${OMM_GUI}
${OMM_UPNPGUI}
${OMMX_ENGINE_VLC}
)

install(TARGETS ommcontroller
RUNTIME DESTINATION bin
LIBRARY DESTINATION lib
)
endif(LIB_OMMGUI_PLATFORM)