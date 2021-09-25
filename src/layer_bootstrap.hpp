// SPDX-FileCopyrightText: 2021 Arthur Brainville (Ybalrid) <ybalrid@ybalrid.info>
//
// SPDX-License-Identifier: MIT
//
// Initial Author: Arthur Brainville <ybalrid@ybalrid.info>

#pragma once

#ifdef _WIN32
#define XR_USE_PLATFORM_WIN32
#define XR_OS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

#include "openxr/openxr_platform_defines.h"
#include "openxr/openxr.h"
#include "api_layer_interface.hpp"
#include "layer.hpp"

#ifndef LAYER_EXPORT
#if defined(__GNUC__) && __GNUC__ >= 4
#define LAYER_EXPORT __attribute__((visibility("default")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590)
#define LAYER_EXPORT __attribute__((visibility("default")))
#elif defined(_WIN32)
#define LAYER_EXPORT __declspec(dllexport)
#else
#define LAYER_EXPORT
#endif
#endif

extern "C" XrResult LAYER_EXPORT XRAPI_CALL xrNegotiateLoaderApiLayerInterface(
	const XrNegotiateLoaderInfo* loaderInfo,
	const char* apiLayerName,
	XrNegotiateApiLayerRequest* apiLayerRequest
);

XRAPI_ATTR XrResult XRAPI_CALL thisLayer_xrCreateApiLayerInstance(const XrInstanceCreateInfo* info,
	const struct XrApiLayerCreateInfo* apiLayerInfo,
	XrInstance* instance);

XRAPI_ATTR XrResult XRAPI_CALL thisLayer_xrGetInstanceProcAddr(XrInstance instance,
	const char* name,
	PFN_xrVoidFunction* function);


