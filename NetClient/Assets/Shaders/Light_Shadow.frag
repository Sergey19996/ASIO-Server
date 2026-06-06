#version 330 core

uniform sampler2D levelBlockTex;
uniform float u_dayCycle;
uniform vec2 levelSizeTiles; // 50 на 50 тайлов

in vec2 TexCoord;
out vec4 FragColor;

void main() {
    float angle = u_dayCycle * 6.28318;
    
    // Определяем, день сейчас или ночь
    bool isNight = (u_dayCycle > 0.5);
    
    // Вычисляем базовый вектор направления на светило
    vec2 dir = vec2(cos(angle), sin(angle));

    // ИНВЕРСИЯ: Если ночь, разворачиваем вектор на 180 градусов для луны
    if (isNight) {
        dir = -dir; 
    }

    // Высота текущего активного светила над горизонтом
    float height = abs(sin(angle));
    float safeHeight = max(height, 0.2); // Ограничение длины тени, чтобы они не были бесконечными на закате
    
    // Переводим текущие координаты фрагмента в пространство тайлов (0..50)
    vec2 currentPos = TexCoord * levelSizeTiles; 
    float shadow = 1.0;

    // --- ИСПРАВЛЕНИЕ 1: ПРОВЕРКА САМОГО СЕБЯ НА ТВЕРДОСТЬ ---
    // Если текущий пиксель САМ находится внутри стены, мы досрочно выходим из расчета.
    // Твердые блоки не должны бросать тени внутрь себя или друг на друга.
    if (texture(levelBlockTex, TexCoord).r > 0.5) {
        // Стены всегда освещены глобальным светом, их яркость регулируется только днем/ночью
        FragColor = vec4(1.0, isNight ? 1.0 : 0.0, 0.0, 1.0);
        return;
    }

    // Считаем тени для пустых клеток пола, если светило достаточно высоко
    if (height > 0.05) {
        // --- ИСПРАВЛЕНИЕ 2: ИНВЕРСИЯ НАПРАВЛЕНИЯ ШАГА Луча ---
        // Чтобы тень падала ОТ солнца, мы используем (-dir). 
        // Деление на safeHeight заставляет тени становиться длиннее, когда солнце близко к горизонту
        vec2 rayStep = (-dir / safeHeight) * 0.06; 

        for(int i = 1; i < 15; i++) {
            vec2 checkPos = currentPos + rayStep * float(i);
            vec2 uv = checkPos / levelSizeTiles;

            // Если луч вышел за границы карты — тень упасть не может
            if(uv.x < 0 || uv.y < 0 || uv.x > 1 || uv.y > 1) break;

            // Если луч по пути от пустой клетки в сторону солнца наткнулся на стену
            if(texture(levelBlockTex, uv).r > 0.5) {
                // Днем тени плотнее (0.4), ночью — едва заметные (0.8)
                float strength = isNight ? 0.8 : 0.4;
                shadow = strength; 
                break;
            }
        }
    }

    // Результат: R - тень, G - фактор ночи (для окрашивания в синий в другом шейдере)
    FragColor = vec4(shadow, isNight ? 1.0 : 0.0, 0.0, 1.0);
}