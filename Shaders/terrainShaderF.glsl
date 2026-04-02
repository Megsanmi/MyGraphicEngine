#version 460

out vec4 FragColor;

uniform vec3 object_color = vec3(0.5,0.7,0.5);
uniform vec3 cameraPos;

void main()
{
    FragColor = vec4(object_color, 1.0);
}