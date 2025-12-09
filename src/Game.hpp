#pragma once
#include "Map.hpp"
#include "Simulation.hpp"
#include <SDL.h>
#include <iostream>

class Game {
public:
  Game();
  ~Game();

  bool init(const char *title, int xpos, int ypos, int width, int height,
            bool fullscreen);
  void handleEvents();
  void update();
  void render();
  void clean();

  bool running() { return m_isRunning; }

private:
  bool m_isRunning;
  SDL_Window *m_window;
  SDL_GLContext m_glContext;
  Map *m_map;

  // Camera
  float m_zoom;
  float m_offsetX;
  float m_offsetY;

  Simulation *m_simulation;

  // UI state
  bool m_paused;
  bool m_dragging;
  int m_lastMouseX, m_lastMouseY;

  // Controls
  float m_simSpeed;
  float m_zombieSize;

  // UI Rects (simple hardcoded for now)
  struct Rect {
    int x, y, w, h;
  };
  Rect m_speedBar;
  Rect m_sizeBar;
  Rect m_pauseButton;
};
