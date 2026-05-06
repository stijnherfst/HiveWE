#version 450 core

#extension GL_ARB_bindless_texture : require

layout (location = 2) uniform bool render_lighting;
layout (location = 3) uniform vec3 light_direction;

struct LayerTextureIds {
	uint albedo;
	uint normal;
	uint orm;
	uint emissive;
	uint team_color;
	uint environment;
	uint _pad0;
	uint _pad1;
};

struct LayerParams {
	float alpha_test;
	uint  layer_lit;
	uint  is_team_color;
	uint  _pad;
};

layout(std430, binding = 9) buffer TextureHandles {
	sampler2D textures[];
};

layout(std430, binding = 10) buffer LayerTexturesBuf {
	LayerTextureIds layer_textures[];
};

layout(std430, binding = 11) buffer LayerParamsBuf {
	LayerParams layer_params[];
};

in vec2 UV;
in vec3 Normal;
in vec4 vertexColor;
in vec3 team_color;
flat in int layer_index;

out vec4 color;

void main() {
	LayerTextureIds ids = layer_textures[layer_index];
	LayerParams p = layer_params[layer_index];
	sampler2D diffuse = textures[ids.albedo];

	if (p.is_team_color != 0u) {
		color = vec4(team_color * texture(diffuse, UV).r, 1.f) * vertexColor;
	} else {
		color = texture(diffuse, UV) * vertexColor;
	}

	if (vertexColor.a == 0.0 || color.a < p.alpha_test) {
		discard;
	}

	if (p.layer_lit != 0u && render_lighting) {
		float contribution = (dot(Normal, -light_direction) + 1.f) * 0.5f;
		color.rgb *= clamp(contribution, 0.f, 1.f);
	}
}
