// SPDX-FileCopyrightText: 2021 Arthur Brainville (Ybalrid) <ybalrid@ybalrid.info>
//
// SPDX-License-Identifier: MIT
//
// Initial Author: Arthur Brainville <ybalrid@ybalrid.info>

#pragma once
#ifdef _MSC_VER
#pragma warning(disable : 26812) //OpenXR is a C API that doesn't use `enum class`
#endif

#include "openxr/openxr.h"
#include <string>
#include <vector>
#include <unordered_map>

class OpenXRLayer
{
public:
	struct ShimFunction
	{
		ShimFunction(const char* functionName, PFN_xrVoidFunction thisLayer_xrFunction) :
			functionName{ functionName },
			thisLayer_xrFunction{ thisLayer_xrFunction }
		{}

		ShimFunction() = default;

		std::string functionName{};
		PFN_xrVoidFunction thisLayer_xrFunction = nullptr;
		PFN_xrVoidFunction nextLayer_xrFunction = nullptr;
	};

	OpenXRLayer(const OpenXRLayer&) = delete;
	OpenXRLayer(OpenXRLayer&&) = delete;
	OpenXRLayer& operator=(const OpenXRLayer&) = delete;
	OpenXRLayer& operator=(OpenXRLayer&&) = delete;
	~OpenXRLayer();

	static void CreateLayerContext(PFN_xrGetInstanceProcAddr getInstanceProcAddr, const std::vector<ShimFunction>& shims = {});

	static OpenXRLayer& GetLayerContext();
	XrResult GetInstanceProcAddr(XrInstance instance, const char* name, PFN_xrVoidFunction* function);
	void LoadDispatchTable(XrInstance instance);
	PFN_xrVoidFunction GetNextLayer(const std::string& function);
	static void SetEnabledExtensions(const std::vector<const char*>& extensionList);
	static bool IsExtensionEnabled(const char* extensionName);
	static bool IsValid() { return this_layer != nullptr; }

private:
	explicit OpenXRLayer(PFN_xrGetInstanceProcAddr nextLayerGetInstanceProcAddr);
	static OpenXRLayer* this_layer;

	PFN_xrGetInstanceProcAddr nextLayer_xrGetInstanceProcAddr;
	static std::vector<const char*> extensions;
	std::unordered_map<std::string, ShimFunction> functions;
};

#define GetPFN(name) PFN_##name
#define GetNextLayerFunction(name)\
	reinterpret_cast<GetPFN(name)>(OpenXRLayer::IsValid() ? OpenXRLayer::GetLayerContext().GetNextLayer(#name) : nullptr)
