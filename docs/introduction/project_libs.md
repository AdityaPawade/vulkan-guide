---
layout: default
title: Project layout and libraries
parent: introduction
nav_order: 99
---

## Project layout

For the Vulkan engine, we use a specific folder layout.

- `/assets` will contain textures and 3d models that we use over the guide
- `/bin` is where the executables will get built. We don't use the normal CMake build folders to simplify the paths to assets and shaders
- `/shaders` will contain all our shaders, and their compiled output
- `/chapter-N` folders contain the code for each of the chapters of the guide
- `/third_party` contains all of the libraries we use, vendored in, so all of the source code is readily available

## Libraries

On the engine, we use a set of libraries, most stored in `/third_party`. The only library that isn't vendored-in is SDL

All the libraries in third-party are vendored-in, and CMake will build them automatically. For SDL, it's required to build it separately from the project, and tell CMake where to find it.

The Libraries we are using:
- [GLM (openGL Mathematics)](https://github.com/g-truc/glm) Mathematics library, Header only. We use this for its matrix and vector math functionality. It's a library that contains types that are directly compatible with GLSL types in most cases. For example, a `glm::mat4` has similar operations and is directly compatible with a `mat4` in a shader. While it says OpenGL in the name, it works great with Vulkan.

- [SDL](https://www.libsdl.org/) Windowing and input library, Separately built. SDL is one of the most used libraries to create a window and access input devices in a crossplatform way. SDL works on almost every platform, and is very well supported. Used in Unreal Engine, Unity, Source, and others. We use it in the project to have a easy and fast way to open a window, and have detailed keyboard input.

- [dear IMGUI](https://github.com/ocornut/imgui) Easy to use immediate Graphical-User-Interface (GUI) library. This allows us to create editable widgets such as sliders and windows for the user interface. It's widely used in the game industry for debug tools. In the project, we use it to create interactive options for rendering.

- [STB Image](https://github.com/nothings/stb) Image loading library, header only. Small and easy to use library to load image files. It can load common image formats such as BMP, PNG, JPEG, and others. It is part of a set of other similar single-file libraries.

- [Tiny Obj Loader](https://github.com/tinyobjloader/tinyobjloader) .Obj model loader library, header only. Fast and small library to load the .obj 3d model format that we will use when loading 3d models. 

- [Vk Bootstrap](https://github.com/charles-lunarg/vk-bootstrap/blob/master/src/VkBootstrap.cpp) Abstracts a big amount of boilerplate that Vulkan has when setting up. Most of that code is written once and never touched again, so we will skip most of it using this library. This library simplifies instance creation, swapchain creation, and extension loading. It will be removed from the project eventually in an optional chapter that explains how to initialize that Vulkan boilerplate the "manual" way.

- [VMA (vulkan memory allocator)](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) Implements memory allocators for Vulkan, header only. In Vulkan, the user has to deal with the memory allocation of buffers, images, and other resources on their own. This can be very difficult to get right in a performant and safe way. Vulkan Memory Allocator does it for us and allows us to simplify the creation of images and other resources. Widely used in personal Vulkan engines or smaller scale projects like emulators. Very high end projects like Unreal Engine or AAA engines write their own memory allocators.

Next: [Building Project]({{ site.baseurl }}{% link docs/new_chapter_0/building_project.md %})


{% include comments.html term="Introduction Comments" %}
