#include <stdio.h>
#include "nuGL.h"

int main(void) {
  nu_Window *window = nu_create_window(600, 600, NULL, NULL);
  while(!glfwWindowShouldClose(window->glfw_window)) {
    // Update input
    nu_update_input(window);
    // Rendering 
    nu_start_frame(window);
    nu_end_frame(window);
  }
  nu_destroy_window(&window);
  return 0;
}
