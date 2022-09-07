#include "generate_ppu.hpp"

#include "data_path.hpp"
#include "load_save_png.hpp"
#include "PPU466.hpp"

#include <glm/glm.hpp>
#include <vector>
#include <iostream>
#include <fstream>

#include <unordered_map>

void generate_palette(PPU466 &ppu) {
    // Load palette file
    std::string palette_file = data_path("data/palette.png");
    glm::uvec2 palette_size;
    std::vector<glm::u8vec4> palette;
    OriginLocation origin = UpperLeftOrigin;
    uint8_t palette_width = 4;
    uint8_t palette_height = 8;
    load_png(palette_file, &palette_size, &palette, origin);
    if (palette_size != glm::uvec2(palette_width, palette_height)) {
        std::cerr << "File " << palette_file << " is not of expected size (8 x 4) " << std::endl;
    }

    // Set PPU palette to the palette that was loaded
    for (uint8_t i = 0; i < palette_height * palette_width; i += palette_width) {
        ppu.palette_table[i / palette_width] = {
            palette[i],
            palette[i + 1],
            palette[i + 2],
            palette[i + 3]
        };
    }
}

void generate_tile(PPU466 &ppu, std::string filename, uint8_t tile_index, uint8_t palette_index) {
    auto palette_table_entry = ppu.palette_table[palette_index];

    // Load tile
    std::string tile_file = data_path(filename);
    glm::uvec2 tile_size;
    std::vector<glm::u8vec4> tile_data;
    OriginLocation origin = UpperLeftOrigin;
    uint8_t tile_width = 8;

    load_png(tile_file, &tile_size, &tile_data, origin);
    if (tile_size != glm::uvec2(tile_width, tile_width)) {
        std::cerr << "File " << tile_file << " is not of expected size (8 x 8) " << std::endl;
    }
    
    // Create bits for tile
    std::array<uint8_t, 8> tile_bit0 = {0};
    std::array<uint8_t, 8> tile_bit1 = {0};
    // Loop through rows of tile
    for (uint8_t i = 0; i < tile_width; i++) {
        uint8_t i_backwards = 8 - 1 - i;
        tile_bit0[i_backwards] = 0;
        tile_bit1[i_backwards] = 0;
        // Loop through columns of tile
        for (uint8_t j = 0; j < tile_width; j++) {
            tile_bit0[i_backwards] <<= 1;
            tile_bit1[i_backwards] <<= 1;
            glm::u8vec4 color = tile_data[i * tile_width + j];
            // Find index in palette
            uint8_t color_index = 0;
            for (uint8_t k = 0; k < palette_table_entry.size(); k++) {
                if (color == palette_table_entry[k]) {
                    color_index = k;
                    break;
                }
            }
            // Create bit0 and bit1 values
            tile_bit0[i_backwards] += color_index % 2;
            tile_bit1[i_backwards] += color_index / 2;
        }
    }

    // Finish creation of tile
    ppu.tile_table[tile_index].bit0 = tile_bit0;
    ppu.tile_table[tile_index].bit1 = tile_bit1;
}

void generate_sprites(PPU466 &ppu, std::unordered_map<std::string, std::vector<uint8_t>> &sprite_indices) {
    std::ifstream file_stream;
    std::string sprite_map = data_path("data/sprites.txt");
    file_stream.open(sprite_map);

    uint8_t tile_index = 0;
    uint8_t sprite_index = 0;

    auto get_sprite_info_and_generate = [&tile_index, &sprite_index, &sprite_indices](PPU466 &ppu, std::string line) {
        // Format of txt file: name of png, front or back, number of sprites, palette index

        // Get name of png
        std::string sprite_name = "";
        uint16_t index = 0;
        for (index = 0; index < line.size(); index++) {
            if (line[index] != ' ') {
                sprite_name += line[index];
            }
            else {
                sprite_name = "data/" + sprite_name + ".png";
                break;
            }
        }

        // Get front or back info
        std::string front_or_back = "";
        for (index = index + 1; index < line.size(); index++) {
            if (line[index] != ' ') {
                front_or_back += line[index];
            }
            else {
                break;
            }
        }

        // Get number of sprites
        std::string num_sprites_str = "";
        for (index = index + 1; index < line.size(); index++) {
            if (line[index] != ' ') {
                num_sprites_str += line[index];
            }
            else {
                break;
            }
        }
        uint8_t num_sprites = (uint8_t)(std::stoi(num_sprites_str));

        // Get palette index
        uint8_t palette_index = line[index + 1] - '0';
        index++;

        // Generate tile
        generate_tile(ppu, sprite_name, tile_index, palette_index);

        // Generate sprites associated with tile
        std::vector<uint8_t> indices;
        sprite_indices[sprite_name] = indices;
        for (uint8_t i = 0; i < num_sprites; i++) {
            sprite_indices[sprite_name].push_back(sprite_index);
            
            ppu.sprites[sprite_index].x = i * 60;
            ppu.sprites[sprite_index].y = tile_index * 20;
            ppu.sprites[sprite_index].index = tile_index;
            ppu.sprites[sprite_index].attributes = front_or_back == "front" ? palette_index : (1 << 7) + palette_index;
            sprite_index++;
        }

        tile_index++;
    };

    while (!file_stream.eof()) {
        std::string line;
        std::getline(file_stream, line);
        
        get_sprite_info_and_generate(ppu, line);
    }
    
}

void generate_background(PPU466 &ppu) {
    uint8_t tile_index = 16;
    uint8_t palette_index = 3;
    generate_tile(ppu, "data/background.png", tile_index, palette_index);
    for (uint16_t i = 0; i < ppu.BackgroundWidth * ppu.BackgroundHeight; i++) {
        ppu.background[i] = (palette_index << 8) + tile_index;
    }
}