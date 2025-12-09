#include "Map.hpp"
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

    calculateFlowField();
  }
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
  // Simple immediate mode rendering for now
  glColor3f(0.5f, 0.5f, 0.5f); // Grey walls
  glColor3f(0.5f, 0.5f, 0.5f); // Grey walls

  glBegin(GL_QUADS);
  for (int y = 0; y < m_height; ++y) {
    for (int x = 0; x < m_width; ++x) {
      if (m_data[y * m_width + x] == 1) {
        // Draw in World Coordinates (0..800, 0..600)
        // 1 unit = 1 pixel (at zoom 1.0)
        glVertex2f(x, y);
        glVertex2f(x + 1, y);
        glVertex2f(x + 1, y + 1);
        glVertex2f(x, y + 1);
      }
    }
  }
  glEnd();

  // Draw Center Fortress
  glColor3f(1.0f, 0.0f, 0.0f); // Red
  glPointSize(5.0f);
  glBegin(GL_POINTS);
  glVertex2f(0.0f, 0.0f);
  glEnd();
  glPointSize(1.0f);
}
