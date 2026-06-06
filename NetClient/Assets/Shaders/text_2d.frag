#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D fontAtlas;
uniform sampler2D fogTex;
uniform vec3 color;

uniform bool WorldSpace;
uniform vec2 charTilePos;
uniform vec2 levelSize;

void main()
{
 if (WorldSpace) {
        // 1. Переводим координаты тайла в UV (0.0 - 1.0)
        // ВАЖНО: если туман инвертирован по Y, используй: 1.0 - (charTilePos.y / levelSize.y)
        vec2 fogUV = charTilePos / levelSize;
        
        // 2. Берем видимость ТОЛЬКО в точке нахождения персонажа
        float visibility = texture(fogTex, fogUV).r;

        // 3. Если точка скрыта туманом — убираем ВЕСЬ текст ника
        if (visibility < 0.5) { // 0.1 чтобы ник не мерцал на границе
            discard;
        }
    }

    float alpha = texture(fontAtlas, TexCoord).r;

     FragColor = vec4(color, alpha);
  //   FragColor = vec4(1, 1, 1, 1);
}