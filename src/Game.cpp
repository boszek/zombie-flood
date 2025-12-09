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
  m_maskButton = {290, 10, 60, 50};
}

Game::~Game() {}

bool Game::init(const char *title, int xpos, int ypos, int width, int height,
                bool fullscreen) {
  int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN |
              SDL_WINDOW_RESIZABLE; // Ensure SHOWN is set
  if (fullscreen) {
    flags |= SDL_WINDOW_FULLSCREEN;
  }

  if (SDL_Init(SDL_INIT_VIDEO) == 0) {
    std::cout << "Subsystems Initialised!..." << std::endl;

    // Init SDL_Image
    // SDL_GL_SetAttribute... codes
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    m_window = SDL_CreateWindow(title, xpos, ypos, width, height, flags);
    if (m_window) {
      std::cout << "Window created!" << std::endl;
      m_glContext = SDL_GL_CreateContext(m_window);
      if (m_glContext) {
        std::cout << "OpenGL Context created!" << std::endl;
      } else {
        return false;
      }

      // Initialize SDL_image
      int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
      if (!(IMG_Init(imgFlags) & imgFlags)) {
        printf("SDL_image could not initialize! SDL_image Error: %s\n",
               IMG_GetError());
        return false;
      }

      // Setup Projection
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glOrtho(0, width, height, 0, -1, 1); // 0,0 at top-left
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();

      // Initialize Map
      m_map = new Map(width, height);
      if (!m_map->load("assets/map.jpg", "assets/mask.png")) {
        printf("Failed to load map assets!\n");
        return false;
      }

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

        // UI Interaction
        if (mx >= m_speedBar.x && mx <= m_speedBar.x + m_speedBar.w &&
            my >= m_speedBar.y && my <= m_speedBar.y + m_speedBar.h) {
          float ratio = (float)(mx - m_speedBar.x) / m_speedBar.w;
          m_simSpeed = ratio * 2.0f; // Max speed 2x
        } else if (mx >= m_sizeBar.x && mx <= m_sizeBar.x + m_sizeBar.w &&
                   my >= m_sizeBar.y && my <= m_sizeBar.y + m_sizeBar.h) {
          float ratio = (float)(mx - m_sizeBar.x) / m_sizeBar.w;
          m_zombieSize = 1.0f + ratio * 2.5f; // 1.0 to 3.5
        } else if (mx >= m_maskButton.x &&
                   mx <= m_maskButton.x + m_maskButton.w &&
                   my >= m_maskButton.y &&
                   my <= m_maskButton.y + m_maskButton.h) {
          m_showMask = !m_showMask;
        } else if (mx >= m_pauseButton.x &&
                   mx <= m_pauseButton.x + m_pauseButton.w &&
                   my >= m_pauseButton.y &&
                   my <= m_pauseButton.y + m_pauseButton.h) {
          m_paused = !m_paused;
        } else {
          // Start Dragging
          m_dragging = true;
          m_dragStartX = mx;
          m_dragStartY = my;
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
        float dx = (event.motion.x - m_dragStartX) / m_zoom;
        float dy = (event.motion.y - m_dragStartY) / m_zoom;

        m_offsetX -= dx;
        m_offsetY -= dy;

        m_dragStartX = event.motion.x;
        m_dragStartY = event.motion.y;
      }
      break;
    case SDL_WINDOWEVENT:
      if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
        int w = event.window.data1;
        int h = event.window.data2;
        glViewport(0, 0, w, h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, w, h, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
      }
      break;
    case SDL_MOUSEWHEEL:
      int mx, my;
      SDL_GetMouseState(&mx, &my);
      float oldZoom = m_zoom;

      if (event.wheel.y > 0) {
        m_zoom *= 1.1f;
      } else if (event.wheel.y < 0) {
        m_zoom /= 1.1f;
      }

      // Zoom towards mouse cursor:
      // Offset += MouseWorldOld - MouseWorldNew
      // MouseWorld = MouseScreen / Zoom
      // Offset += mx / oldZoom - mx / newZoom
      m_offsetX += (float)mx / oldZoom - (float)mx / m_zoom;
      m_offsetY += (float)my / oldZoom - (float)my / m_zoom;
      break;
    }
  }
}

void Game::update() {
  float dt = 1.0f / 60.0f;
  if (m_simulation && !m_paused) {
    m_simulation->setZombieSize(m_zombieSize);
    m_simulation->update(dt * m_simSpeed);
  }
}

void Game::render() {
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glLoadIdentity();

  // Apply Camera Transform
  glScalef(m_zoom, m_zoom, 1.0f);
  glTranslatef(-m_offsetX, -m_offsetY, 0.0f);

  if (m_map) {
    m_map->render();
    if (m_showMask) {
      m_map->renderMask();
    }
  }

  if (m_simulation)
    m_simulation->render();

  // Render UI Overlay
  glLoadIdentity(); // Reset view for UI

  // Set up a proper Orthographic projection for UI.
  // We need to match the current window size logic
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

  // Mask Button
  if (m_showMask)
    glColor3f(0.5f, 0.5f, 1.0f); // Blueish active
  else
    glColor3f(0.3f, 0.3f, 0.3f); // Dark Grey inactive

  glRectf(m_maskButton.x, m_maskButton.y, m_maskButton.x + m_maskButton.w,
          m_maskButton.y + m_maskButton.h);

  // Restore Projection
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW); // Ensure we leave in ModelView mode

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
