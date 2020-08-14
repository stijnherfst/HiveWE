#version 450 core

layout (binding = 0) uniform sampler2D image;

layout (location = 1) uniform float alpha_test;
layout (location = 2) uniform bool show_lighting;

in vec2 UV;
in vec3 Normal;

out vec4 outColor;

void main() {
	outColor = texture(image, UV);

	if (show_lighting) {
		vec3 light_direction = vec3(-0.3f, -0.3f, 0.25f);
		light_direction = normalize(light_direction);

		float contribution = (dot(Normal, light_direction) + 1.f) * 0.5f;

		outColor.rgb *= clamp(contribution + 0.3f, 0.f, 1.f);
	}

	if (outColor.a < alpha_test) {
		discard;
	}
}