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

  // Grid Init
  m_cellSize = 4; // Slightly larger than max zombie size (3.5)
  m_gridWidth = (w + m_cellSize - 1) / m_cellSize;
  m_gridHeight = (h + m_cellSize - 1) / m_cellSize;
  m_grid.resize(m_gridWidth * m_gridHeight);

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
      p.ax = 0;
      p.ay = 0;
      m_particles.push_back(p);
    }
  }
}

void Simulation::updateGrid() {
  for (auto &cell : m_grid) {
    cell.clear();
  }
  for (int i = 0; i < m_particles.size(); ++i) {
    const auto &p = m_particles[i];
    int gx = (int)p.x / m_cellSize;
    int gy = (int)p.y / m_cellSize;
    if (gx >= 0 && gx < m_gridWidth && gy >= 0 && gy < m_gridHeight) {
      m_grid[gy * m_gridWidth + gx].push_back(i);
    }
  }
}

void Simulation::getNeighbors(int pIndex, std::vector<int> &neighbors) {
  const auto &p = m_particles[pIndex];
  int gx = (int)p.x / m_cellSize;
  int gy = (int)p.y / m_cellSize;

  for (int y = -1; y <= 1; ++y) {
    for (int x = -1; x <= 1; ++x) {
      int nx = gx + x;
      int ny = gy + y;
      if (nx >= 0 && nx < m_gridWidth && ny >= 0 && ny < m_gridHeight) {
        const auto &cell = m_grid[ny * m_gridWidth + nx];
        neighbors.insert(neighbors.end(), cell.begin(), cell.end());
      }
    }
  }
}

void Simulation::update(float dt) {
  int w = m_map->getWidth();
  int h = m_map->getHeight();
  int centerX = w / 2;
  int centerY = h / 2;
  float goalRadiusSq = 25.0f;

  float maxSpeed = 10.0f;
  float maxForce = 20.0f; // Steering force magnitude
  float separationDist =
      m_zombieSize * 1.2f; // Keep distance slightly larger than size

  // Continuous Spawning
  if (m_particles.size() < 10000) {
    // ... (Spawning logic can remain same or be refactored, keeping it minimal
    // for diff size) Re-implementing simplified spawning for this block
    for (int i = 0; i < 5; ++i) { // 5 per frame
      int side = std::rand() % 4;
      int lx, ly;
      // ... (Edge logic)
      if (side == 0) {
        lx = 0;
        ly = std::rand() % h;
      } else if (side == 1) {
        lx = w - 1;
        ly = std::rand() % h;
      } else if (side == 2) {
        lx = std::rand() % w;
        ly = 0;
      } else {
        lx = std::rand() % w;
        ly = h - 1;
      }

      if (m_map->getData()[ly * w + lx] == 0) {
        Particle p = {(float)lx + 0.5f, (float)ly + 0.5f, 0, 0, 0, 0};
        m_particles.push_back(p);
      }
    }
  }

  updateGrid();
  std::vector<int> neighbors;
  neighbors.reserve(50);

  for (int i = 0; i < m_particles.size(); ++i) {
    Particle &p = m_particles[i];

    // Reset acceleration
    p.ax = 0;
    p.ay = 0;

    // 1. Flow Field Following
    int ix = (int)p.x;
    int iy = (int)p.y;
    Map::Vector2 flow = m_map->getFlowAt(ix, iy);

    // Desired velocity based on flow
    float desiredX = flow.x * maxSpeed;
    float desiredY = flow.y * maxSpeed;

    // Steering = Desired - Velocity
    float steerX = desiredX - p.vx;
    float steerY = desiredY - p.vy;

    // Limit steering force
    float steerLen = std::sqrt(steerX * steerX + steerY * steerY);
    if (steerLen > maxForce) {
      steerX = (steerX / steerLen) * maxForce;
      steerY = (steerY / steerLen) * maxForce;
    }

    // Add randomness
    float rx = ((float)std::rand() / RAND_MAX - 0.5f) * 10.0f;
    float ry = ((float)std::rand() / RAND_MAX - 0.5f) * 10.0f;

    p.ax += steerX + rx;
    p.ay += steerY + ry;

    // 2. Separation
    neighbors.clear();
    getNeighbors(i, neighbors);

    float sepX = 0;
    float sepY = 0;
    int sepCount = 0;

    for (int nIdx : neighbors) {
      if (nIdx == i)
        continue;
      const Particle &other = m_particles[nIdx];
      float dx = p.x - other.x;
      float dy = p.y - other.y;
      float dSq = dx * dx + dy * dy;

      // Use user-defined size for separation
      float minSep = m_zombieSize;
      if (dSq > 0 && dSq < minSep * minSep) {
        float d = std::sqrt(dSq);
        // Stronger repulsive force closer we are
        float force = (minSep - d) / d; // Simple linear repulsion
        sepX += dx * force; // Normalize by dividing by d is done implicitly by
                            // dx/d * (minSep-d) ? No dx/d * mag.
        // dx/d is normal. magnitude is (minSep - d). so dx/d * (minSep - d) =
        // dx * (minSep/d - 1). Let's use simple weighting: vector/distSq
        sepX += (dx / d) * (minSep - d) * 50.0f; // 50.0f separation weight
        sepY += (dy / d) * (minSep - d) * 50.0f;
        sepCount++;
      }
    }

    p.ax += sepX;
    p.ay += sepY;

    // 3. Integration
    p.vx += p.ax * dt;
    p.vy += p.ay * dt;

    // Limit speed
    float speedSq = p.vx * p.vx + p.vy * p.vy;
    if (speedSq > maxSpeed * maxSpeed) {
      float speed = std::sqrt(speedSq);
      p.vx = (p.vx / speed) * maxSpeed;
      p.vy = (p.vy / speed) * maxSpeed;
    }

    float nextX = p.x + p.vx * dt;
    float nextY = p.y + p.vy * dt;

    // 4. Wall Collision (Simple slide)
    int nix = (int)nextX;
    int niy = (int)nextY;
    if (nix >= 0 && nix < w && niy >= 0 && niy < h) {
      if (m_map->getData()[niy * w + nix] == 0) {
        p.x = nextX;
        p.y = nextY;
      } else {
        // Hit wall, kill velocity perpendicular? Or just stop.
        p.vx = 0;
        p.vy = 0;
        // Don't update pos
      }
    } else {
      // Map bounds
      p.x = std::max(0.0f, std::min((float)w, nextX));
      p.y = std::max(0.0f, std::min((float)h, nextY));
    }

    // 5. Goal Check
    // Reuse iter erase logic?
    // Doing erase in loop with index access is tricky.
    // Better to mark for deletion or swap-remove.
    // Let's stick to simple loop structure but be careful.
    // Actually, "update" loop above is index based.
    // Let's do a second pass for removal or handle it here with swap-back?
    // Swap-back changes indices, breaking updateGrid/neighbors if done
    // mid-frame. Actually, updateGrid is done once. If I swap-back, indices
    // change. So if I process i, and swap last into i, I need to process i
    // again. But grid refers to old indices! Safest: Use `erase` but iterate
    // backwards? Or just mark dead. Or just simple erase and adjust `i` and
    // rebuild grid next frame. Performance hit of erase is O(N), but we only
    // erase a few per frame.
  }

  // Cleanup loop
  for (auto it = m_particles.begin(); it != m_particles.end();) {
    float distanceSq = (it->x - centerX) * (it->x - centerX) +
                       (it->y - centerY) * (it->y - centerY);
    if (distanceSq < goalRadiusSq) {
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

      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

      glDisable(GL_BLEND);

      glPopMatrix();
    }
  }
  glDisable(GL_TEXTURE_2D);
}
