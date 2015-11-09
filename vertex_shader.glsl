#version 330

in vec3 in_position;
in vec3 in_normal;

out vec4 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main(void) {
  normal = model * vec4(in_normal, 0.0);

  gl_Position = projection * view * model * vec4(in_position, 1.0);
}
