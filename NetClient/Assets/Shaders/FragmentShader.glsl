#version 330 core
out vec4 FragColor;
in vec3 fragPos;  // ������� ������� ��������� �  NDC (-1 -> 1)


//in vec2 TexCoord;
//uniform sampler2D texture0;
uniform vec4 color;
uniform int symbol;
uniform float radius;
uniform vec2 direction;
vec4 mainColor;



void main()
{
 vec4 finalColor = color; // �������� uniform
if (symbol ==  35){ // #

float border =0.01f;

if(fragPos.x <= border || fragPos.x >= 1 - border ||fragPos.y >= 1 - border || fragPos.y <= border ){ 
mainColor = vec4(1,1,1,1);
}else
mainColor = color;  // background collor

}else if(symbol == 36) // $ 
{
	 vec2 center = vec2(0.5, 0.5);
        vec2 delta = fragPos.xy - center;

        float dist2 = dot(delta, delta); // dx^2 + dy^2

        if( dist2 < radius * radius){

        if (dist2 > radius * radius - 0.1)
        {
            mainColor = vec4(1.0, 1.0, 1.0, 1.0); // ������
        }
        else
        {
            mainColor = vec4(0.0, 0.0, 0.0, 0.0); // ���������� ��� (��� ������ ����)
        }

        float dirFrag = dot(normalize(direction), normalize(delta)); // ������� ���� �����

        if (dirFrag > 0.99)
        {
            mainColor = vec4(0.0, 1.0, 1.0, 1.0); // ��������� �� �����������
        }
        }

        finalColor = mainColor;

}

finalColor = mainColor;

//vec4 texColor = vec4(texture(texture0,TexCoord));
FragColor = finalColor;
}
