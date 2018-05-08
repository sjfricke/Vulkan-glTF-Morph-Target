# Vulkan Morph Targets (Blend Shapes) using glTF 2.0 models

This was made to be a proof of concepts for people to reference when hacking at their own project

## Credit

Most of the template is from [Sascha Willem's Vulkan-glTF-PBR demo](https://github.com/SaschaWillems/Vulkan-glTF-PBR) so huge shout out to him! I wanted to use morph targets for my own purpose, knew [glTF had it in its specification](https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#morph-targets) and decided to create a demo since no one else seems to have.

## glTF 2.0 Model loading

Model loading and rendering is implemented in the [vkglTF::Model](./base/VulkanglTFModel.hpp) class and uses the [tiny glTF library](https://github.com/syoyo/tinygltf) to import the glTF 2.0 files. 

Note that this is not a full glTF model class implementation, this was to show the steps for morph target rendering/parsing.

## Cloning

This repository contains submodules for some of the external dependencies, so when doing a fresh clone you need to clone recursively:

```
git clone --recursive https://github.com/sjfricke/Vulkan-glTF-Morph-Target.git
```

Updating submodules manually:

```
git submodule init
git submodule update
```

## Building

The repository contains everything required to compile and build the examples on **Windows**, **Linux** and **Android** using a C++ compiler that supports C++11. All required dependencies are included.

### Windows, Linux

Use the provided CMakeLists.txt with CMake to generate a build configuration for your favorite IDE or compiler, e.g.:

Windows:
```
cmake -G "Visual Studio 14 2015 Win64"


cd bin
Vulkan-glTF-Morph-Target.exe
```

Linux:
```
cmake .
make

cd bin
./Vulkan-glTF-Morph-Target
```

### Android 

#### Prerequisites

- [Android Studio 3.0](https://developer.android.com/studio/index.html) or higher
- Android [NDK](https://developer.android.com/ndk/downloads/index.html)
- Android API 24+

#### Building
- In Android Studio, select ```Import project```
- Select the ```android``` sub folder of the repository
- Once import has finished the project can be build, run and debugged from Android Studio

## Links
* [glTF format specification](https://github.com/KhronosGroup/glTF)
* [glTF Sample Models](https://github.com/KhronosGroup/glTF-Sample-Models)
* [tiny glTF library](https://github.com/syoyo/tinygltf)