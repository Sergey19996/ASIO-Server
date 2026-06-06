#version 330 core
layout (location = 0) in vec4 vertex; // x, y, u, v

out vec2 TexCoord;
uniform mat4 model;
uniform mat4 projection;

// Добавляем эти параметры
uniform vec2 uvOffset; 
uniform vec2 uvScale;

void main(){
    // Трансформируем стандартные 0-1 в нужный участок атласа
    TexCoord = vertex.zw * uvScale + uvOffset; 
    gl_Position = projection * model * vec4(vertex.xy, 0.0, 1.0);
}