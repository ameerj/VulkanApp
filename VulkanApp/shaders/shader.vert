#version 450

// in from vulkan vertex input
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;
layout(location = 2) in vec2 tex;

layout (set = 0, binding = 0) uniform UboViewProjection {
	mat4 projection;
	mat4 view;
} ubo_view_projection;

layout(push_constant) uniform PushModel{
	mat4 model;
} push_model;

// No longer in use
//
//layout (binding = 1) uniform UboModel {
//	mat4 model;
//} ubo_model;
//

layout(location = 0) out vec3 fragCol;
layout(location = 1) out vec2 fragTex;

void main() {
	gl_Position = ubo_view_projection.projection * ubo_view_projection.view * push_model.model * vec4(pos, 1.0);
	fragCol = col;
	fragTex = tex;
}