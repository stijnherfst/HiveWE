#version 450 core

layout (binding = 0) uniform sampler2D image;

layout (location = 1) uniform float alpha_test;
layout (location = 2) uniform bool show_lighting;

in vec2 UV;
in vec3 Normal;
in vec4 vertexColor;

out vec4 outColor;

void main() {
	outColor = texture(image, UV) * vertexColor;

	// if (show_lighting) {
	// 	vec3 light_direction = vec3(-0.3, -0.3, 0.25);
	// 	light_direction = normalize(light_direction);

	// 	outColor.rgb *= clamp(dot(Normal, light_direction) + 0.45, 0, 1);
	// }

	if (outColor.a < alpha_test) {
		discard;
	}
}