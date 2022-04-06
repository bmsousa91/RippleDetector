#plugin build file
cmake_minimum_required(VERSION 3.5.0)

#include common rules
include(../PluginRules.cmake)

#add sources, not including OpenEphysLib.cpp
add_sources(${PLUGIN_NAME}
	RippleDetector.cpp
	RippleDetector.h
	RippleDetectorEditor.cpp
	RippleDetectorEditor.h
	)
	
#optional: create IDE groups
#plugin_create_filters()