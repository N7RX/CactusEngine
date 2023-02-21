# Cactus Engine
*(It will feel like hugging a cactus when using this engine.)*<br/><br/>A tiny game engine built for my small-scale personal projects. As of now it is still only a rendering engine and is WIP.
<br/>
<img src="/README_pix/Screenshot_0.png" width="640" height="360">
<br/>
<br/>

#### Features

Engine features:

- OpenGL and Vulkan support
- Entity-Component-System structure
- Deferred shading
- Multi-thread rendering (Vulkan only)
- Scene saving and reading
- ImGUI support (OpenGL only)
- Basic render graph

Work in progress:

* ImGUI support for Vulkan
* Vulkan pipeline caching
* Vulkan sub-pass support
* Improved render graph
* Compute pipeline support and GPU particles

Planned features:

* Frustum culling
* PBR render graph
* Audio support (possibly through OpenAL)
* Enhanced GUI

Ideas under consideration, may not put into work:

* Physics system support (possibly through Bullet Physics)
* Texture compression
* LOD system
* SPIR-V optimization
* Bone animation system
* Vulkan ray tracing
* Video decoding support

<br/>

Deprecated feature(s):

- Heterogeneous-GPU rendering (Vulkan only). Available in `heterogeneous` branch.

<br/>

#### Dependencies

- [GLFW (Window/Input Utility)](https://github.com/glfw/glfw)
- [GLM (Graphics Mathematics)](https://github.com/g-truc/glm)
- [Dear ImGui (GUI Utility)](https://github.com/ocornut/imgui)
- [Assimp (Asset Import)](https://github.com/assimp/assimp)
- [stb (Image Utility)](https://github.com/nothings/stb)
- [JsonCpp (JSON parser)](https://github.com/open-source-parsers/jsoncpp)
- [Vulkan SDK 1.2+](https://www.lunarg.com/vulkan-sdk/)
- [Volk (Vulkan Loader)](https://github.com/zeux/volk)
- [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
- [SPIRV-Cross (SPIR-V Utility)](https://github.com/KhronosGroup/SPIRV-Cross)
- OpenGL 4.6
- [GLAD (OpenGL Loader)](https://github.com/Dav1dde/glad/tree/master)
- C++ 17
- Visual Studio 2022

<br/>

#### Samples

(Sample screenshots taken from `legacy-2020` branch)

<br/>

<img src="/README_pix/Screenshot_1.png" width="640" height="360">

<img src="/README_pix/Screenshot_2.png" width="640" height="360">

<img src="/README_pix/Screenshot_3.png" width="640" height="360">

3D Models Source:

- Crytek Sponza & Serapis Bust from [Morgan McGuire's Computer Graphics Archive](https://casual-effects.com/data)
- Unity-Chan from [Unity-Chan Official Website](https://unity-chan.com/)
- Lucy from [Stanford 3D Scanning Repository](http://graphics.stanford.edu/data/3Dscanrep/)