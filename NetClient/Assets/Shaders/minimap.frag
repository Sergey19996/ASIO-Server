#version 330 core
out vec4 FragColor;
uniform sampler2D memoryTex; // Старая память (пройденные места)
uniform sampler2D currentFogTex; // Текущий свет
uniform vec2 playerPos; // Нормализованные (0-1)
uniform vec2 beacons[8]; 

uniform vec2 enemies[16]; // Добавляем врагов
uniform int enemyCount;
uniform int beaconCount;
in vec2 TexCoord;



void main() {
      // Инвертируем Y для корректного отображения
    vec2 uv = vec2(TexCoord.x, 1.0 - TexCoord.y);
     // 1. Рисуем золотую обводку (квадратную)
    bool isBorder = (TexCoord.x < 0.02 || TexCoord.x > 1.0 - 0.02 || 
                     TexCoord.y < 0.02 || TexCoord.y > 1.0 - 0.02);
    
    if(isBorder) {
        FragColor = vec4(0.8, 0.6, 0.1, 1.0); // Золотой цвет
        return;
    }

    float mem = texture(memoryTex, uv).r; // берём с памяти либо 0 либо 1
    float current = texture(currentFogTex, uv).r; // берём с памяти либо 0 либо 1
    
    // Базовый цвет карты: исследованное — тусклое, текущее — яркое
    vec3 color = vec3(0.2) * mem;  // либо 0 либо 0.2
    color = max(color, vec3(0.5, 0.5, 0.6) * current); //тут в кругу current - мы будем 0.5 0.5 0.6

    // Рисуем игрока (маленькая белая точка)
    if(distance(uv, playerPos) < 0.015) color = vec3(1.0, 1.0, 1.0); // если дистанция маленькая между uv и игроком делаем 1 

    // Рисуем маяки (красные, если потушены / зеленые, если горят)
    for(int i = 0; i < beaconCount; i++) {
        if(distance(uv, beacons[i]) < 0.01) color = vec3(0.0, 1.0, 0.0);
    }
     // ВРАГИ (красные) - рисуем только если в этой точке есть свет (current > 0.1)
    for(int i = 0; i < enemyCount; i++) {
        if(distance(uv, enemies[i]) < 0.015) {
             float visibility = texture(currentFogTex, enemies[i]).r;
             if(visibility > 0.1) color = vec3(1.0, 0.0, 0.0);
        }
    }


    FragColor = vec4(color, 0.8); // 0.8 - прозрачность самой миникарты
}