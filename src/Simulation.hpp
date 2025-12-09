#pragma once
#include "Map.hpp"
#include <vector>

struct Particle {
  float x, y;
  float vx, vy;
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
};
