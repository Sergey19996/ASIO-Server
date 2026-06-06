#version 330 core


out vec4 FragColor; 


in vec2 fragPos;  // позиция каждого фрагмента в  NDC (0 -> 1)
in vec2 WorldPos; // Передано из вертексного шейдера
in vec2 TexCoord; // Получено из вершинного шейдера

// --- ЮНИФОРМЫ ДЛЯ СИСТЕМЫ АТЛАСА ---
uniform sampler2D u_textureAtlas; // Юнит GL_TEXTURE0 (Ваш Level_tiles)
uniform vec2 u_uvOffset;          // Сдвиг конкретного кадра тайла
uniform vec2 u_uvScale;           // Размер кадра в атласе (например, 0.0625)


// --- ВАШИ СТАРЫЕ ЮНИФОРМЫ ---
uniform int symbol; // // 0 = Рисуем текстуру, 33 = HP bar, 35 = Старый блок, 36 = Снаряд
uniform bool bFrame;
uniform vec2 quadSize;
uniform vec4 color; // Uniform-цвет (будет использоваться как Tint-фильтр для текстуры)


float radius = 0.5f;
uniform vec2 direction;


// --- НОВЫЕ ПАРАМЕТРЫ ---
uniform vec2 levelSizePx;
uniform sampler2D fogTex;
uniform sampler2D memoryTex; // Память (r)
uniform vec2 charTilePos;
uniform bool ProjectileOwner;
uniform float u_time;
// --------------------
uniform sampler2D lightMapTex;
uniform float lightIntensity; // (cos + 1) * 0.5

// Цвета атмосферы
vec3 dayCol = vec3(1.0, 0.95, 0.8);
vec3 nightCol = vec3(0.26, 0.84, 0.81);

void main()
{
       // --- ВАЖНО: Инициализируем mainColor сразу! ---
   
   // 1. Читаем данные освещения
    vec2 fogUV = WorldPos / levelSizePx;
    vec4 lightData = texture(lightMapTex, fogUV); // Получаем R (shadow) и G (isNight)
    float shadow = lightData.r;
    bool isNight = lightData.g > 0.5;

    // 2. Базовая видимость (туман войны)
    float current = texture(fogTex, fogUV).r;
    float memory = texture(memoryTex, fogUV).r;
    float visibility = max(current, memory * 0.45); // memory просто хранит единицы там где мы были 

    if (visibility < 0.01) discard;

    // 3. Атмосфера
    vec3 ambient = mix(nightCol, dayCol, lightIntensity) * 0.6;
 

  // Сюда запишем итоговый цвет пикселя до применения света и тумана
    vec4 mainColor = vec4(1.0); 
       if (symbol == 0) // Наш новый стандарт: 0 означает "Рисуем текстуру из атласа"
    {
        // Рассчитываем точные координаты пикселя внутри нужной ячейки атласа
        // Используем clamp, чтобы текстура "не заезжала" на соседние тайлы из-за фильтрации
           vec2 safeTexCoord = mix(vec2(0.01), vec2(0.99), TexCoord);

           // Рассчитываем итоговые UV по безопасным координатам
    vec2 atlasCoords = u_uvOffset + (safeTexCoord * u_uvScale);
        
        // Читаем пиксель из атласа тайлов
        vec4 texColor = texture(u_textureAtlas, atlasCoords);
        
        // Если пиксель полностью прозрачный (например, фон декорации/травы) - выбрасываем его
        if(texColor.a < 0.1) discard;

        // Умножаем цвет текстуры на цвет из uniform (чтобы можно было подкрашивать тайлы при необходимости)
        mainColor = texColor * color; 
    }
    else if (symbol == 33) // Процедурный HP Bar (Ваш код)
    { 
        vec2 levelSize = vec2(levelSizePx.x / 32.0, levelSizePx.y / 32.0);
        vec2 charFogUV = charTilePos / levelSize;
        float charVisibility = texture(fogTex, charFogUV).r;

        if (charVisibility < 0.5) { 
            discard;
        }
   
        float border = 1.00f;  
        vec2 px = fragPos.xy * quadSize; 
        if(px.x <= border || px.x >= quadSize.x - border ||
           px.y <= border || px.y >= quadSize.y - border)
        {
            mainColor = vec4(1.0, 0.84, 0.0, 1.0); // золотая рамка
        }
        else
        {
            mainColor = color;  
        }
    }
    else if (symbol == 35) // Старый процедурный блок '#' (Оставлен для совместимости)
    { 
        mainColor = color; 
        float border = 1.00f;  
        vec2 px = fragPos.xy * quadSize; 
        if(px.x <= border || px.x >= quadSize.x - border ||
           px.y <= border || px.y >= quadSize.y - border)
        {
            if(bFrame == true){
                mainColor = vec4(1.0, 0.84, 0.0, 1.0); 
            }
        }
        else
        {
            mainColor = color;  
        }
    }
    else if (symbol == 36 || symbol == 64) // Снаряды (Ваш код)
    {
        if(visibility < 0.5 && !ProjectileOwner) 
            discard;

        vec2 center = vec2(0.5, 0.5);
        vec2 delta = fragPos.xy - center;
        float dist = length(delta);

        if (dist < radius)
        {
            if (dist > radius - 0.1)
                mainColor = color;  
            else
                mainColor = vec4(0.0);

            float dirFrag = dot(normalize(direction), normalize(delta));

            if (dirFrag > 0.99)
            {
                if (symbol == 36)
                    mainColor = vec4(0.0, 1.0, 1.0, 1.0);  
            } 
        }
        else 
        {
            discard;
        }
    }
    else // Любые другие старые символы
    {
        mainColor = color;
    }

    // --- 3. ФИНАЛЬНЫЙ РАСЧЕТ ОСВЕЩЕНИЯ (Ваш оригинальный код) ---
    // Применяем тени, свет окружающей среды и умножаем на общую видимость (туман)
    vec3 finalRGB = mainColor.rgb * (ambient + vec3(shadow));
    
    // Ограничиваем цвета, чтобы не было пересветов
    finalRGB = clamp(finalRGB, 0.0, 1.0);

    FragColor = vec4(finalRGB, mainColor.a) * visibility;
    
    // 5. ИТОГОВЫЙ ВЫВОД
    // Глобальный свет * Тени
//    vec3 finalRGB = mainColor.rgb * shadow;
//    
//    // Подсветка от "фонарика" (видимой зоны)
//    float flashlightStrong = mix(0.6, 0.3, lightIntensity);
//    finalRGB += mainColor.rgb *ambient* current * flashlightStrong;

 //   FragColor = vec4(finalRGB, mainColor.a * visibility);
}
