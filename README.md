# gpu~

A Pure Data external that uses OpenGL compute shaders (GLSL) for real-time audio processing.

This external supports multichannel audio and allows passing arbitrary float parameters to the shader for dynamic control. Below and example of the shader:

``` gsl
#version 430
layout(local_size_x = 1) in;

// do not change this
layout(std430, binding = 0) buffer InputData { float inData[]; };
layout(std430, binding = 1) buffer OutputData { float outData[]; };
layout(std430, binding = 2) buffer ParamData { float parametersData[];};

// here you can change, this multiplies the input by a parameters. Same as use [*~].
void main() {
    uint i = gl_GlobalInvocationID.x;
    float x = inData[i];
    outData[i] = x * parametersData[0];
}

```

## Features

- GPU-accelerated audio processing using GLSL compute shaders.
- Supports multiple audio channels.
- Dynamically reloadable shaders.
- Parameter setting via messages in Pure Data.

## Requirements

- OpenGL 4.3 or higher (for compute shader support).
- Pure Data (Pd) 0.55 or higher recommended.
- A C++ compiler with C++23 or newer.
- GLEW or equivalent for OpenGL function loading.
- CMake (recommended for building).

## Building

```sh
mkdir build
cd build
cmake ..
make

