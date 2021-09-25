// SPDX-FileCopyrightText: 2021 Arthur Brainville (Ybalrid) <ybalrid@ybalrid.info>
//
// SPDX-License-Identifier: MIT
//
// Initial Author: Arthur Brainville <ybalrid@ybalrid.info>

#include "layer_bootstrap.hpp"
#include "layer_shims.hpp"
#include "layer_config.hpp"
#include <cstring>

extern "C" XrResult LAYER_EXPORT XRAPI_CALL xrNegotiateLoaderApiLayerInterface(const XrNegotiateLoaderInfo * loaderInfo, const char* apiLayerName,
	XrNegotiateApiLayerRequest* apiLayerRequest)
{
	if (nullptr == loaderInfo || nullptr == apiLayerRequest || loaderInfo->structType != XR_LOADER_INTERFACE_STRUCT_LOADER_INFO ||
		loaderInfo->structVersion != XR_LOADER_INFO_STRUCT_VERSION || loaderInfo->structSize != sizeof(XrNegotiateLoaderInfo) ||
		apiLayerRequest->structType != XR_LOADER_INTERFACE_STRUCT_API_LAYER_REQUEST ||
		apiLayerRequest->structVersion != XR_API_LAYER_INFO_STRUCT_VERSION ||
		apiLayerRequest->structSize != sizeof(XrNegotiateApiLayerRequest) ||
		loaderInfo->minInterfaceVersion > XR_CURRENT_LOADER_API_LAYER_VERSION ||
		loaderInfo->maxInterfaceVersion < XR_CURRENT_LOADER_API_LAYER_VERSION ||
		loaderInfo->maxInterfaceVersion > XR_CURRENT_LOADER_API_LAYER_VERSION ||
		loaderInfo->maxApiVersion < XR_CURRENT_API_VERSION ||
		loaderInfo->minApiVersion > XR_CURRENT_API_VERSION ||
		0 != strcmp(apiLayerName, XR_THISLAYER_NAME)) {
		return XR_ERROR_INITIALIZATION_FAILED;
	}

	apiLayerRequest->layerInterfaceVersion = XR_CURRENT_LOADER_API_LAYER_VERSION;
	apiLayerRequest->layerApiVersion = XR_CURRENT_API_VERSION;
	apiLayerRequest->getInstanceProcAddr = reinterpret_cast<PFN_xrGetInstanceProcAddr>(thisLayer_xrGetInstanceProcAddr);
	apiLayerRequest->createApiLayerInstance = reinterpret_cast<PFN_xrCreateApiLayerInstance>(thisLayer_xrCreateApiLayerInstance);

	return XR_SUCCESS;
}

XrResult thisLayer_xrCreateApiLayerInstance(const XrInstanceCreateInfo* info, const XrApiLayerCreateInfo* apiLayerInfo,
	XrInstance* instance)
{
	if (nullptr == apiLayerInfo || XR_LOADER_INTERFACE_STRUCT_API_LAYER_CREATE_INFO != apiLayerInfo->structType ||
		XR_API_LAYER_CREATE_INFO_STRUCT_VERSION > apiLayerInfo->structVersion ||
		sizeof(XrApiLayerCreateInfo) > apiLayerInfo->structSize || nullptr == apiLayerInfo->nextInfo ||
		XR_LOADER_INTERFACE_STRUCT_API_LAYER_NEXT_INFO != apiLayerInfo->nextInfo->structType ||
		XR_API_LAYER_NEXT_INFO_STRUCT_VERSION > apiLayerInfo->nextInfo->structVersion ||
		sizeof(XrApiLayerNextInfo) > apiLayerInfo->nextInfo->structSize ||
		0 != strcmp(XR_THISLAYER_NAME, apiLayerInfo->nextInfo->layerName) ||
		nullptr == apiLayerInfo->nextInfo->nextGetInstanceProcAddr ||
		nullptr == apiLayerInfo->nextInfo->nextCreateApiLayerInstance)
	{
		return XR_ERROR_INITIALIZATION_FAILED;
	}

	//Prepare to call this function down the layer chain
	XrApiLayerCreateInfo newApiLayerCreateInfo;
	memcpy(&newApiLayerCreateInfo, &apiLayerInfo, sizeof(newApiLayerCreateInfo));
	newApiLayerCreateInfo.nextInfo = apiLayerInfo->nextInfo->next;

	XrInstanceCreateInfo instanceCreateInfo = *info;
	std::vector<const char*> extension_list_without_implemented_extensions;
	std::vector<const char*> enabled_this_layer_extensions;

	//If we deal with extensions, we will check the list of enabled extensions.
	//We remove ours form the list if present, and we store the list of *our* extensions that were enabled
	#if XR_THISLAYER_HAS_EXTENSIONS
	{
		for (size_t enabled_extension_index = 0; enabled_extension_index < instanceCreateInfo.enabledExtensionCount; ++enabled_extension_index)
		{
			const char* enabled_extension_name = instanceCreateInfo.enabledExtensionNames[enabled_extension_index];
			bool implemented_by_us = false;

			for (const auto layer_extension_name : layer_extension_names)
			{
				if(0 == strcmp(enabled_extension_name, layer_extension_name))
				{
					implemented_by_us = true;
					break;
				}
			}

			if(implemented_by_us)
				enabled_this_layer_extensions.push_back(enabled_extension_name);
			else
				extension_list_without_implemented_extensions.push_back(enabled_extension_name);
		}

		instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extension_list_without_implemented_extensions.size());
		instanceCreateInfo.enabledExtensionNames = extension_list_without_implemented_extensions.data();
		OpenXRLayer::SetEnabledExtensions(enabled_this_layer_extensions);
	}
#endif


	//This is the real "bootstrap" of this layer's
	OpenXRLayer::CreateLayerContext(apiLayerInfo->nextInfo->nextGetInstanceProcAddr, ListShims());

	XrInstance newInstance = *instance;
	const auto result = apiLayerInfo->nextInfo->nextCreateApiLayerInstance(&instanceCreateInfo, &newApiLayerCreateInfo, &newInstance);
	if (XR_FAILED(result))
	{
		return XR_ERROR_LAYER_INVALID;
	}

	OpenXRLayer::GetLayerContext().LoadDispatchTable(newInstance);

	*instance = newInstance;
	return result;
}

XrResult thisLayer_xrGetInstanceProcAddr(XrInstance instance, const char* name, PFN_xrVoidFunction* function)
{
	return OpenXRLayer::GetLayerContext().GetInstanceProcAddr(instance, name, function);
}
