#include "Simulation.hpp"
#include <SDL_opengl.h>
#include <cmath>
#include <cstdlib>

Simulation::Simulation(Map *map)
    : m_map(map), m_score(0), m_textureID(0), m_zombieSize(1.0f) {}

Simulation::~Simulation() {}

void Simulation::init(int particleCount) {
  m_particles.clear();
  m_particles.reserve(particleCount);

  int w = m_map->getWidth();
  int h = m_map->getHeight();
  const auto &data = m_map->getData();

  for (int i = 0; i < particleCount; ++i) {
    // Find random valid position
    int lx, ly;
    int attempts = 0;
    do {
      lx = std::rand() % w;
      ly = std::rand() % h;
      attempts++;
    } while (data[ly * w + lx] != 0 && attempts < 100);

    if (attempts < 100) {
      Particle p;
      p.x = lx + 0.5f;
      p.y = ly + 0.5f;
      p.vx = 0;
      p.vy = 0;
      m_particles.push_back(p);
    }
  }
}

void Simulation::update(float dt) {
  int w = m_map->getWidth();
  int h = m_map->getHeight();
  int centerX = w / 2;
  int centerY = h / 2;
  float radiusSq = 25.0f; // 5 units radius squared

  float speed = 10.0f; // Units per second
  float randomness = 2.0f;

  // Continuous Spawning
  if (m_particles.size() < 10000) { // Cap at 10000
    // Spawn 10 per frame
    for (int i = 0; i < 10; ++i) {
      // Spawn at edges? Or random?
      // "spawns at random positions on the edges of the map"
      int side = std::rand() % 4;
      int lx, ly;
      if (side == 0) {
        lx = 0;
        ly = std::rand() % h;
      } // Left
      else if (side == 1) {
        lx = w - 1;
        ly = std::rand() % h;
      } // Right
      else if (side == 2) {
        lx = std::rand() % w;
        ly = 0;
      } // Top
      else {
        lx = std::rand() % w;
        ly = h - 1;
      } // Bottom

      if (m_map->getData()[ly * w + lx] == 0) {
        Particle p;
        p.x = lx + 0.5f;
        p.y = ly + 0.5f;
        p.vx = 0;
        p.vy = 0;
        m_particles.push_back(p);
      }
    }
  }

  for (auto it = m_particles.begin(); it != m_particles.end();) {
    Particle &p = *it;

    int ix = (int)p.x;
    int iy = (int)p.y;

    // Get flow
    Map::Vector2 flow = m_map->getFlowAt(ix, iy);

    // Simple movement: move towards flow
    // Add some random jitter
    float rx = ((float)std::rand() / RAND_MAX - 0.5f) * randomness;
    float ry = ((float)std::rand() / RAND_MAX - 0.5f) * randomness;

    float dx = (flow.x * speed + rx) * dt;
    float dy = (flow.y * speed + ry) * dt;

    // Update velocity for rotation (smooth it?)
    // Simple assignment for now
    if (dx != 0 || dy != 0) {
      float len = std::sqrt(dx * dx + dy * dy);
      p.vx = dx / len;
      p.vy = dy / len;
    }

    float nextX = p.x + dx;
    float nextY = p.y + dy;

    // Collision Check
    int nix = (int)nextX;
    int niy = (int)nextY;

    if (nix >= 0 && nix < w && niy >= 0 && niy < h) {
      if (m_map->getData()[niy * w + nix] == 0) {
        p.x = nextX;
        p.y = nextY;
      } else {
        // Hit wall, maybe slide? Simple: don't move
      }
    }

    // Check Goal
    float distSq =
        (p.x - centerX) * (p.x - centerX) + (p.y - centerY) * (p.y - centerY);
    if (distSq < radiusSq) {
      m_score++;
      it = m_particles.erase(it);
    } else {
      ++it;
    }
  }
}

void Simulation::render() {
  if (m_textureID != 0) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    glColor3f(1.0f, 1.0f, 1.0f); // White modulation
  } else {
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.0f, 1.0f, 0.0f); // Green fallback
    glPointSize(m_zombieSize * 2.0f);
    glBegin(GL_POINTS);
  }

  int w = m_map->getWidth();
  int h = m_map->getHeight();

  // Use display list or arrays for performance? Immediate mode is slow for 5000
  // particles. But for now, keep immediate mode.

  if (m_textureID == 0) {
    // Fallback Rendering
    for (const auto &p : m_particles) {
      float glX = (p.x / w) * 2.0f - 1.0f;
      float glY = -((p.y / h) * 2.0f - 1.0f);
      glVertex2f(glX, glY);
    }
    glEnd();
    glPointSize(1.0f);
  } else {
    // Textured Quads
    // We need to render in world space or screen space?
    // Map render translates to coordinate system where 0..width is mapped to
    // -1..1 or similar? Wait, Map::render draws in 0..width, 0..height. And
    // Game::render scales it... wait. Game::render does: glScalef(m_zoom,
    // m_zoom, 1.0f); glTranslatef(m_offsetX, m_offsetY, 0.0f); And then calls
    // Map::render.

    // I previously used -1..1 in Simulation::render, which was WRONG if
    // Map::render uses 0..width. Let's align with Map::render which seems to
    // use cell coordinates directly.

    // Map::render code:
    // glRectf(x, y, x + 1.0f, y + 1.0f);
    // This means Map draws from (0,0) to (width, height).

    // So Simulation should also draw in (0,0) to (width, height) space.
    // My previous Simulation::render used -1..1. That was probably rendering
    // tiny dots in the center! I need to fix this to use p.x, p.y directly.

    float size = m_zombieSize;

    for (const auto &p : m_particles) {
      float x = p.x;
      float y = p.y;
      float vx = p.vx;
      float vy = p.vy;

      float angle = 0.0f;
      if (vx != 0 || vy != 0) {
        angle = std::atan2(vy, vx) * 180.0f / 3.14159f;
      }

      glPushMatrix();
      glTranslatef(x, y, 0.0f);
      glRotatef(angle, 0.0f, 0.0f, 1.0f);

      glBegin(GL_QUADS);
      glTexCoord2f(0, 0);
      glVertex2f(-size / 2, -size / 2);
      glTexCoord2f(1, 0);
      glVertex2f(size / 2, -size / 2);
      glTexCoord2f(1, 1);
      glVertex2f(size / 2, size / 2);
      glTexCoord2f(0, 1);
      glVertex2f(-size / 2, size / 2);
      glEnd();

      glPopMatrix();
    }
  }
  glDisable(GL_TEXTURE_2D);
}
