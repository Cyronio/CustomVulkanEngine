# Slime Rancher

Slime Rancher is a short and basic demo game to showcase a custom game engine built on top of an adapted version of the original Vulkan Tutorial. <br>
It uses https://github.com/hlavacs/AdaptedVulkanTutorial. Please see below for build instructions and the respective folder for the license. <br>
In the demo game you control a little alien slime that lives in the Viking Room and tries to protect the forest against black-and-whiteness. Light up the magical spheres in time before everything loses its color. <br>
***Engine specs*** <br>

<img src="https://github.com/Cyronio/CubeShooter/blob/master/SlimeRancher.jpg">

### Controls

| Key                | Description                                        |
|--------------------|----------------------------------------------------|
| <kbd>W</kbd>       | Up                                                 |
| <kbd>A</kbd>       | Left                                               |
| <kbd>S</kbd>       | Right                                              |
| <kbd>D</kbd>       | Down                                               |
| <kbd>Space</kbd>   | Light Up Orb                                       |

### Media Sources
Sources from OpenGameArt.org: <br>
Obj and Texture Files (public domain) <br>
"In to the Forest" by Snabisch  <br>
"Lively Meadow" by Mathew Pablo  <br>

### Video Demo

***Video-File*** <br>

YouTube: https://youtu.be/VbepFlzNZ0M <br>

# Adapted Vulkan Tutorial

The Adapted Vulkan Toturial shows how to draw an object using the Vulkan API. It is based on the origin Vulkan Tutorial, but has been adapted to include the following features:

* SDL2 instead of GLFW.
* Vulkan Memory Allocator (VMA).
* Volk function loader.
* Imgui.
* Slang instead of GLSL.

All features except Imgui are already part of the Vulkan SDK, and thus make sense to be used.

On Windows compile using MSVC and the compile.cmd file, or use CMake from VS Code or run (using MSVC generator)
```
cmake -B build
cmake --build build --config Debug
build\Debug\28_model_loading.exe
```

You need two prerequisites:

* The Vulkan SDK needs to be installed, and the environment variable VULKAN_SDK needs to point to it.
* You need to clone the IMGUI repo (https://github.com/ocornut/imgui) into some directory. In the file *compile.cmd* or *CMakeLists.txt*, set the IMGUI variable to this directory. 
* When using CMake, Imgui is automatically fetched from the Git repo (usually to build/_deps/imgui-src), no need to clone it.

Only the CPP files are needed, no need to compile it.

"# CustomVulkanEngine" 
