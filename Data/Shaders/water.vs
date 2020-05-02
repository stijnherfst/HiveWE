#version 450 core

layout (location = 0) in vec2 vPosition;

layout (binding = 0) uniform sampler2D water_height_texture;
layout (binding = 1) uniform sampler2D ground_height_texture;
layout (binding = 2) uniform sampler2D water_exists_texture;

layout (location = 0) uniform mat4 MVP;
layout (location = 1) uniform vec4 shallow_color_min;
layout (location = 2) uniform vec4 shallow_color_max;
layout (location = 3) uniform vec4 deep_color_min;
layout (location = 4) uniform vec4 deep_color_max;
layout (location = 5) uniform float water_offset;

out vec2 UV;
out vec4 Color;

const float min_depth = 10.f / 128;
const float deeplevel = 64.f / 128;
const float maxdepth = 72.f / 128;

void main() { 
	ivec2 size = textureSize(water_height_texture, 0) - 1;
	ivec2 pos = ivec2(gl_InstanceID % size.x, gl_InstanceID / size.x);
	ivec2 height_pos = ivec2(vPosition + pos);
	float water_height = texelFetch(water_height_texture, height_pos, 0).r + water_offset;

	bool is_water = texelFetch(water_exists_texture, pos, 0).r > 0
	 || texelFetch(water_exists_texture, pos + ivec2(1, 0), 0).r > 0
	 || texelFetch(water_exists_texture, pos + ivec2(1, 1), 0).r > 0
	 || texelFetch(water_exists_texture, pos + ivec2(0, 1), 0).r > 0;

	gl_Position = is_water ? MVP * vec4(vPosition + pos, water_height, 1) : vec4(2.0, 0.0, 0.0, 1.0);

	UV = vec2(vPosition.x, 1 - vPosition.y);

	float ground_height = texelFetch(ground_height_texture, height_pos, 0).r;
	float value = clamp(water_height - ground_height, 0.f, 1.f);
	if (value <= deeplevel) {
		value = max(0.f, value - min_depth) / (deeplevel - min_depth);
		Color = shallow_color_min * (1.f - value) + shallow_color_max * value;
	} else {
		value = clamp(value - deeplevel, 0.f, maxdepth - deeplevel) / (maxdepth - deeplevel);
		Color = deep_color_min * (1.f - value) + deep_color_max * value;
	}
 }