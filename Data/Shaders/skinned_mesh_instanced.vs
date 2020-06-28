#version 450 core

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec2 vUV;
layout (location = 2) in vec3 vNormal;
layout (location = 3) in mat4 vInstance;
layout (location = 7) in uint vVertexGroup; // use to index into matrices list

layout (location = 8) uniform usamplerBuffer u_groupIndexing; // index by VertexGroup
layout (location = 9) uniform samplerBuffer u_nodeMatrices; // index from groupIndexing
layout (location = 10) uniform int u_nodeCount;

layout (location = 11) in float layer_alpha;
layout (location = 12) in vec3 geoset_color;
layout (location = 0) uniform mat4 VP;

out vec2 UV;
out vec3 Normal;
out vec4 vertexColor;

mat4 fetchMatrix(int nodeIndex) {
	return mat4(
		texelFetch(u_nodeMatrices, int(gl_InstanceID * u_nodeCount * 4 + nodeIndex * 4)),
		texelFetch(u_nodeMatrices, int(gl_InstanceID * u_nodeCount * 4 + nodeIndex * 4 + 1)),
		texelFetch(u_nodeMatrices, int(gl_InstanceID * u_nodeCount * 4 + nodeIndex * 4 + 2)),
		texelFetch(u_nodeMatrices, int(gl_InstanceID * u_nodeCount * 4 + nodeIndex * 4 + 3)));
}

void main() {
	uvec4 groupIndexData = texelFetch(u_groupIndexing, int(vVertexGroup));
	uint boneNumber = groupIndexData[0];
	vec3 position = vPosition;
    vec4 p = vec4(position, 1);
	
	if (boneNumber > 0) {
		mat4 b0 = fetchMatrix(int(groupIndexData[1]));
		mat4 b1 = fetchMatrix(int(groupIndexData[2]));
		mat4 b2 = fetchMatrix(int(groupIndexData[3]));
		// TODO handle N size groupIndexData, like war3, instead of size 3
		
		position = vec3(b0 * p + b1 * p + b2 * p) / float(boneNumber);
		//compute p again now after position is updated:
		p = vec4(position, 1.f);
	}


	gl_Position = VP * vInstance * p;
	UV = vUV;
	
	vertexColor = vec4(geoset_color, layer_alpha);
	if(vertexColor.a <= 0.75) {
		gl_Position = vec4(0);
	}
}