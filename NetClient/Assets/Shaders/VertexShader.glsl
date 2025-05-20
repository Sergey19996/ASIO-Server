#version 330 core
layout (location = 0) in vec4 vertex;
out vec3 fragPos;


//out vec2 TexCoord;


uniform mat4 model;
uniform mat4 projection;

//for bricks

uniform vec2 uv;
uniform vec2 spriteScale;
void main(){




gl_Position = projection * model * vec4(vertex.xy,0.0,1.0);
 
  // TexCoord = vec2(vertex.z * spriteScale.x + uv.x,vertex.w * spriteScale.y + uv.y );
   fragPos = vec3(vertex.xy,1.0);
  

}