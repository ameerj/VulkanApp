#version 450
layout(input_attachment_index = 0, binding = 0) uniform subpassInput inputColor;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput inputDepth;

layout(location = 0) out vec4 color;

void main() {
	int xHalf = 1800/2;
	if (gl_FragCoord.x > xHalf) {
		float low_bound = 0.99;
		float up_bound = 1.0;
		
		float depth = subpassLoad(inputDepth).r;
		float depth_color_scaled =  1.0f - ((depth - low_bound) / (up_bound - low_bound));
		color = vec4(subpassLoad(inputColor).rgb * depth_color_scaled, 1.0f);
	}
	else {
		color = subpassLoad(inputColor).rgba;
	}
}