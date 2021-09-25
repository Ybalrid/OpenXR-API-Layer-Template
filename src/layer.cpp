// SPDX-FileCopyrightText: 2021 Arthur Brainville (Ybalrid) <ybalrid@ybalrid.info>
//
// SPDX-License-Identifier: MIT
//
// Initial Author: Arthur Brainville <ybalrid@ybalrid.info>

#include "layer.hpp"
#include "layer_config.hpp"
#include <stdexcept>

OpenXRLayer* OpenXRLayer::this_layer = nullptr;
std::vector<const char*> OpenXRLayer::extensions{};

void OpenXRLayer::SetEnabledExtensions(const std::vector<const char*>& extensionList)
{
	extensions = extensionList;
}

bool OpenXRLayer::IsExtensionEnabled(const char* extensionName)
{
	const auto iterator = std::find_if(extensions.begin(),
		extensions.end(),
		[extensionName](const char* enabled_extension)
		{
			return 0 == strcmp(extensionName, enabled_extension);
		});

	return iterator != extensions.end();
}

OpenXRLayer::OpenXRLayer(PFN_xrGetInstanceProcAddr nextLayerGetInstanceProcAddr) :
	nextLayer_xrGetInstanceProcAddr{ nextLayerGetInstanceProcAddr }
{
	if (this_layer)
		throw std::runtime_error("Initialization of OpenXR layer " XR_THISLAYER_NAME " failed: Cannot create more than one instance of OpenXRLayer class.");

	this_layer = this;
}

OpenXRLayer::~OpenXRLayer()
{
	this_layer = nullptr;
}

void OpenXRLayer::CreateLayerContext(PFN_xrGetInstanceProcAddr getInstanceProcAddr,
	const std::vector<ShimFunction>& shims)
{
	(void)shims;
	new OpenXRLayer(getInstanceProcAddr);

	for (auto shim : shims)
		this_layer->functions[shim.functionName] = shim;
}

OpenXRLayer& OpenXRLayer::GetLayerContext()
{
	if (this_layer)
		return *this_layer;

	throw std::runtime_error("Layer uninitialized. Cannot dereference singleton");
}

XrResult OpenXRLayer::GetInstanceProcAddr(XrInstance instance,
	const char* name,
	PFN_xrVoidFunction* function)
{
	if (!nextLayer_xrGetInstanceProcAddr)
		throw std::runtime_error("Somehow we don't have a pointer to the next layer in the chain's GetInstanceProcAddr function.");

	const auto function_from_table = functions.find(name);
	if (function_from_table != functions.end())
	{
		*function = function_from_table->second.thisLayer_xrFunction;
		return XR_SUCCESS;
	}

	return nextLayer_xrGetInstanceProcAddr(instance, name, function);
}

void OpenXRLayer::LoadDispatchTable(XrInstance instance)
{
	PFN_xrVoidFunction functionPointer = nullptr;
	for (auto& [functionName, shim] : functions)
		if (XR_SUCCEEDED(nextLayer_xrGetInstanceProcAddr(instance, functionName.c_str(), &functionPointer)))
			shim.nextLayer_xrFunction = functionPointer;
}

PFN_xrVoidFunction OpenXRLayer::GetNextLayer(const std::string& function)
{
	return functions.at(function).nextLayer_xrFunction;
}
