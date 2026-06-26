#pragma once

struct GLFWwindow;

// Deklaracje funkcji obslugujacych zdarzenia (tzw. callbacks) z okna GLFW.
// Zostana one podpiete do okna w glownej funkcji programu.

// Wywolywana przy kazdym ruchu myszy. Odpowiada za obrot kamery.
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

// Wywolywana przy wcisnieciu lub zwolnieniu klawisza na klawiaturze.
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

// Wywolywana za kazdym razem, gdy uzytkownik zmieni rozmiar okna.
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

// Wywolywana przy kliknieciu przycisku myszy (np. lewy przycisk do raycastingu).
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);