#version 330 core

// 1. Статические данные вершины квада (layout 0)
layout (location = 0) in vec4 vertex; // x, y = позиция вершины [0..1]; z, w = базовые UV [0..1]

// 2. Динамические данные инстанса из нашего нового класса
layout (location = 1) in vec2 a_position;
layout (location = 2) in vec2 a_size;
layout (location = 3) in float a_rotation;
layout (location = 4) in vec4 a_color;
layout (location = 5) in vec2 a_uvOffset;
layout (location = 6) in vec2 a_uvScale;

// Передаем во фрагментный шейдер
out vec2 TexCoords;
out vec4 InstanceColor;

// Глобальные матрицы (передаются из C++ один раз за кадр)
uniform mat4 projection; // Ортографическая проекция экрана
uniform mat4 view;       // Матрица трансформации камеры (скроллинг, зум)

// Добавляем выходы в вершинном шейдере:
out vec2 LocalPos;
out vec2 WorldPosOut;
// Передаем размер окна в пикселях (Width, Height) из C++
uniform vec2 u_screenSize; 
void main() {
    // Получаем базовые координаты вершины квадрата (0.0 или 1.0)
    vec2 pos = vertex.xy;
     LocalPos = pos; // Передаем локальные 0..1 координаты квада
   
    // --- СТРОИМ МАТРИЦУ ТРАНСФОРМАЦИИ ПРЯМО В ШЕЙДЕРЕ ---
    
    // 1. Масштабирование (Scale)
    pos *= a_size;


    // 2. Поворот вокруг центра (Rotation)
    if (a_rotation != 0.0) {
        float rad = radians(a_rotation);
        float s = sin(rad);
        float c = cos(rad);
        
        // Смещаем центр вращения в середину спрайта
        vec2 center = a_size * 0.5;
        pos -= center;
        
        // Матрица поворота 2D
        float x_new = pos.x * c - pos.y * s;
        float y_new = pos.x * s + pos.y * c;
        pos = vec2(x_new, y_new);
        
        // Возвращаем центр обратно
        pos += center;
    }

    // 3. Перемещение (Translation) в мировые координаты уровня
    pos += a_position;

    // Передаем честную мировую позицию для расчета Fog/Света во фрагментный шейдер
    WorldPosOut = pos; 

    // Вычисляем начальную позицию в пространстве клипинга (NDC)
    vec4 clipSpacePos = projection * view * vec4(pos, 0.0, 1.0);

    // --- МАГИЯ PIXEL-PERFECT ДЛЯ ИНСТАНСИНГА ---
    // Переводим из  .диапазона NDC (-1..1).          в физические пиксели экрана
    vec2 pixelPos = (clipSpacePos.xy / clipSpacePos.w * 0.5 + 0.5) * u_screenSize;
    
    // Округляем до ближайшего целого пикселя монитора
    pixelPos = round(pixelPos);
    
    // Возвращаем координаты обратно в пространство клипинга NDC
    clipSpacePos.xy = (pixelPos / u_screenSize - 0.5) * 2.0 * clipSpacePos.w;

    // Записываем итоговую позицию вершины
    gl_Position = clipSpacePos;

    // --- РАСЧЕТ ТЕКСТУРНЫХ КООРДИНАТ ДЛЯ АТЛАСА ---
    vec2 safeLocalUV = mix(vec2(0.01), vec2(0.99), vertex.zw);
    TexCoords = safeLocalUV * a_uvScale + a_uvOffset;

    InstanceColor = a_color;
}