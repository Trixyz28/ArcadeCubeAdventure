#version 450
#extension GL_ARB_separate_shader_objects : enable
layout(set = 1, binding = 0) uniform LightUniformBufferObject {
	mat4 mvpMat;
	mat4 mMat;
	mat4 nMat;
	float id;
} ubo;
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNorm;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNorm;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out float fragId;

void main() {
	gl_Position = ubo.mvpMat * vec4(inPosition, 1.0);
	fragPos = (ubo.mMat * vec4(inPosition, 1.0)).xyz;
	fragNorm = mat3(ubo.nMat) * inNorm;
	fragTexCoord = inUV;
	fragId = ubo.id;
}