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

const float PI = 3.14159265358979323846;

void main() {
    uint k = gl_GlobalInvocationID.x; // frequency bin index
    uint N = uint(audioData[2]);      // blockSize (expected 4096)

    float re = 0.0;
    float im = 0.0;

    for (uint n = 0; n < N; n++) {
        float s = inData[n];
        float angle = 2.0 * PI * float(k) * float(n) / float(N);
        re += s * cos(angle);
        im -= s * sin(angle);
    }

    outData[k] = sqrt(re * re + im * im);
}
