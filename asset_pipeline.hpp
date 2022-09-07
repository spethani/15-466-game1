#pragma once

#include "PPU466.hpp"

#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

void generate_palette(PPU466 &ppu);
void generate_sprites(PPU466 &ppu, std::unordered_map<std::string, std::vector<uint8_t>> &sprite_indices);
void generate_background(PPU466 &ppu);