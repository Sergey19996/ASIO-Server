#version 330 core
out float FragColor; // шеёдер вернет 0 - темно - 1 светло 


struct Light {
    vec2 pos;
    float radius;
};


uniform vec2 playerPos;          // Позиция игрока В МИРЕ (в пикселях)
uniform float playerAuraRadius;   // Маленький радиус ближнего света БЕЗ ТЕНЕЙ


uniform Light projectiles[16];
uniform int projectileCount;


uniform float softness;

uniform sampler2D blockTex; // 1 пиксель = 1 тайл
uniform vec2 tileSize;      // (32,32)
uniform vec2 levelSize;     // (levelW, levelH) - в 

// --- В начало шейдера к юниформам ---
struct Beacon {
    vec2 pos;
    float radius;
};
uniform Beacon beacons[8]; // Максимум 8 активных маяков
uniform int beaconCount;


// Выносим DDA в функцию, чтобы использовать для игрока и для снарядов
float getVisibility(vec2 lightPos, float lightRadius, vec2 tileWorldPos) {
    vec2 dir = tileWorldPos - lightPos; // в пиксеклях
    float dist = length(dir);  // в пикселях

    if (dist > lightRadius) return 0.0;

    vec2 rayDir = normalize(dir / tileSize); // перевёл в тайлы мира
    vec2 rayStart = lightPos / tileSize; // / тайлы
    vec2 mapPos = floor(rayStart);  // округлил в тайлы по низк 
    vec2 deltaDist = abs(1.0 / rayDir); // Шаг в тайлах 
    vec2 step;
    vec2 sideDist;
     vec2 startTilePos = rayStart;

    if (rayDir.x < 0) {
        step.x = -1;
        sideDist.x = (startTilePos.x - mapPos.x) * deltaDist.x;
    } else {
        step.x = 1;
        sideDist.x = (mapPos.x + 1.0 - startTilePos.x) * deltaDist.x;
    }

    if (rayDir.y < 0) {
        step.y = -1;
        sideDist.y = (startTilePos.y - mapPos.y) * deltaDist.y;
    } else {
        step.y = 1;
        sideDist.y = (mapPos.y + 1.0 - startTilePos.y) * deltaDist.y;
    }

    float shadow = 1.0;
    for (int i = 0; i < 64; i++) {
        float t; // в тайлах 
        if (sideDist.x < sideDist.y) {
            t = sideDist.x;
            sideDist.x += deltaDist.x;
            mapPos.x += step.x;
        } else {
            t = sideDist.y;
            sideDist.y += deltaDist.y;
            mapPos.y += step.y;
        }

        if (mapPos.x < 0 || mapPos.y < 0 || mapPos.x >= levelSize.x || mapPos.y >= levelSize.y) break;

        if (texture(blockTex, (mapPos + 0.5) / levelSize).r > 0.5) {
            float wallDistPx = t * tileSize.x;
            float depth = dist - wallDistPx;
            shadow = 1.0 - smoothstep(0.0, 12.0, depth); // 12.0 - граница полутени за стеной
            break;
        }
        if (t * tileSize.x >= dist) break;
    }

    float radiusFade = 1.0 - smoothstep(lightRadius - softness, lightRadius, dist);
    return shadow * radiusFade;
}
void main()
{
      // Сначала проверяем игрока
 // Точная мировая float-позиция пикселя с учётом даунсэмплинга маски
    vec2 tileWorldPos = (gl_FragCoord.xy / 4.0) * tileSize; 

    // Базовое значение освещённости
    float result = 0.0f;

    // =========================================================================
    // 1. НОВАЯ АУРА ИГРОКА (БЕЗ ТЕНЕЙ — РАБОТАЕТ КАК МАЯК)
    // =========================================================================
    float dPlayer = distance(tileWorldPos, playerPos);
    if (dPlayer < playerAuraRadius) {
        // Мягкое радиальное затухание от центра игрока к краям ауры
        float playerAura = 1.0 - smoothstep(playerAuraRadius - softness, playerAuraRadius, dPlayer);
        result = max(result, playerAura);
    }

//
    // 2. Свет от снарядов (выбираем максимальную яркость)
    for (int i = 0; i < projectileCount; i++) {
        float pLight = getVisibility(projectiles[i].pos, projectiles[i].radius, tileWorldPos);
        result = max(result, pLight);
    }
     // 3. Свет от МАЯКОВ (БЕЗ ТЕНЕЙ - просто градиент)
    for (int i = 0; i < beaconCount; i++) {
        float d = distance(tileWorldPos, beacons[i].pos);
        if (d < beacons[i].radius) {
            // Мягкое затухание к краям
            float bLight = 1.0 - smoothstep(beacons[i].radius - softness, beacons[i].radius, d);
            result = max(result, bLight);
        }
    }
    
    FragColor = result;
}