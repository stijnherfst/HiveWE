#version 450 core

layout (binding = 0) uniform sampler2D diffuse;
layout (binding = 1) uniform sampler2D normal;
layout (binding = 2) uniform sampler2D orm;
layout (binding = 3) uniform sampler2D emissive;
layout (binding = 4) uniform sampler2D teamColor;
layout (binding = 5) uniform sampler2D environment;

layout (location = 1) uniform float alpha_test;
layout (location = 2) uniform bool show_lighting;

in vec2 UV;
in vec3 tangent_light_direction;
in vec4 vertexColor;

out vec4 color;

void main() {
	// color = vec4(1.f, 1.f, 1.f, 1.f);

	color = texture(diffuse, UV) * vertexColor;
	
	if (show_lighting) {
		vec3 emissive_texel = texture(emissive, UV).rgb;
		vec4 orm_texel = texture(orm, UV);
		vec3 tc_texel = texture(teamColor, UV).rgb;
		color.rgb = (color.rgb * (1 - orm_texel.w) + color.rgb * tc_texel * orm_texel.w);

		// normal is a 2 channel normal map so we have to deduce the 3rd value
		vec2 normal_texel = texture(normal, UV).xy * 2.0 - 1.0;
		vec3 normal = vec3(normal_texel, sqrt(1.0 - dot(normal_texel, normal_texel)));

		float lambert = clamp(dot(normal, -tangent_light_direction), 0.f, 1.f);
		color.rgb *= clamp(lambert + 0.1, 0.f, 1.f);
		color.rgb += emissive_texel;
	}


	if (color.a < alpha_test) {
		discard;
	}
}