# Cactus Engine
*(It will feel like hugging a cactus when using this engine.)*<br/><br/>A tiny game engine built for small-scale personal projects, targeting desktop platform. As of now it is still only a rendering engine and is WIP.
<br/><br/><img src="/README_pix/Screenshot_0.png" width="640" height="360">
<br/>
<br/>

#### Features

Engine features:

- Entity-Component-System structure
- OpenGL and Vulkan support
- Multi-thread rendering
- Deferred shading
- Render graph
- Scene saving and reading
- ImGUI support

Working in progress:

* ImGUI support for Vulkan
* PBR render graph
* (and more...)

<br/>

Deprecated feature(s):

- Heterogeneous-GPU rendering (Vulkan only). Available in `heterogeneous` branch.

<br/>

#### Dependencies

- [GLFW (window/input utility)](https://github.com/glfw/glfw)
- [GLM (graphics mathematics)](https://github.com/g-truc/glm)
- [Dear ImGui (GUI utility)](https://github.com/ocornut/imgui)
- [Assimp (asset import)](https://github.com/assimp/assimp)
- [stb (image utility)](https://github.com/nothings/stb)
- [JsonCpp (JSON parser)](https://github.com/open-source-parsers/jsoncpp)
- [Vulkan SDK 1.2+](https://www.lunarg.com/vulkan-sdk/)
- [Volk (Vulkan loader)](https://github.com/zeux/volk)
- [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
- [SPIRV-Cross (SPIR-V utility)](https://github.com/KhronosGroup/SPIRV-Cross)
- OpenGL 4.6
- [GLAD (OpenGL loader)](https://github.com/Dav1dde/glad/tree/master)
- C++ 17
- Visual Studio 2022

<br/>

#### Samples

<br/>

<img src="/README_pix/Screenshot_1.png" width="640" height="360">

<img src="/README_pix/Screenshot_2.png" width="640" height="360">

<img src="/README_pix/Screenshot_3.png" width="640" height="360">

Sample model sources:

- Crytek Sponza & Serapis Bust from [Morgan McGuire's Computer Graphics Archive](https://casual-effects.com/data)
- Unity-Chan from [Unity-Chan Official Website](https://unity-chan.com/)
- Lucy from [Stanford 3D Scanning Repository](http://graphics.stanford.edu/data/3Dscanrep/)