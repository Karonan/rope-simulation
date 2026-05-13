#ifndef ROPE_H
#define ROPE_H

#include <glm/glm.hpp>
#include <vector>

struct Particle {
  glm::vec3 pos;
  glm::vec3 prevPos;
  bool pinned;
};

class Rope {
public:
  std::vector<Particle> particles;

  int numParticles = 20;
  float damping = 0.99f;
  glm::vec3 gravity = glm::vec3(0.0f, -9.8f, 0.0f);
  float restLength = 0.05f;
  int iterations = 10;

  Rope() {
    // Initialize particles
    for (int i = 0; i < numParticles; i++) {
      Particle p;
      p.pos = glm::vec3(i * restLength, 0.5f, 0.0f);
      p.prevPos = p.pos;
      p.pinned = (i == 0); // Pin the first particle
      particles.push_back(p);
    }
  }

  void update(float dt) {
    // 1. Verlet Integration
    for (auto &p : particles) {
      if (p.pinned)
        continue;
      glm::vec3 vel = (p.pos - p.prevPos) * damping;
      p.prevPos = p.pos;
      p.pos += vel + gravity * dt * dt;
    }

    solveConstraints();
  }

  void solveConstraints() {
    for (int iter = 0; iter < iterations; iter++) {
      for (int i = 0; i < (int)particles.size() - 1; i++) {
        Particle &a = particles[i];
        Particle &b = particles[i + 1];

        glm::vec3 delta = b.pos - a.pos;
        float dist = glm::length(delta);
        if (dist < 1e-6f)
          continue;

        float correction = (dist - restLength) / dist;
        glm::vec3 offset = delta * 0.5f * correction;

        if (!a.pinned)
          a.pos += offset;
        if (!b.pinned)
          b.pos -= offset;
      }
    }
  }
};

#endif
