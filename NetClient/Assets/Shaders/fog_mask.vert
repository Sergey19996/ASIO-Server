#version 330 core
layout (location = 0) in vec2 aPos;
out vec2 TexCoord;


void main()
{
 TexCoord = aPos;
 
 gl_Position = vec4( // переводим uv в ndc
    aPos.x * 2.0 - 1.0,
    aPos.y * 2.0 - 1.0,
    0.0,
    1.0
);
}