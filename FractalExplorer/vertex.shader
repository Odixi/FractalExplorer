#version 460 core

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;

out vec4 outColor; // output a color to the fragment shader

void main() {
	gl_Position = position;
	outColor = color;
};
