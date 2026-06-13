#include "HeightmapGenerator.h"
#include "FastNoiseLite.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include "../utils/Filters.h"

Heightmap HeightmapGenerator::generate(int baseSeed, int offsetX, int offsetY, int width, int height, const std::vector<NoiseLayer>& layers, const ModifierParams& modifiers) {
    Heightmap finalMap(width, height);
    
    if (layers.empty()) return finalMap;

    for (const auto& layer : layers) {
        if (!layer.enabled) continue;

        FastNoiseLite noise;
        noise.SetSeed(baseSeed + layer.seedOffset);
        noise.SetFrequency(layer.baseFrequency);
        noise.SetFractalOctaves(layer.octaves);
        noise.SetFractalLacunarity(layer.scale);
        noise.SetFractalGain(layer.persistence);

        switch (layer.type) {
            case NoiseType::Perlin:
                noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
                noise.SetFractalType(FastNoiseLite::FractalType_FBm);
                break;
            case NoiseType::Simplex:
                noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
                noise.SetFractalType(FastNoiseLite::FractalType_FBm);
                break;
            case NoiseType::Ridged:
                noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
                noise.SetFractalType(FastNoiseLite::FractalType_Ridged);
                break;
            case NoiseType::PingPong:
                noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
                noise.SetFractalType(FastNoiseLite::FractalType_PingPong);
                noise.SetFractalPingPongStrength(2.0f);
                break;
            case NoiseType::Cellular:
                noise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
                noise.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_EuclideanSq);
                noise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance);
                // Voronoi/Cellular isn't usually fractalized via FBM effectively without custom weighting, 
                // but FastNoiseLite can do it.
                noise.SetFractalType(FastNoiseLite::FractalType_FBm);
                break;
            case NoiseType::Value:
                noise.SetNoiseType(FastNoiseLite::NoiseType_Value);
                noise.SetFractalType(FastNoiseLite::FractalType_FBm);
                break;
        }

        // Generate layer and blend
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                // FastNoiseLite returns values roughly [-1, 1]
                float val = noise.GetNoise((float)(x + offsetX), (float)(y + offsetY));
                
                // For distance-based cellular, return is [0, 1] but can go > 1, so remap if necessary.
                // Generally we map [-1, 1] -> [0, 1] for all
                if (layer.type != NoiseType::Cellular) {
                    val = (val + 1.0f) * 0.5f;
                }
                
                // PingPong returns positive values, but may need bounding
                val = std::clamp(val, 0.0f, 1.0f);
                
                float weightedVal = val * layer.weight;
                
                float& current = finalMap.at(x, y);
                
                switch (layer.blend) {
                    case BlendMode::Add: current += weightedVal; break;
                    case BlendMode::Subtract: current -= weightedVal; break;
                    case BlendMode::Multiply: current *= (val); break; // Multiply by unweighted val or 1? Often we want full mix. Let's do current *= (1.0f - layer.weight + val * layer.weight)
                    case BlendMode::Min: current = std::min(current, val * layer.weight); break;
                    case BlendMode::Max: current = std::max(current, val * layer.weight); break;
                }
            }
        }
    }
    
    // Multiply logic tweak:
    // If multiply was used, the above simplistic approach works but could be better. For now we keep it Add mostly.
    
    // Apply Modifiers
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float& val = finalMap.at(x, y);

            // Island Modifier (RedBlobGames Distance Field)
            if (modifiers.islandMix > 0.0f) {
                float nx = 2.0f * x / (width - 1.0f) - 1.0f;
                float ny = 2.0f * y / (height - 1.0f) - 1.0f;
                // Square bump distance function
                float d = 1.0f - (1.0f - nx * nx) * (1.0f - ny * ny);
                // Lerp between val and (1 - d)
                val = val * (1.0f - modifiers.islandMix) + (1.0f - d) * modifiers.islandMix;
            }

            // Exponentiation (RedBlobGames Redistribution)
            if (modifiers.exponent != 1.0f) {
                val = std::pow(std::max(0.0f, val), modifiers.exponent);
            }

            // Terracing
            if (modifiers.terraceSteps > 0) {
                val = std::round(val * modifiers.terraceSteps) / (float)modifiers.terraceSteps;
            }
        }
    }

    // Smoothing moved to post-erosion in main.cpp
    // Strict Normalization
    float minV = 99999.0f, maxV = -99999.0f;
    for (float v : finalMap.data) {
        if (v < minV) minV = v;
        if (v > maxV) maxV = v;
    }
    float range = maxV - minV;
    if (range > 0.0f) {
        for (float& v : finalMap.data) {
            v = (v - minV) / range;
        }
    }

    return finalMap;
}
