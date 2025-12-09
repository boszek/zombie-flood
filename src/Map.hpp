#pragma once
#include <SDL.h>
#include <vector>

class Map {
public:
  Map(int width, int height);
  ~Map();

  void generate();
  void render(SDL_Renderer *renderer); // Using SDL_Renderer for simplicity
                                       // initially, or OpenGL?
  // GAME.md said OpenGL, so maybe we should stick to OpenGL rendering or use
  // SDL_Renderer if we want to keep it simple first. The skeleton uses
  // SDL_Window with OpenGL flag, but I haven't set up OpenGL rendering pipeline
  // yet. To be safe and compliant with "Game uses C++, OpenGL and SDL2", I
  // should probably use OpenGL for rendering. However, fast iteration 1 might
  // benefit from SDL_Renderer if I didn't set up GL loader (like GLEW/GLAD)
  // yet? Wait, modern OpenGL needs a loader. Mac has OpenGL framework but usage
  // is deprecated. Let's stick to raw OpenGL 1.x/2.x (immediate mode) for
  // simplicity if we don't want to add GLAD/GLEW, OR just use SDL_Renderer
  // which uses OpenGL backend on Mac.

  // User requirement: "Game uses C++, OpenGL and SDL2".
  // "running in a window, displays a 2D map from bitmap".

  // Let's try to use SDL_Renderer for the "bitmap" part first as it abstracts
  // this nicely, unless the user strictly meant "Raw OpenGL calls". Given
  // "fluid simulation", raw OpenGL (points/particles) might be better later.
  // Let's implement a simple render function that we can fill in.

  // Actually, I'll define render() to take nothing for now, assuming it calls
  // GL functions.
  void render();

  int getWidth() const { return m_width; }
  int getHeight() const { return m_height; }

  const std::vector<unsigned char> &getData() const { return m_data; }

  // Flow field: 0-7 direction, 255 = blocked/center?
  // Actually, vector<pair<float, float>> might be better for smooth movement,
  // or just pair<int, int> Let's store pairs of floats representing normalized
  // direction vector.
  struct Vector2 {
    float x, y;
  };
  const std::vector<Vector2> &getFlowField() const { return m_flowField; }
  Vector2 getFlowAt(int x, int y) const;

private:
  int m_width;
  int m_height;
  std::vector<unsigned char> m_data; // 0 = empty, 1 = obstacle/wall
  std::vector<Vector2> m_flowField;

  void calculateFlowField();
};
