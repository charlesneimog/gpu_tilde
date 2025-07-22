# gpu~

A Pure Data external that uses OpenGL compute shaders (GLSL) for real-time audio processing.

This external supports multichannel audio and allows passing arbitrary float parameters to the shader for dynamic control. Below and example of the shader:

``` gsl
#version 430
layout(local_size_x = 1) in;

// Audio Input ch 1 index 0 to blockSize-1, ch 2 from blockSize to 2*blockSize-1, ...
layout(std430, binding = 0) buffer InputData     { float inData[]; };

// Audio Out ch 1 index 0 to blockSize-1, ch 2 from blockSize to 2*blockSize-1, ...
layout(std430, binding = 1) buffer OutputData    { float outData[]; };

layout(std430, binding = 2) buffer AudioData   { float audioData[]; };
// float globalPhase1Hz = audioData[0]; // phase accumuled for 1 Hz between 0 e 2π
// float sampleRate     = audioData[1];
// uint  blockSize      = uint(audioData[2]);
// uint  numChannels    = uint(audioData[3]);

// defined using "param 0 <value>" in puredata
layout(std430, binding = 3) buffer ParamData     { float parametersData[]; };


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
cmake . -B build
cmake --build build
```

