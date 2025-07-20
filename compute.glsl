#version 430
layout(local_size_x = 1) in;

layout(std430, binding = 0) buffer InputData {
    float inData[];
};

layout(std430, binding = 1) buffer OutputData {
    float outData[];
};

layout(std430, binding = 2) buffer ParamData {
    float parametersData[];
};

void main() {
    uint i = gl_GlobalInvocationID.x;
    float x = inData[i];
    outData[i] = x * parametersData[0];
}
