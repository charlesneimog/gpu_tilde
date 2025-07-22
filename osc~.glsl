#version 430
layout(local_size_x = 1) in;

layout(std430, binding = 0) buffer InputData   { float inData[]; };
layout(std430, binding = 1) buffer OutputData  { float outData[]; };
layout(std430, binding = 2) buffer AudioData   { float audioData[]; };
layout(std430, binding = 3) buffer ParamData   { float parametersData[]; };

const float PI = 3.14159265358979323846;

void main() {
    uint i = gl_GlobalInvocationID.x;

    float globalPhase1Hz = audioData[0]; // fase acumulada para 1 Hz entre 0 e 2π
    float sampleRate     = audioData[1];
    uint  blockSize      = uint(audioData[2]);
    uint  numChannels    = uint(audioData[3]);

    float fundamental = parametersData[0];

    uint channel     = i / blockSize;
    uint sampleIndex = i % blockSize;

    float freq = fundamental * float(channel + 1);

    // incremento por sample para esta frequência
    float delta = 2.0 * PI * freq / sampleRate;

    // fase total = fase 1Hz * freq + incremento local pela amostra
    float phase = globalPhase1Hz * freq + delta * float(sampleIndex);

    outData[i] = sin(phase);
}

