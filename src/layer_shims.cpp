// SPDX-FileCopyrightText: 2021-2023 Arthur Brainville (Ybalrid) <ybalrid@ybalrid.info>
//
// SPDX-License-Identifier: MIT
//
// Initial Author: Arthur Brainville <ybalrid@ybalrid.info>

#include "layer_shims.hpp"

#include <cassert>
#include <iostream>

//IMPORTANT: to allow for multiple instance creation/destruction, the contect of the layer must be re-initialized when the instance is being destroyed.
//Hooking xrDestroyInstance is the best way to do that.
XRAPI_ATTR XrResult XRAPI_CALL thisLayer_xrDestroyInstance(
	XrInstance instance)
{
	PFN_xrDestroyInstance nextLayer_xrDestroyInstance = GetNextLayerFunction(xrDestroyInstance);

	OpenXRLayer::DestroyLayerContext();

	assert(nextLayer_xrDestroyInstance != nullptr);
	return nextLayer_xrDestroyInstance(instance);
}



//Define the functions implemented in this layer like this:
XRAPI_ATTR XrResult XRAPI_CALL thisLayer_xrEndFrame(XrSession session,
	const XrFrameEndInfo* frameEndInfo)
{
	//First time this runs, it will fetch the pointer from the loaded OpenXR dispatch table
	static PFN_xrEndFrame nextLayer_xrEndFrame = GetNextLayerFunction(xrEndFrame);

	//Do some additional things;
	std::cout << "Display frame time is " << frameEndInfo->displayTime << "\n";

	//Call down the chain
	const auto result = nextLayer_xrEndFrame(session, frameEndInfo);

	//Maybe do something with the original return value?
	if(result == XR_ERROR_TIME_INVALID)
		std::cout << "frame time is invalid?\n";

	//Return what should be returned as if you were the actual function
	return result;
}

#if XR_THISLAYER_HAS_EXTENSIONS
//The following function doesn't exist in the spec, this is just a test for the extension mecanism
XRAPI_ATTR XrResult XRAPI_CALL thisLayer_xrTestMeTEST(XrSession session)
{
	(void)session;
	std::cout << "xrTestMe()\n";
	return XR_SUCCESS;
}
#endif

//This functions returns the list of function pointers and name we implement, and is called during the initialization of the layer:
std::vector<OpenXRLayer::ShimFunction> ListShims()
{
	std::vector<OpenXRLayer::ShimFunction> functions;
	functions.emplace_back("xrDestroyInstance", PFN_xrVoidFunction(thisLayer_xrDestroyInstance));

	//List every functions that is callable on this API layer
	functions.emplace_back("xrEndFrame", PFN_xrVoidFunction(thisLayer_xrEndFrame));

#if XR_THISLAYER_HAS_EXTENSIONS
	if (OpenXRLayer::IsExtensionEnabled("XR_TEST_test_me"))
		functions.emplace_back("xrTestMeTEST", PFN_xrVoidFunction(thisLayer_xrTestMeTEST));
#endif

	return functions;
}
