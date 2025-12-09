#pragma once
#include "Map.hpp"
#include <vector>

struct Particle {
  float x, y;
  float vx, vy;
  float ax, ay;
};

class Simulation {
public:
  Simulation(Map *map);
  ~Simulation();

  void init(int particleCount);
  void update(float dt); // dt in seconds
  void render();

  void setTexture(unsigned int textureID) { m_textureID = textureID; }
  void setZombieSize(float size) { m_zombieSize = size; }

  int getScore() const { return m_score; }
  int getParticleCount() const { return m_particles.size(); }

private:
  Map *m_map;
  std::vector<Particle> m_particles;
  int m_score;
  unsigned int m_textureID;
  float m_zombieSize;

  // Spatial Grid
  std::vector<std::vector<int>> m_grid;
  int m_gridWidth;
  int m_gridHeight;
  int m_cellSize;

  void updateGrid();
  void getNeighbors(int pIndex, std::vector<int> &neighbors);
};
