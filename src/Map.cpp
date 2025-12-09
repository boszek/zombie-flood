#include "Map.hpp"
#include <SDL_image.h>
#include <SDL_opengl.h> // Include OpenGL header
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <queue> // For BFS

Map::Map(int width, int height) : m_width(width), m_height(height) {
  m_data.resize(m_width * m_height, 0);
  m_flowField.resize(m_width * m_height, {0.0f, 0.0f});
  std::srand(std::time(nullptr));
  calculateFlowField(); // Call calculateFlowField in constructor
}

Map::~Map() {}

bool Map::load(const char *bgFile, const char *maskFile) {
  // Load Background Texture
  SDL_Surface *bgSurf = IMG_Load(bgFile);
  if (!bgSurf) {
    printf("Failed to load map background: %s\n", IMG_GetError());
    return false;
  }

  // Set map dimensions from background image
  m_width = bgSurf->w;
  m_height = bgSurf->h;

  glGenTextures(1, &m_textureID);
  glBindTexture(GL_TEXTURE_2D, m_textureID);

  // Choose format based on bytes per pixel
  int mode = GL_RGB;
  if (bgSurf->format->BytesPerPixel == 4)
    mode = GL_RGBA;

  glTexImage2D(GL_TEXTURE_2D, 0, mode, bgSurf->w, bgSurf->h, 0, mode,
               GL_UNSIGNED_BYTE, bgSurf->pixels);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  SDL_FreeSurface(bgSurf);

  // Load Mask
  SDL_Surface *maskSurf = IMG_Load(maskFile);
  if (!maskSurf) {
    printf("Failed to load map mask: %s\n", IMG_GetError());
    return false;
  }

  // Resize data vectors
  m_data.resize(m_width * m_height);
  m_speedModifiers.resize(m_width * m_height);
  m_flowField.resize(m_width * m_height);

  // Parse Mask Pixels
  // Ensure we are reading correctly regardless of format.
  // For simplicity, let's assume 32-bit or 24-bit.
  // Better: use getPixel helper.

  SDL_LockSurface(maskSurf);

  auto getPixel = [&](int x, int y) -> Uint32 {
    int bpp = maskSurf->format->BytesPerPixel;
    Uint8 *p = (Uint8 *)maskSurf->pixels + y * maskSurf->pitch + x * bpp;
    switch (bpp) {
    case 1:
      return *p;
    case 2:
      return *(Uint16 *)p;
    case 3:
      if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        return p[0] << 16 | p[1] << 8 | p[2];
      else
        return p[0] | p[1] << 8 | p[2] << 16;
    case 4:
      return *(Uint32 *)p;
    default:
      return 0;
    }
  };

  for (int y = 0; y < m_height; ++y) {
    for (int x = 0; x < m_width; ++x) {
      // Map image size might differ from mask? Assume same or clamp.
      // If mask is smaller/larger, we should probably scale logic, but let's
      // assume user provided matching assets or we clamp to map size.
      if (x >= maskSurf->w || y >= maskSurf->h) {
        m_data[y * m_width + x] = 1; // Out of bounds -> Wall
        m_speedModifiers[y * m_width + x] = 0.0f;
        continue;
      }

      Uint32 pixel = getPixel(x, y);
      Uint8 r, g, b;
      SDL_GetRGB(pixel, maskSurf->format, &r, &g, &b);

      int idx = y * m_width + x;

      if (r == 0 && g == 0 && b == 0) {
        // Black = Wall
        m_data[idx] = 1;
        m_speedModifiers[idx] = 0.0f;
      } else if (b > 200 && r < 50 && g < 50) {
        // Blue = Road / Fast
        m_data[idx] = 0;
        m_speedModifiers[idx] = 1.0f;
      } else if (r > 200 && g < 50 && b < 50) {
        // Red = Slow
        m_data[idx] = 0;
        m_speedModifiers[idx] = 0.5f;
      } else {
        // Other (e.g. white/noise) -> Default Walkable
        m_data[idx] = 0;
        m_speedModifiers[idx] = 1.0f;
      }
    }
  }

  SDL_UnlockSurface(maskSurf);
  SDL_FreeSurface(maskSurf);

  // Recalculate AI
  calculateFlowField();

  return true;
}

float Map::getSpeedModifier(int x, int y) const {
  if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
    return m_speedModifiers[y * m_width + x];
  }
  return 0.0f;
}

void Map::generate() {
  // Simple cellular automata or random noise for "apocalyptic city"
  // Let's start with random noise + smoothing (cellular automata steps)

  // Random initialization
  for (int i = 0; i < m_width * m_height; ++i) {
    m_data[i] = (std::rand() % 100 < 40) ? 1 : 0; // 40% walls
  }

  // Smoothing steps
  for (int k = 0; k < 5; ++k) {
    std::vector<unsigned char> newData = m_data;
    for (int y = 0; y < m_height; ++y) {
      for (int x = 0; x < m_width; ++x) {
        int neighbors = 0;
        for (int dy = -1; dy <= 1; ++dy) {
          for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0)
              continue;
            int nx = x + dx;
            int ny = y + dy;
            if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height) {
              if (m_data[ny * m_width + nx] == 1)
                neighbors++;
            } else {
              neighbors++; // Border is wall
            }
          }
        }

        if (m_data[y * m_width + x] == 1) {
          if (neighbors < 4)
            newData[y * m_width + x] = 0;
          else
            newData[y * m_width + x] = 1;
        } else {
          if (neighbors > 4)
            newData[y * m_width + x] = 1;
          else
            newData[y * m_width + x] = 0;
        }
      }
    }
    m_data = newData;
  }

  // Ensure center is empty (for the fortress, though fortress is goal, zombies
  // spawn outside) Actually, requirements say "center fortress... zombies from
  // surroundings". So let's clear the center.
  int centerX = m_width / 2;
  int centerY = m_height / 2;
  int clearRadius = 10;

  for (int y = centerY - clearRadius; y <= centerY + clearRadius; ++y) {
    for (int x = centerX - clearRadius; x <= centerX + clearRadius; ++x) {
      if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
        m_data[y * m_width + x] = 0;
      }
    }
  }
  calculateFlowField();
}

void Map::calculateFlowField() {
  std::vector<int> distance(m_width * m_height, -1);
  std::vector<int> queue;

  int centerX = m_width / 2;
  int centerY = m_height / 2;

  // Target is the center
  distance[centerY * m_width + centerX] = 0;
  queue.push_back(centerY * m_width + centerX);

  // BFS
  int head = 0;
  while (head < queue.size()) {
    int idx = queue[head++];
    int x = idx % m_width;
    int y = idx / m_width;

    // Check 4 neighbors
    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};

    for (int k = 0; k < 4; ++k) {
      int nx = x + dx[k];
      int ny = y + dy[k];
      if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height) {
        int nidx = ny * m_width + nx;
        if (m_data[nidx] == 0 && distance[nidx] == -1) {
          distance[nidx] = distance[idx] + 1;
          queue.push_back(nidx);
        }
      }
    }
  }

  // Compute Vectors from distance map
  for (int y = 0; y < m_height; ++y) {
    for (int x = 0; x < m_width; ++x) {
      int idx = y * m_width + x;
      if (m_data[idx] == 1)
        continue; // Wall
      if (distance[idx] == -1)
        continue; // Unreachable

      float vecX = 0;
      float vecY = 0;

      int minVal = distance[idx];

      // Check neighbors to find gradient
      int dx[] = {0, 0, -1, 1};
      int dy[] = {-1, 1, 0, 0};

      for (int k = 0; k < 4; k++) {
        int nx = x + dx[k];
        int ny = y + dy[k];
        if (nx >= 0 && nx < m_width && ny >= 0 && ny < m_height) {
          int nidx = ny * m_width + nx;
          if (distance[nidx] != -1 && distance[nidx] < minVal) {
            // Point towards smaller distance
            vecX += dx[k];
            vecY += dy[k];
          }
        }
      }

      // Normalize
      float len = std::sqrt(vecX * vecX + vecY * vecY);
      if (len > 0) {
        m_flowField[idx] = {vecX / len, vecY / len};
      } else {
        m_flowField[idx] = {0.0f, 0.0f}; // If no gradient, stay put
      }
    }
  }
}

Map::Vector2 Map::getFlowAt(int x, int y) const {
  if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
    return m_flowField[y * m_width + x];
  }
  return {0.0f, 0.0f};
}

void Map::render() {
  // Render Background Texture
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_textureID);
  glColor3f(1.0f, 1.0f, 1.0f); // White tint to show texture as-is

  glBegin(GL_QUADS);
  glTexCoord2f(0.0f, 0.0f);
  glVertex2f(0.0f, 0.0f);
  glTexCoord2f(1.0f, 0.0f);
  glVertex2f((float)m_width, 0.0f);
  glTexCoord2f(1.0f, 1.0f);
  glVertex2f((float)m_width, (float)m_height);
  glTexCoord2f(0.0f, 1.0f);
  glVertex2f(0.0f, (float)m_height);
  glEnd();

  glDisable(GL_TEXTURE_2D);

  // Debug: Overlay obstacles slightly?
  // Uncomment to see logic map over texture
  /*
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glColor4f(1.0f, 0.0f, 0.0f, 0.3f); // Red overlay
  glBegin(GL_POINTS);
  for(int y=0; y<m_height; ++y) {
      for(int x=0; x<m_width; ++x) {
          if(m_data[y*m_width+x] == 1) glVertex2f(x, y);
      }
  }
  glEnd();
  glDisable(GL_BLEND);
  */

  // Draw Center Fortress
  glColor3f(1.0f, 0.0f, 0.0f); // Red
  glPointSize(5.0f);
  glBegin(GL_POINTS);
  glVertex2f(0.0f, 0.0f);
  glEnd();
  glPointSize(1.0f);
}

void Map::renderMask() {
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_maskTextureID);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glColor4f(1.0f, 1.0f, 1.0f, 0.5f); // 50% opacity

  glBegin(GL_QUADS);
  glTexCoord2f(0.0f, 0.0f);
  glVertex2f(0.0f, 0.0f);
  glTexCoord2f(1.0f, 0.0f);
  glVertex2f((float)m_width, 0.0f);
  glTexCoord2f(1.0f, 1.0f);
  glVertex2f((float)m_width, (float)m_height);
  glTexCoord2f(0.0f, 1.0f);
  glVertex2f(0.0f, (float)m_height);
  glEnd();

  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
}
