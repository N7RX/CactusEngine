# Cactus Engine
A tiny ECS rendering engine based on Vulkan and OpenGL. 
*(You will feel like grabbing a cactus when using this engine.)*<br/>
<br/>
<img src="/README_pix/Screenshot_0.png" width="640" height="360">
<br/><br/>
This project is working in progress.<br/><br/>Completed parts include:

- Fundamental ECS
- Multi-pass forward rendering (OpenGL)
- Multi-pass forward rendering (Vulkan)
- Multi-threaded render command recording (Vulkan)
- Heterogeneous-GPU rendering (Vulkan)
- Render Graph Prototype (Vulkan)
- Scene saving and reading with JSON file

Currently working on:

* Render Graph (Vulkan)
* ImGui support under Vulkan

<br/>

#### Dependencies

- Vulkan 1.2
- OpenGL 4.3
- GLFW, GLAD
- GLM
- Dear ImGui
- Assimp 5.0
- Eigen
- stb_image
- jsoncpp
- VMA
- SPIRV-Cross



#### Sample

<img src="/README_pix/Screenshot_1.png" width="640" height="360">

<img src="/README_pix/Screenshot_2.png" width="640" height="360">

<img src="/README_pix/Screenshot_3.png" width="640" height="360">

Model Copyright:

- Crytek Sponza & Serapis Bust from Morgan McGuire's [Computer Graphics Archive](https://casual-effects.com/data)
- Unity-Chan from [Unity-Chan Official Website](https://unity-chan.com/)
- Lucy from [Stanford 3D Scanning Repository](http://graphics.stanford.edu/data/3Dscanrep/)