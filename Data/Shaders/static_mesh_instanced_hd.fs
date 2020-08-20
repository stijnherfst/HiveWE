#version 450 core

layout (binding = 0) uniform sampler2D diffuse;
layout (binding = 1) uniform sampler2D normal;
layout (binding = 2) uniform sampler2D orm;
layout (binding = 3) uniform sampler2D environemt;
layout (binding = 4) uniform sampler2D emissive;

layout (location = 1) uniform float alpha_test;
layout (location = 2) uniform bool show_lighting;

in vec2 UV;
in vec3 Normal;
in vec3 TangentLightDirection;

out vec4 color;

void main() {

	color = texture(diffuse, UV);
	
	vec2 preNormal = texture(normal, UV).rg * 2.f - 1.f;
	vec3 normall = vec3(preNormal, sqrt(1.f - preNormal.x * preNormal.x - preNormal.y * preNormal.y));

	if (show_lighting) {
		float contribution = (dot(normall, TangentLightDirection) + 1.f) * 0.5f;

		color.rgb *= clamp(contribution + 0.3f, 0.f, 1.f);
	}

	if (color.a < alpha_test) {
		discard;
	}
}