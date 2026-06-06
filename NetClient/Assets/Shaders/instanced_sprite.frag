#version 330 core

out vec4 FragColor; 

// Входящие данные из нового инстанс-вершинного шейдера
in vec2 LocalPos;      // Позиция фрагмента внутри квада (0 -> 1), бывший fragPos
in vec2 WorldPosOut;   // Мировые координаты пикселя на уровне для расчета тумана
in vec2 TexCoords;     // Готовые, уже рассчитанные в VS координаты атласа
in vec4 InstanceColor; // Индивидуальный цвет текущего спрайта (бывший uniform color)

// --- ТЕКСТУРЫ И СИСТЕМА АТЛАСА ---
uniform sampler2D u_textureAtlas; // Юнит GL_TEXTURE0
// u_uvOffset и u_uvScale больше НЕ НУЖНЫ, они приходят сразу в TexCoords!

// --- СИСТЕМНЫЕ ЮНИФОРМЫ ---
uniform int symbol; // Оставляем для совместимости: 0=атлас, 33=HP, 35=блок, 36=снаряд
uniform bool bFrame;
uniform vec2 quadSize; // Будет использоваться размер текущего инстанса

float radius = 0.5f;
uniform vec2 direction;

// --- ПАРАМЕТРЫ ОСВЕЩЕНИЯ И ТУМАНА ВОЙНЫ ---
uniform vec2 levelSizePx;
uniform sampler2D fogTex;
uniform sampler2D memoryTex; 
uniform vec2 charTilePos;
uniform bool ProjectileOwner;
uniform float u_time;
uniform sampler2D lightMapTex;
uniform float lightIntensity; 

// Цвета атмосферы
vec3 dayCol = vec3(1.0, 0.95, 0.8);
vec3 nightCol = vec3(0.26, 0.84, 0.81);

void main()
{
    // 1. Читаем данные освещения и тумана войны по мировым координатам
    vec2 fogUV = WorldPosOut / levelSizePx;
    vec4 lightData = texture(lightMapTex, fogUV); // Получаем R (shadow) и G (isNight)
    float shadow = lightData.r;
    bool isNight = lightData.g > 0.5;

    // 2. Базовая видимость (туман войны)
    float current = texture(fogTex, fogUV).r;
    float memory = texture(memoryTex, fogUV).r;
    float visibility = max(current, memory * 0.45); 

    if (visibility < 0.01) discard;

    // 3. Атмосфера
    vec3 ambient = mix(nightCol, dayCol, lightIntensity) * 0.6;
 
    // Сюда запишем итоговый цвет пикселя до применения света и тумана
    vec4 mainColor = vec4(1.0); 

    if (symbol == 0) // Стандарт: Рисуем текстуру из атласа
    {


        // Читаем пиксель по готовым UV координатам атласа из вершинного шейдера
        vec4 texColor = texture(u_textureAtlas, TexCoords);
        
        // Альфа-тест (выбрасываем прозрачные пиксели)
        if(texColor.a < 0.1) discard;

        // Применяем индивидуальный цвет инстанса (например, для теней или пульсаций)
        mainColor = texColor * InstanceColor; 
    }
    else if (symbol == 33) // Процедурный HP Bar
    { 
        vec2 levelSize = vec2(levelSizePx.x / 32.0, levelSizePx.y / 32.0);
        vec2 charFogUV = charTilePos / levelSize;
        float charVisibility = texture(fogTex, charFogUV).r;

        if (charVisibility < 0.5) { 
            discard;
        }
   
        float border = 1.00f;  
        vec2 px = LocalPos * quadSize; 
        if(px.x <= border || px.x >= quadSize.x - border ||
           px.y <= border || px.y >= quadSize.y - border)
        {
            mainColor = vec4(1.0, 0.84, 0.0, 1.0); // золотая рамка
        }
        else
        {
            mainColor = InstanceColor;  
        }
    }
    else if (symbol == 35) // Старый процедурный блок '#'
    { 
        mainColor = InstanceColor; 
        float border = 1.00f;  
        vec2 px = LocalPos * quadSize; 
        if(px.x <= border || px.x >= quadSize.x - border ||
           px.y <= border || px.y >= quadSize.y - border)
        {
            if(bFrame == true){
                mainColor = vec4(1.0, 0.84, 0.0, 1.0); 
            }
        }
        else
        {
            mainColor = InstanceColor;  
        }
    }
    else if (symbol == 36 || symbol == 64) // Снаряды
    {
        if(visibility < 0.5 && !ProjectileOwner) 
            discard;

        vec2 center = vec2(0.5, 0.5);
        vec2 delta = LocalPos - center;
        float dist = length(delta);

        if (dist < radius)
        {
            if (dist > radius - 0.1)
                mainColor = InstanceColor;  
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
        mainColor = InstanceColor;
    }

    // --- 4. ФИНАЛЬНЫЙ РАСЧЕТ ОСВЕЩЕНИЯ ---
    vec3 finalRGB = mainColor.rgb * (ambient + vec3(shadow));
    
    // Ограничиваем цвета, чтобы не было пересветов
    finalRGB = clamp(finalRGB, 0.0, 1.0);

    FragColor = vec4(finalRGB, mainColor.a) * visibility;
}