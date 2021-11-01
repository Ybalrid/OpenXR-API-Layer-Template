# OpenXR API Layer template

<!--
SPDX-License-Identifier: MIT
SPDX-FileCopyrightText: 2021 Arthur Brainville (Ybalrid) <ybalrid@ybalrid.info>
-->

This template project allows you to easily create [OpenXR API layers](https://www.khronos.org/registry/OpenXR/specs/1.0/html/xrspec.html#api-layers).

OpenXR layers are runtime libraries that allow you to shim code between an OpenXR application and an OpenXR runtime.

They can be used for validation of the API or for implementing additional OpenXR extensions.

## Configuration

This project is CMake-based. The configuration for the layer manifest, including extension declaration, happens in the top-level CMakeList.txt file.

In the make CMakeLists.txt file at the root of the repository, you should edit the following lines:

```cmake
project(OpenXR-layer-template)
set(library_name "layer_module")
set(layer_name "XR_APILAYER_test_me")
set(layer_version "1")
set(layer_description "some text here")
```

These values are used to configure both the source code and the manifest file for distribution and loading.

Please refer to the comments in the file for more information. For the declaration of OpenXR extensions that you wish to implement, 
please refer to the following sections of this readme file.

## Building

Simply run CMake, or use CMake GUI. I recommend creating a `build` subfolder.

```bash
mkdir build
cd build
cmake ..
```

## OpenXR function shim

Refer to the example implementation of intercepting calls to `xrEndFrame` in `layer_shims.cpp`.

The two main things are 

1. Define a function that has the same signature as the OpenXR function it should implement
1. If the function is a shim of an OpenXR runtime function, you will need to call the original function

Here's an example for xrEndFrame, that will print in the console the display time of every single frame:

```cpp
XRAPI_ATTR XrResult XRAPI_CALL thisLayer_xrEndFrame(XrSession session, const XrFrameEndInfo* frameEndInfo)
{
	//First time this runs, it will fetch the pointer from the loaded OpenXR dispatch table
	static PFN_xrEndFrame nextLayer_xrEndFrame = GetNextLayerFunction(xrEndFrame);

	std::cout << "Display frame time is " << frameEndInfo->displayTime << "\n";

	const auto result = nextLayer_xrEndFrame(session, frameEndInfo);
	if(result == XR_ERROR_TIME_INVALID)
		std::cout << "frame time is invalid?\n";

	return result;
}
```

The initialization process stores the function pointer for the next layer (or the instance we actually can't know and should act transparently).
The `GetNextLayerFunction(xrFunctionName)` macro is a helper to allow you to retrive it from the `OpenXRLayer` instance.

To avoid clashing with the function declared in `openxr.h`, we prefix these functions with `thisLayer_`.

Each function must be registered. To do so, you should add a line similar to this one in the `ListShims()` function at the end `layer_shims.cpp`.

```cpp
functions.emplace_back("xrEndFrame", PFN_xrVoidFunction(thisLayer_xrEndFrame));
```

## Layer implemented extensions

API Layers *can* implement OpenXR instance extensions.

This template project allows you to configure the output layer for this task too.

In the main CMakeLists.txt, you'll find these commented-out declarations:

```cmake
#To add extension declaration to the layer manifest file, fill these two string lists : "extensions" and "extension_versions"
#These two needs to match in lenght
list(APPEND extensions "XR_TEST_test_me")  # <---
list(APPEND extension_versions "42")       # <---
```

Uncomment the `list(APPEND ...)` calls, and replace them with the name and version of the extension.

This will do three things:

1. It will add the list of extensions into the layer manifest file
2. It will add the list of extensions supported by the layer in `layer_config.hpp`.
3. It will activate code in the layer initialization function to create an instance that adds listed extensions in their `enabledExtensionNames` array.

If the extension in question requires adding new OpenXR functions, they need to be added to the shim dispatch table. However, since these functions presumably don't exist in the runtime/next layer (as you are the implementation point), you don't need to find a function pointer to call back to.

For example, this template implements a bogus extension called `XR_TEST_test_me`; this function's only purpose is to add the function `xrTestMeTEST`. This function only prints a message to the standard output.

To implement this extension, you need to uncomment its declaration in the main CMake file, then uncomment in `layer_shims.cpp` the following function :

```cpp
XRAPI_ATTR XrResult XRAPI_CALL thisLayer_xrTestMeTEST(XrSession session)
{
	(void)session;
	std::cout << "xrTestMe()\n";
	return XR_SUCCESS;
}
```

In `ListShims()`, you need to check if the client application has requested the usage of the extension, and if it has, you should register the new function in the same way a function shim should be:

```cpp
if (OpenXRLayer::IsExtensionEnabled("XR_TEST_test_me"))
	functions.emplace_back("xrTestMeTEST", PFN_xrVoidFunction(thisLayer_xrTestMeTEST));
```

## Internal API reference

This is the internal API that you are expected to access inside `layer_shims.cpp`:

### Macros

```cpp
PFN_xrFunctionName GetNextLayerFunction(xrFunctionName);
```

Return the pointer to `xrFunctionName`, with the correct function pointer type.

If the function is not implemented in the next layer or the runtime, this will return `nullptr`.
___

```cpp
XR_THISLAYER_NAME
```

String literal that is the configured layer name.
___
```cpp
XR_THISLAYER_HAS_EXTENSIONS
```

`bool` constant. If `true`, it means that this layer implements extension commands.

### Globals

```cpp
static const char * layer_extension_names[] { /*...*/ };
```

A global constant array of the extension names that are implemented by this layer (if any)

Only declard if `XR_THISLAYER_HAS_EXTENSIONS` is `true`.
___
```cpp
static bool OpenXRLayer::IsExtensionEnabled(const char* extensionName)
```
Returns `true` if the client application enables the extension in question.
___

```cpp
OpenXRLayer::ShimFunction(const char * functionName, PFN_xrVoidFunction thisLayer_functionPointer);
```
The public constructor of `OpenXRLayer::ShimFunction`. This structure represents one OpenXR function that is implemented inside this layer (and may layer on top of another function)

## Initialization

The whole Layer initialization is done inside `layer_bootstrap.cpp`.

The following steps occur:

1. OpenXR loader finds this layer by *some means*, parse the manifest file, find the library and load it
2. `xrNegotiateLoaderApiLayerInterface()` is called by the loader. A number of checks are performed, if they are all successful, the loader now has access to `thisLayer_xrGetInstanceProcAddr()` and `thisLayer_xrCreateApiLayerInstance()`
3. When the client wants to create an XrInstance, `thisLayer_xrCreateApiLayerInstane` is called. We have access to the enabledExtensionList from the `XrInstanceCreateInfo()`
4. If the layer *implements an extension*, and if the client enables this extension, we keep this information internally, and we *remove* this extension from the list that will be passed down the layer chain.
5. We call the `LoadShims()` function. This initializes the list of function pointers of all shim functions in this layer.
6. We call the next layer's `xrCreateApiLayerInstance` function with the modified parameters
7. We load all the function pointers from all the shimmed functions by this layer from the *next* layer in the chain to call them.
8. The loader, or the client application will call `xrGetInstanceProcAddr()`, this will call our layer's `thisLayer_xrGetInstanceProcAddr()`. This is they will obtain a pointer to either our implementation of an OpenXR function. If we don't implement the function in question, the next layer down the chain will be called.

## Usage

Once the layer is built, it will be located in the `<build>/layer_name` directory, alongside its Json Manifest.

Many different configurations may result in an OpenXR application loading and using an API layer. Please refer to the OpenXR *Loader* specification (not the core specification) for more information.
They are platform-dependant. On Windows, they involve setting registry keys. On Linux, they involve symlinking the .json manifest files to specific places in the filesystem.

A simple way to get a Loader implicitly enabled by an openxr application is to specify a number of environment variables when starting the client program.

```
XR_API_LAYER_PATH=path_to_the_layer_output_folder/XR_APILAYER_test_me
XR_ENABLE_API_LAYERS=XR_APILAYER_test_me
```

You can also specify `XR_LOADER_DEBUG=all` to cause the loader to output all debug messages it can to standard output. This is useful for debugging layer loading issues.

## Legal

Copyright (C) 2021 Artur Brainville, distributed under the MIT license agreement. (REUSE 3.0 compliant.) 

This is built on top of the OpenXR SDK. The OpenXR SDK is distributed by the Khronos group under the terms of the Apache 2.0 License. 
This template project does not link against any binaries from the OpenXR SDK but includes header files generated from the OpenXR registry and the standard OpenXR loader project.

*OpenXR™ is a trademark owned by The Khronos Group Inc. and is registered as a trademark in China, the European Union, Japan, and the United Kingdom.*
