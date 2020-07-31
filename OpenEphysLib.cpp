#include <PluginInfo.h>
#include <string>
#include "RippleDetector.h"

#ifdef WIN32
#include <Windows.h>
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif

extern "C" EXPORT void getLibInfo(Plugin::LibraryInfo *info)
{
	info->apiVersion = PLUGIN_API_VER;
	info->name = "Ripple Detector";
	info->libVersion = 1;
	info->numPlugins = 1;
}

extern "C" EXPORT int getPluginInfo(int index, Plugin::PluginInfo *info)
{
	switch (index)
	{
	case 0:
		info->type = Plugin::PLUGIN_TYPE_PROCESSOR;
		info->processor.name = "Ripple Detector";
		info->processor.type = Plugin::FilterProcessor;
		info->processor.creator = &(Plugin::createProcessor<RippleDetector>);
		break;
	default:
		return -1;
		break;
	}
	return 0;
}

#ifdef WIN32
BOOL WINAPI DllMain(IN HINSTANCE hDllHandle,
					IN DWORD nReason,
					IN LPVOID Reserved)
{
	return TRUE;
}

#endif
