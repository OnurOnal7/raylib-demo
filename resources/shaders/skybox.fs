#version 330 core

layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 brightColor;

in vec3 texDir;

uniform samplerCube cubemap;

void main() {
    vec4 texColor = texture(cubemap, normalize(texDir));
    fragColor = pow(texColor, vec4(2.2)); // gamma correction
    brightColor = vec4(0.0, 0.0, 0.0, 0.0);
}
