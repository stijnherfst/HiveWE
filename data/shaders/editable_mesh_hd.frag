#version 450 core

layout (binding = 0) uniform sampler2D albedo;
layout (binding = 1) uniform sampler2D normal_map;
layout (binding = 2) uniform sampler2D orm;
layout (binding = 3) uniform sampler2D emissive;
layout (binding = 4) uniform sampler2D team_color_texture;

layout (location = 1) uniform float alpha_test;
layout (location = 2) uniform bool show_lighting;

in vec2 UV;
in vec3 tangent_light_direction;
in vec4 vertexColor;
in vec3 team_color;

out vec4 color;

void main() {
	color = texture(albedo, UV) * vertexColor;

	if (vertexColor.a == 0.0 || color.a < alpha_test) {
		discard;
	}

	if (show_lighting) {
		vec3 emissive_texel = texture(emissive, UV).rgb;
		vec4 orm_texel = texture(orm, UV);
		vec3 tc_texel = texture(team_color_texture, UV).rgb;
		color.rgb = (color.rgb * (1 - orm_texel.w) + color.rgb * tc_texel.r * team_color * orm_texel.w);

		vec2 normal_texel = texture(normal_map, UV).xy * 2.0 - 1.0;
		vec3 N = vec3(normal_texel, sqrt(max(0.0, 1.0 - dot(normal_texel, normal_texel))));

		float lambert = clamp(dot(N, -tangent_light_direction), 0.f, 1.f);
		color.rgb *= clamp(lambert + 0.1, 0.f, 1.f);
		color.rgb += emissive_texel;
	}
}
