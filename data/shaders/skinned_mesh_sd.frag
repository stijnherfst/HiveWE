#version 450 core

layout (binding = 0) uniform sampler2D diffuse;

layout (location = 2) uniform bool show_lighting;
layout (location = 8) uniform vec3 light_direction;
layout (location = 10) uniform bool is_team_color;

in vec2 UV;
in vec3 Normal;
in vec4 vertexColor;
in vec3 team_color;

out vec4 color;

void main() {
	if (is_team_color) {
		color = vec4(team_color * texture(diffuse, UV).r, 1.f) * vertexColor;
	} else {
		color = texture(diffuse, UV) * vertexColor;
	}

	if (show_lighting) {
		float contribution = (dot(Normal, -light_direction) + 1.f) * 0.5f;
		color.rgb *= clamp(contribution, 0.f, 1.f);
	}
}