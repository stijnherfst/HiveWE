#version 450 core

layout (location = 0) uniform mat4 MVP;
layout (location = 1) uniform vec4 shallow_color_min;
layout (location = 2) uniform vec4 shallow_color_max;
layout (location = 3) uniform vec4 deep_color_min;
layout (location = 4) uniform vec4 deep_color_max;
layout (location = 5) uniform float water_offset;
layout (location = 7) uniform ivec2 map_size;

out vec2 UV;
out vec4 Color;

const float min_depth = 10.f / 128;
const float deeplevel = 64.f / 128;
const float maxdepth = 72.f / 128;

const vec2[6] position = vec2[6](
	vec2(1, 1),
	vec2(0, 1),
	vec2(0, 0),
	vec2(0, 0),
	vec2(1, 0),
	vec2(1, 1)
);

layout(std430, binding = 0) buffer layoutName {
    float cliff_levels[];
};

layout(std430, binding = 1) buffer layoutName2 {
    float water_heights[];
};

layout(std430, binding = 2) buffer layoutName3 {
    unsigned char water_exists[];
};

void main() { 
	// Position of the quad's bottom left vertex
	ivec2 quad_pos = ivec2(gl_InstanceID % map_size.x, gl_InstanceID / map_size.x);
	
	bool is_water = water_exists[quad_pos.y * map_size.x + quad_pos.x] > 0u
				 || water_exists[quad_pos.y * map_size.x + quad_pos.x + 1] > 0u
				 || water_exists[(quad_pos.y + 1) * map_size.x + quad_pos.x] > 0u
				 || water_exists[(quad_pos.y + 1) * map_size.x + quad_pos.x + 1] > 0u;

	UV = vec2(position[gl_VertexID].x, 1.f - position[gl_VertexID].y);

	ivec2 height_pos = ivec2(position[gl_VertexID]) + quad_pos;
	const float water_height = water_heights[height_pos.y * map_size.x + height_pos.x] + water_offset;
	const float ground_height = cliff_levels[height_pos.y * map_size.x + height_pos.x];

	float value = clamp(water_height - ground_height, 0.f, 1.f);
	if (value <= deeplevel) {
		value = max(0.f, value - min_depth) / (deeplevel - min_depth);
		Color = shallow_color_min * (1.f - value) + shallow_color_max * value;
	} else {
		value = clamp(value - deeplevel, 0.f, maxdepth - deeplevel) / (maxdepth - deeplevel);
		Color = deep_color_min * (1.f - value) + deep_color_max * value;
	}
	
	gl_Position = is_water ? MVP * vec4(position[gl_VertexID] + quad_pos, water_height, 1) : vec4(2.0, 0.0, 0.0, 1.0);
 }