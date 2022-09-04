#include "asset_pipeline.hpp"

#include "data_path.hpp"
#include "load_save_png.hpp"
#include "PPU466.hpp"

#include <glm/glm.hpp>
#include <vector>
#include <iostream>
#include <fstream>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext.hpp"

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
    std::cout << glm::to_string(palette_size) << std::endl;

    // Set PPU palette to the palette that was loaded
    for (uint8_t i = 0; i < palette_height * palette_width; i += palette_width) {
        std::cout << glm::to_string(palette[i + 3]) << std::endl;
        ppu.palette_table[i / palette_width] = {
            palette[i],
            palette[i + 1],
            palette[i + 2],
            palette[i + 3]
        };
    }
}

void generate_sprite(PPU466 &ppu, std::string filename, uint8_t tile_index, uint8_t palette_index, uint8_t sprite_index, std::string front_or_back) {
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

    // Create the sprite
    ppu.sprites[sprite_index].x = sprite_index * 20;
    ppu.sprites[sprite_index].y = 0;
    ppu.sprites[sprite_index].index = tile_index;
    ppu.sprites[sprite_index].attributes = front_or_back == "front" ? palette_index : (1 << 7) + palette_index;
}

void generate_sprites(PPU466 &ppu) {
    std::ifstream file_stream;
    std::string sprite_map = data_path("data/sprites.txt");
    file_stream.open(sprite_map);

    

    uint8_t tile_index = 0;
    uint8_t sprite_index = 0;

    auto get_sprite_info = [&tile_index, &sprite_index](PPU466 &ppu, std::string line) {
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

        std::string front_or_back = "";
        for (index = index + 1; index < line.size(); index++) {
            if (line[index] != ' ') {
                front_or_back += line[index];
            }
            else {
                break;
            }
        }

        uint8_t palette_index = line[index + 1] - '0';

        generate_sprite(ppu, sprite_name, tile_index, palette_index, sprite_index, front_or_back);
    };

    while (!file_stream.eof()) {
        std::string line;
        std::getline(file_stream, line);
        
        get_sprite_info(ppu, line);

        tile_index++;
        sprite_index++;
    }
    
}