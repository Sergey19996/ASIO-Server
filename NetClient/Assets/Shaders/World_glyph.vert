#version 330 core
layout (location = 0) in vec4 vertex;

out vec2 fragPos;
out vec2 WorldPos;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;

// ѕередаем размер окна в пиксел€х (Width, Height) из C++
uniform vec2 u_screenSize; 

void main(){
    // 1. —читаем позицию в мире дл€ логики тумана
    vec4 worldCoord = model * vec4(vertex.xy, 0.0, 1.0);
    WorldPos = worldCoord.xy;
    
    fragPos = vertex.xy;
    TexCoord = vertex.zw;

    // 2. —читаем финальное положение на экране стандартным образом
    vec4 pos = projection * view * worldCoord;

    // 3. ћј√»я PIXEL-PERFECT: округл€ем позицию до физических пикселей монитора.
    // ѕереводим из пространства NDC (-1 до 1) в пиксели экрана, округл€ем и возвращаем обратно.
    // Ёто полностью уничтожает швы при любом OnResize и любом дробном zoom!
    pos.xy = (pos.xy * 0.5 + 0.5) * u_screenSize;
    pos.xy = round(pos.xy);
    pos.xy = (pos.xy / u_screenSize - 0.5) * 2.0 * pos.w;

    gl_Position = pos;
}