#version 460 core

layout(location = 0) out vec4 color;

uniform vec4 u_color;

in vec4 outColor;

void main() {
	color = outColor;
};