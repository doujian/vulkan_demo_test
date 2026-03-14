#version 450

layout(location = 0) in vec2 fragTexCoord;

layout(set = 0, binding = 0) uniform sampler2D tex1;
layout(set = 0, binding = 1) uniform sampler2D tex2;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 color1 = texture(tex1, fragTexCoord);
    vec4 color2 = texture(tex2, fragTexCoord);
    
    outColor = mix(color1, color2, 0.5);
}