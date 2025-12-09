#include "Game.hpp"
#include <SDL_image.h>
#include <SDL_opengl.h>

Game::Game()
    : m_isRunning(false), m_window(nullptr), m_glContext(nullptr),
      m_map(nullptr), m_zoom(2.0f), m_offsetX(0.0f), m_offsetY(0.0f),
      m_simulation(nullptr), m_paused(false), m_dragging(false),
      m_simSpeed(1.0f), m_zombieSize(1.0f) {
  // Initialize UI layout
  m_speedBar = {10, 10, 200, 20};
  m_sizeBar = {10, 40, 200, 20};
  m_pauseButton = {220, 10, 60, 50};
}

Game::~Game() {}

bool Game::init(const char *title, int xpos, int ypos, int width, int height,
                bool fullscreen) {
  int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN; // Ensure SHOWN is set
  if (fullscreen) {
    flags |= SDL_WINDOW_FULLSCREEN;
  }

  if (SDL_Init(SDL_INIT_VIDEO) == 0) {
    std::cout << "Subsystems Initialised!..." << std::endl;

    // Init SDL_Image
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
      std::cout << "SDL_image Init Failed: " << IMG_GetError() << std::endl;
      // Continue without texture? Or fail?
      // Let's continue, textureID will remain 0.
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    m_window = SDL_CreateWindow(title, xpos, ypos, width, height, flags);
    if (m_window) {
      std::cout << "Window created!" << std::endl;
      m_glContext = SDL_GL_CreateContext(m_window);
      if (m_glContext) {
        std::cout << "OpenGL Context created!" << std::endl;
      } else {
        return false;
      }

      // Setup Projection
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glOrtho(0, width, height, 0, -1, 1); // 0,0 at top-left
      glMatrixMode(GL_MODELVIEW);

      // Initialize Map
      m_map = new Map(width, height);
      m_map->generate();

      m_simulation = new Simulation(m_map);

      // Load Texture
      SDL_Surface *surface = IMG_Load("assets/zombie1.png");
      if (surface) {
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        GLenum format =
            (surface->format->BytesPerPixel == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0,
                     format, GL_UNSIGNED_BYTE, surface->pixels);
        SDL_FreeSurface(surface);

        m_simulation->setTexture(tex);
        std::cout << "Texture loaded successfully!" << std::endl;
      } else {
        std::cout << "Failed to load zombie texture: " << IMG_GetError()
                  << std::endl;
      }

      // Initialize Simulation
      m_simulation->init(5000); // 5000 zombies

      m_isRunning = true;
    } else {
      return false;
    }

    return true;
  }
  return false;
}

void Game::handleEvents() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT:
      m_isRunning = false;
      break;
    case SDL_KEYDOWN:
      switch (event.key.keysym.sym) {
      case SDLK_ESCAPE:
        m_isRunning = false;
        break;
      case SDLK_SPACE:
        m_paused = !m_paused;
        break;
      case SDLK_LEFT:
        m_offsetX += 0.1f / m_zoom;
        break;
      case SDLK_RIGHT:
        m_offsetX -= 0.1f / m_zoom;
        break;
      case SDLK_UP:
        m_offsetY -= 0.1f / m_zoom;
        break;
      case SDLK_DOWN:
        m_offsetY += 0.1f / m_zoom;
        break;
      case SDLK_EQUALS: // +
      case SDLK_PLUS:
        m_zoom *= 1.1f;
        break;
      case SDLK_MINUS:
        m_zoom /= 1.1f;
        break;
      }
      break;
    case SDL_MOUSEBUTTONDOWN:
      if (event.button.button == SDL_BUTTON_LEFT) {
        int mx = event.button.x;
        int my = event.button.y;

        // Check UI
        if (mx >= m_speedBar.x && mx <= m_speedBar.x + m_speedBar.w &&
            my >= m_speedBar.y && my <= m_speedBar.y + m_speedBar.h) {
          m_simSpeed =
              (float)(mx - m_speedBar.x) / m_speedBar.w * 5.0f; // Max 5x speed
          char title[64];
          snprintf(title, 64, "Zombie Flood - Speed: %.1f, Size: %.1f",
                   m_simSpeed, m_zombieSize);
          SDL_SetWindowTitle(m_window, title);
        } else if (mx >= m_sizeBar.x && mx <= m_sizeBar.x + m_sizeBar.w &&
                   my >= m_sizeBar.y && my <= m_sizeBar.y + m_sizeBar.h) {
          m_zombieSize = (float)(mx - m_sizeBar.x) / m_sizeBar.w * 3.0f +
                         0.5f; // Size 0.5 to 3.5
          m_simulation->setZombieSize(m_zombieSize);

          char title[64];
          snprintf(title, 64, "Zombie Flood - Speed: %.1f, Size: %.1f",
                   m_simSpeed, m_zombieSize);
          SDL_SetWindowTitle(m_window, title);
        } else if (mx >= m_pauseButton.x &&
                   mx <= m_pauseButton.x + m_pauseButton.w &&
                   my >= m_pauseButton.y &&
                   my <= m_pauseButton.y + m_pauseButton.h) {
          m_paused = !m_paused;
        } else {
          // Start dragging map
          m_dragging = true;
          m_lastMouseX = mx;
          m_lastMouseY = my;
        }
      }
      break;
    case SDL_MOUSEBUTTONUP:
      if (event.button.button == SDL_BUTTON_LEFT) {
        m_dragging = false;
      }
      break;
    case SDL_MOUSEMOTION:
      if (m_dragging) {
        float dx = (event.motion.x - m_lastMouseX) / m_zoom;
        float dy = (event.motion.y - m_lastMouseY) / m_zoom;
        // Invert Y? OpenGL Y is up, Screen Y is down.
        // Translating scene: +X moves scene right, -Y moves scene down (if Y
        // up). Dragging mouse Right (+X) should move camera Left (scene
        // Right) to pan? Usually dragging world: move mouse Right -> World
        // moves Right. translate(offsetX, offsetY) moves the world. Mouse
        // Right (+dx)
        // -> offsetX += dx. Mouse Down (+dy) -> offsetY -= dy (since screen Y
        // is down, world Y is up).

        m_offsetX += dx;
        m_offsetY += dy;

        m_lastMouseX = event.motion.x;
        m_lastMouseY = event.motion.y;
      } else if (event.motion.state & SDL_BUTTON_LMASK) {
        // Dragging on slider
        int mx = event.motion.x;
        int my = event.motion.y;
        if (mx >= m_speedBar.x && mx <= m_speedBar.x + m_speedBar.w &&
            my >= m_speedBar.y && my <= m_speedBar.y + m_speedBar.h) {
          m_simSpeed = (float)(mx - m_speedBar.x) / m_speedBar.w * 5.0f;
          char title[64];
          snprintf(title, 64, "Zombie Flood - Speed: %.1f, Size: %.1f",
                   m_simSpeed, m_zombieSize);
          SDL_SetWindowTitle(m_window, title);
        } else if (mx >= m_sizeBar.x && mx <= m_sizeBar.x + m_sizeBar.w &&
                   my >= m_sizeBar.y && my <= m_sizeBar.y + m_sizeBar.h) {
          m_zombieSize = (float)(mx - m_sizeBar.x) / m_sizeBar.w * 3.0f + 0.5f;
          m_simulation->setZombieSize(m_zombieSize);
          char title[64];
          snprintf(title, 64, "Zombie Flood - Speed: %.1f, Size: %.1f",
                   m_simSpeed, m_zombieSize);
          SDL_SetWindowTitle(m_window, title);
        }
      }
      break;
    case SDL_MOUSEWHEEL: {
      int mx, my;
      SDL_GetMouseState(&mx, &my);
      float oldZoom = m_zoom;

      if (event.wheel.y > 0) {
        m_zoom *= 1.1f;
      } else if (event.wheel.y < 0) {
        m_zoom /= 1.1f;
      }

      // Zoom towards mouse cursor:
      // NewOffset = OldOffset + MouseX / NewZoom - MouseX / OldZoom
      m_offsetX = m_offsetX + (float)mx / m_zoom - (float)mx / oldZoom;
      m_offsetY = m_offsetY + (float)my / m_zoom - (float)my / oldZoom;
      break;
    }
    }
  }
}

void Game::update() {
  float dt = 1.0f / 60.0f;
  if (m_simulation && !m_paused) {
    // Apply speed multiplier
    // Assuming simulation update takes regular dt, we can't just pass dt *
    // speed if physics is discrete steps But for flow field simple movement,
    // dt scaling is fine.
    m_simulation->update(dt * m_simSpeed);
  }
  // Update m_zombieSize in simulation? Or just pass it during render?
  // User requested "control bars that let me modify... size of zombies".
  // Visual size or collision size?
  // "display the zombie as a sprite", usually visual size.
  // I'll pass it to render.
}

void Game::render() {
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glLoadIdentity();
  glScalef(m_zoom, m_zoom, 1.0f);
  glTranslatef(m_offsetX, m_offsetY, 0.0f);

  if (m_map) {
    m_map->render();
  }
  if (m_simulation) {
    m_simulation->render();
  }

  // Render UI Overlay
  glLoadIdentity(); // Reset view for UI
  // Default Ortho 0..width, 0..height is not set up, we are using default
  // -1..1 or similar? Wait, I never set up projection matrix properly in init
  // or render! The map rendering uses glScalef/glTranslatef which implies we
  // are working in some coordinate system. The map renders from -1..1 in
  // Simulation::render, but Map::render uses 0..width/height? Let's check
  // Map::render logic again. Map::render uses glRectf(x, y, x+1, y+1) for
  // each cell. So it renders in 0..width, 0..height space. Simulation::render
  // transforms to -1..1. This is inconsistent! I need to fix coordinate
  // systems.

  // Let's set up a proper Orthographic projection for UI.
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  glOrtho(0, viewport[2], viewport[3], 0, -1, 1); // Top-left origin
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Speed Bar
  glColor3f(0.5f, 0.5f, 0.5f);
  glRectf(m_speedBar.x, m_speedBar.y, m_speedBar.x + m_speedBar.w,
          m_speedBar.y + m_speedBar.h);
  glColor3f(1.0f, 1.0f, 1.0f);
  float speedFill = (m_simSpeed / 5.0f) * m_speedBar.w;
  glRectf(m_speedBar.x, m_speedBar.y, m_speedBar.x + speedFill,
          m_speedBar.y + m_speedBar.h);

  // Size Bar
  glColor3f(0.5f, 0.5f, 0.5f);
  glRectf(m_sizeBar.x, m_sizeBar.y, m_sizeBar.x + m_sizeBar.w,
          m_sizeBar.y + m_sizeBar.h);
  glColor3f(1.0f, 1.0f, 1.0f);
  float sizeFill = ((m_zombieSize - 0.5f) / 3.0f) * m_sizeBar.w;
  glRectf(m_sizeBar.x, m_sizeBar.y, m_sizeBar.x + sizeFill,
          m_sizeBar.y + m_sizeBar.h);

  // Pause Button
  if (m_paused)
    glColor3f(1.0f, 0.0f, 0.0f); // Red if paused
  else
    glColor3f(0.0f, 1.0f, 0.0f); // Green if running
  glRectf(m_pauseButton.x, m_pauseButton.y, m_pauseButton.x + m_pauseButton.w,
          m_pauseButton.y + m_pauseButton.h);

  // Restore Projection
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);

  SDL_GL_SwapWindow(m_window);
}

void Game::clean() {
  if (m_simulation) {
    delete m_simulation;
    m_simulation = nullptr;
  }
  if (m_map) {
    delete m_map;
    m_map = nullptr;
  }
  SDL_GL_DeleteContext(m_glContext);
  SDL_DestroyWindow(m_window);
  SDL_Quit();
  std::cout << "Game Cleaned" << std::endl;
}
