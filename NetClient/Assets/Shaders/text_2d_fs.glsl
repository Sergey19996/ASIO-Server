#version 330 core
out vec4 FragColor;
//in vec3 fragPos;  // позиция каждого фрагмента в  NDC (-1 -> 1)


in vec2 TexCoord;
uniform sampler2D text;
uniform vec3 color;


void main()
{



vec4 texColor = vec4(1.0,1.0,1.0,texture(text,TexCoord).r);  // мы делаем прозрачным каждый фрагмент неотносящейся к форме буквы
FragColor = vec4(texColor.rgb * color.rgb, texColor.a * 1.0);
}
