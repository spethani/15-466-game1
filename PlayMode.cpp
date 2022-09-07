#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>

//for processing png images
# include "generate_ppu.hpp"

// for keeping track of sprite indices
# include <unordered_map>

// for making sure car x pos <= 250
#include <cmath>

// for keeping track of frog color
#include<string>

std::unordered_map<std::string, std::vector<uint8_t>> sprite_indices;
std::vector<float> sprite_xs(64);
std::vector<float> sprite_ys(64);
float time_since_frog_change;
std::string frog_color;

PlayMode::PlayMode() {

	generate_palette(ppu);
	generate_sprites(ppu, sprite_indices);
	generate_background(ppu);

	{// Position the car sprites on the background
		std::vector<uint8_t> red_cars = sprite_indices["data/red_car.png"];
		std::vector<uint8_t> green_cars = sprite_indices["data/green_car.png"];
		std::vector<uint8_t> yellow_cars = sprite_indices["data/yellow_car.png"];
		for (uint8_t i = 0; i < red_cars.size() && i < green_cars.size() && i < yellow_cars.size(); i++) {
			float x_start = 20.0f;
			float x_offset = 80.0f;
			float y_start = 20.0f;
			float y_offset = 40.0f;
			
			uint8_t red_car_idx = red_cars[i];
			uint8_t green_car_idx = green_cars[i];
			uint8_t yellow_car_idx = yellow_cars[i];

			sprite_xs[red_car_idx] = x_start;
			sprite_ys[red_car_idx] = i * y_offset + y_start;
			ppu.sprites[red_car_idx].x = (uint8_t) sprite_xs[red_car_idx];
			ppu.sprites[red_car_idx].y = (uint8_t) sprite_ys[red_car_idx];

			sprite_xs[green_car_idx] = x_start + x_offset;
			sprite_ys[green_car_idx] = i * y_offset + y_start;
			ppu.sprites[green_car_idx].x = (uint8_t) sprite_xs[green_car_idx];
			ppu.sprites[green_car_idx].y = (uint8_t) sprite_ys[green_car_idx];

			sprite_xs[yellow_car_idx] = x_start + 2 * x_offset;
			sprite_ys[yellow_car_idx] = i * y_offset + y_start;
			ppu.sprites[yellow_car_idx].x = (uint8_t) sprite_xs[yellow_car_idx];
			ppu.sprites[yellow_car_idx].y = (uint8_t) sprite_ys[yellow_car_idx];
		}
	}

	{// Position the frogs on the background
		frog_color = "green";
		uint8_t green_frog_idx = sprite_indices["data/green_frog.png"][0];
		uint8_t red_frog_idx = sprite_indices["data/red_frog.png"][0];
		uint8_t yellow_frog_idx = sprite_indices["data/yellow_frog.png"][0];
		ppu.sprites[green_frog_idx].x = 0;
		ppu.sprites[green_frog_idx].y = 0;
		ppu.sprites[red_frog_idx].x = 250;
		ppu.sprites[red_frog_idx].y = 250;
		ppu.sprites[yellow_frog_idx].x = 250;
		ppu.sprites[yellow_frog_idx].y = 250;
	}

	// Keep track of time elapsed since frog last changed color
	time_since_frog_change = 0.0f;

}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//slowly rotates through [0,1):
	// (will be used to set background color)
	background_fade += elapsed / 10.0f;
	background_fade -= std::floor(background_fade);

	constexpr float PlayerSpeed = 30.0f;
	if (left.pressed) player_at.x -= PlayerSpeed * elapsed;
	if (right.pressed) player_at.x += PlayerSpeed * elapsed;
	// don't let player go down further than bottom of screen
	if (down.pressed) player_at.y = fmax(player_at.y - PlayerSpeed * elapsed, 0.0f);
	if (up.pressed) player_at.y += PlayerSpeed * elapsed;

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;

	{// Update car positions
		std::vector<uint8_t> red_cars = sprite_indices["data/red_car.png"];
		std::vector<uint8_t> green_cars = sprite_indices["data/green_car.png"];
		std::vector<uint8_t> yellow_cars = sprite_indices["data/yellow_car.png"];
		for (uint8_t i = 0; i < red_cars.size() && i < green_cars.size() && i < yellow_cars.size(); i++) {
			float car_speed = i * 30.0f + 30.0f;
			uint8_t red_car_idx = red_cars[i];
			uint8_t green_car_idx = green_cars[i];
			uint8_t yellow_car_idx = yellow_cars[i];
			sprite_xs[red_car_idx] += car_speed * elapsed;
			sprite_xs[green_car_idx] += car_speed * elapsed;
			sprite_xs[yellow_car_idx] += car_speed * elapsed;
			// Make sure x positions don't go past screen width
			sprite_xs[red_car_idx] = (float) fmod(sprite_xs[red_car_idx], 256);
			sprite_xs[green_car_idx] = (float) fmod(sprite_xs[green_car_idx], 256);
			sprite_xs[yellow_car_idx] = (float) fmod(sprite_xs[yellow_car_idx], 256);
		}
	}

	{// Check collisions for cars
		const uint8_t car_height = 5;
		const uint8_t car_width = 8;
		float min_player_x = player_at.x;
		float max_player_x = player_at.x + 8;
		float min_player_y = player_at.y;
		float max_player_y = player_at.y + 8;
		// Check collisions for red cars
		for (uint8_t red_car_idx : sprite_indices["data/red_car.png"]) {
			float min_car_x = sprite_xs[red_car_idx];
			float max_car_x = sprite_xs[red_car_idx] + car_width;
			float min_car_y = sprite_ys[red_car_idx];
			float max_car_y = sprite_ys[red_car_idx] + car_height;
			if ((max_car_x >= min_player_x && max_player_x >= min_car_x) 
				&& (max_car_y >= min_player_y && max_player_y >= min_car_y)) {
				if (frog_color != "red") { // If the frog is same color as car, no collision
					player_at.x = 0;
					player_at.y = 0;
				}
				break;
			}
		}
		// Check collisions for green cars
		for (uint8_t green_car_idx : sprite_indices["data/green_car.png"]) {
			float min_car_x = sprite_xs[green_car_idx];
			float max_car_x = sprite_xs[green_car_idx] + car_width;
			float min_car_y = sprite_ys[green_car_idx];
			float max_car_y = sprite_ys[green_car_idx] + car_height;
			if ((max_car_x >= min_player_x && max_player_x >= min_car_x) 
				&& (max_car_y >= min_player_y && max_player_y >= min_car_y)) {
				if (frog_color != "green") {
					player_at.x = 0;
					player_at.y = 0;
				}
				break;
			}
		}
		// Check collisions for yellow cars
		for (uint8_t yellow_car_idx : sprite_indices["data/yellow_car.png"]) {
			float min_car_x = sprite_xs[yellow_car_idx];
			float max_car_x = sprite_xs[yellow_car_idx] + car_width;
			float min_car_y = sprite_ys[yellow_car_idx];
			float max_car_y = sprite_ys[yellow_car_idx] + car_height;
			if ((max_car_x >= min_player_x && max_player_x >= min_car_x) 
				&& (max_car_y >= min_player_y && max_player_y >= min_car_y)) {
				if (frog_color != "yellow") {
					player_at.x = 0;
					player_at.y = 0;
				}
				break;
			}
		}
	}

	{// Check for win conditions
		if (player_at.y > 240) {
			std::cout << "You Win! Froggo has passed safely." << std::endl;
			std::cout << "Feel free to keep playing." << std::endl;
			player_at.y -= 240;
		}
	}

	// Update time since frog change
	time_since_frog_change += elapsed;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- set ppu state based on game state ---

	//background color will be some hsv-like fade:
	ppu.background_color = glm::u8vec4(
		std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 0.0f / 3.0f) ) ) ))),
		std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 1.0f / 3.0f) ) ) ))),
		std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 2.0f / 3.0f) ) ) ))),
		0xff
	);

	//background scroll:
	ppu.background_position.x = int32_t(-0.5f * player_at.x);
	ppu.background_position.y = int32_t(-0.5f * player_at.y);

	{//decide frog color
		const float time_for_color = 2.0f;
		if (time_since_frog_change > time_for_color) {
			if (frog_color == "green") {
				frog_color = "yellow";
			}
			else if (frog_color == "yellow") {
				frog_color = "red";
			}
			else if (frog_color == "red") {
				frog_color = "green";
			}
			time_since_frog_change -= time_for_color;
		}
	}

	{//player sprite
		if (frog_color == "green") {
			ppu.sprites[0].x = int8_t(player_at.x);
			ppu.sprites[0].y = int8_t(player_at.y);
			ppu.sprites[1].x = 250;
			ppu.sprites[1].y = 250;
			ppu.sprites[2].x = 250;
			ppu.sprites[2].y = 250;
		}
		else if (frog_color == "yellow") {
			ppu.sprites[2].x = int8_t(player_at.x);
			ppu.sprites[2].y = int8_t(player_at.y);
			ppu.sprites[0].x = 250;
			ppu.sprites[0].y = 250;
			ppu.sprites[1].x = 250;
			ppu.sprites[1].y = 250;
		}
		else if (frog_color == "red") {
			ppu.sprites[1].x = int8_t(player_at.x);
			ppu.sprites[1].y = int8_t(player_at.y);
			ppu.sprites[0].x = 250;
			ppu.sprites[0].y = 250;
			ppu.sprites[2].x = 250;
			ppu.sprites[2].y = 250;
		}
	}

	//car sprites:
	{
		std::vector<uint8_t> red_cars = sprite_indices["data/red_car.png"];
		std::vector<uint8_t> green_cars = sprite_indices["data/green_car.png"];
		std::vector<uint8_t> yellow_cars = sprite_indices["data/yellow_car.png"];
		for (uint8_t i = 0; i < red_cars.size() && i < green_cars.size() && i < yellow_cars.size(); i++) {
			uint8_t red_car_idx = red_cars[i];
			uint8_t green_car_idx = green_cars[i];
			uint8_t yellow_car_idx = yellow_cars[i];
			ppu.sprites[red_car_idx].x = (int8_t)(sprite_xs[red_car_idx]);
			ppu.sprites[green_car_idx].x = (int8_t)(sprite_xs[green_car_idx]);
			ppu.sprites[yellow_car_idx].x = (int8_t)(sprite_xs[yellow_car_idx]);
		}
	}

	//--- actually draw ---
	ppu.draw(drawable_size);
}
