#version 330

in vec4 normal;

out vec4 fragment_color;

uniform vec4 color;

void main(void) {
  vec4 light_position = vec4(5.0, 5.0, 0.0, 0.0);
  vec3 diffuse = vec3(0.5, 0.5, 0.5) * max(dot(normalize(normal), normalize(light_position)), 0.0);

  fragment_color = color + vec4(diffuse, 0.0);
}
