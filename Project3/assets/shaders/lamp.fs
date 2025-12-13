#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform vec3 lightColor;
uniform sampler2D lightTexture;

void main()
{
    vec4 texColor = texture(lightTexture, TexCoords);
    
    FragColor = texColor * vec4(lightColor, 1.0);
}