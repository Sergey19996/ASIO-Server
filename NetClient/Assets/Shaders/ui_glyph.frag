#version 330 core
out vec4 FragColor;
in vec2 fragPos;  // позиция каждого фрагмента в  NDC (0 -> 1)
// uniform: размер квадрата в пикселях
uniform vec2 quadSize; // {width, height}

//in vec2 TexCoord;
//uniform sampler2D texture0;
uniform vec4 color;
uniform bool u_useHorizon;
uniform int symbol;
vec4 mainColor;


void main()
{
mainColor = color;

   //  mainColor = color; // дефолт
if (symbol ==  35){ // #

float border =1.00f;  // толщина рамки в пикселях
vec2 px = fragPos.xy * quadSize; // переводим в пиксели 
if(px.x <= border || px.x >= quadSize.x - border ||
   px.y <= border || px.y >= quadSize.y - border)
{
    mainColor = vec4(1.0, 0.84, 0.0, 1.0); // золотая рамка
}else
    mainColor = color;  // background collor

}
if (symbol ==  0){
    vec2 center = vec2(0.5, 0.5);
    float aspectRatio = quadSize.x / quadSize.y;
   
    vec2 distVec = vec2((fragPos.x - center.x) * aspectRatio, fragPos.y - center.y);
    float dist = length(distVec);

    float radius = 0.48;
    float pixel = 1.5 / quadSize.y;
    if (dist > radius + pixel) discard; 
    if (dist < radius) {
      vec4 finalColor = color; 
        if(u_useHorizon){
        // 1. Разделение на День и Ночь (Горизонт)
        // Если uv.y > 0.5 — это верхняя полусфера (День), иначе — Ночь
        vec4 nightColor = vec4(0.02, 0.02, 0.1, 0.9); // Глубокая ночь
        // Плавный переход на линии горизонта (0.5)
        float horizonMask = smoothstep(0.49, 0.51, fragPos.y);
         finalColor = mix(nightColor, color, horizonMask);
        }

        // 2. Ободок
        float thickness = 0.03;
        float ring = smoothstep(radius, radius - pixel, dist) - 
                     smoothstep(radius - thickness, radius - thickness - pixel, dist);
        mainColor = mix(finalColor, vec4(1.0, 1.0, 1.0, 1.0), ring);
        
        mainColor.a *= smoothstep(radius, radius - pixel, dist);
    } else {
        discard;
    }

}
//vec4 texColor = vec4(texture(texture0,TexCoord));
FragColor = mainColor;
}
