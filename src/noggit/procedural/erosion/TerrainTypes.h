#pragma once

#include <vector>
#include <array>
#include <cassert>
#include <string>

// Global structures used across modules

enum class BlendMode { Add, Subtract, Multiply, Min, Max };
enum class NoiseType { Perlin, Simplex, Cellular, Value, Ridged, PingPong };

struct NoiseLayer {
    NoiseType type = NoiseType::Simplex;
    int seedOffset = 0;
    int octaves = 4;
    float scale = 1.0f; // Multiplier for frequency
    float persistence = 0.5f;
    float baseFrequency = 0.01f;
    float weight = 1.0f;
    BlendMode blend = BlendMode::Add;
    bool enabled = true;
};

struct ModifierParams {
    float exponent = 1.0f; // Redistribution
    float islandMix = 0.0f; // 0.0 = original, 1.0 = island
    int terraceSteps = 0; // 0 = disabled
    int smoothing_radius = 0; // 0 = disabled
};

struct Heightmap {
    std::vector<float> data;  // row-major, values [0.0, 1.0]
    int width;
    int height;

    Heightmap(int w, int h) : width(w), height(h), data(w * h, 0.0f) {}

    float& at(int x, int y) { 
        assert(x >= 0 && x < width && y >= 0 && y < height);
        return data[y * width + x]; 
    }

    const float& at(int x, int y) const { 
        assert(x >= 0 && x < width && y >= 0 && y < height);
        return data[y * width + x]; 
    }
};

struct ProceduralAlphamap {
    // 4 channels: R=grass, G=rock, B=snow, A=sand/shore
    // weights per pixel must sum to 1.0
    std::vector<std::array<float, 4>> data;
    int width;
    int height;

    ProceduralAlphamap(int w, int h) : width(w), height(h), data(w * h, {0.0f, 0.0f, 0.0f, 0.0f}) {}
};
