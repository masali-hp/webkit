if (WINOS MATCHES XP)
    list(APPEND JSC_LIBRARIES winmm)
elseif (WINOS MATCHES CE)
    set(JSC_LINK_FLAGS "${JSC_LINK_FLAGS} /STACK:524888,524888")

if (${PLATFORM} MATCHES "HP")
	# When building for CE, we need the webkit libraries to come before
	# the system default libraries.  To do this we add system default libs
	# as webkit libs and zero out system default libs.
	STRING(REPLACE " " "\;" CXX_LIBS ${CMAKE_CXX_STANDARD_LIBRARIES})
	# wtf.lib is an inherited dependency for webkit that needs to come before
	# system libraries for ARM builds when we override new/delete.
	list(APPEND JSC_LIBRARIES ${WTF_LIBRARY_NAME})
	list(APPEND JSC_LIBRARIES ${CXX_LIBS})
	set(CMAKE_CXX_STANDARD_LIBRARIES "")
endif ()
endif ()
