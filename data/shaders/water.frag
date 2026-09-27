#version 450 core

#extension GL_GOOGLE_include_directive : require
#include "terrain_common.glsl"

layout (location = 0) in vec2 UV;
layout (location = 1) in vec4 Color;

layout (location = 0) out vec4 outColor;

void main() {
	TerrainFrame frame = pc.frame;
	outColor = texture(array_textures[frame.water_texture_slot], vec3(UV, frame.current_water_texture)) * Color;
}
