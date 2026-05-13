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
      p.pos = glm::vec3(i * restLength, 0.5f, i * 0.01f);
      p.prevPos = p.pos;
      p.pinned = (i == 0); // Pin the first particle
      particles.push_back(p);
    }
  }

  bool showFloor = true;
  bool showSphere = true;

  void update(float dt) {
    // 1. Verlet Integration
    for (auto &p : particles) {
      if (p.pinned)
        continue;
      glm::vec3 vel = (p.pos - p.prevPos) * damping;
      p.prevPos = p.pos;
      p.pos += vel + gravity * dt * dt;
    }

    solveConstraints(dt);
    if (showFloor) {
      solvePlaneCollision();
    }
    if (showSphere) {
      solveSphereCollision();
    }
  }

  // MARK: solve constrain
  bool xpbd = true;
  float compliance = 0.0001f;

  void solveConstraints(float dt) {
    float alpha = compliance / (dt * dt); // compliance / dt^2

    for (int iter = 0; iter < iterations; iter++) {
      for (int i = 0; i < (int)particles.size() - 1; i++) {
        Particle &a = particles[i];
        Particle &b = particles[i + 1];

        glm::vec3 delta = b.pos - a.pos;
        float dist = glm::length(delta);
        if (dist < 1e-6f)
          continue;

        if (xpbd) {
          float C = dist - restLength;       // constraint violation
          float wA = a.pinned ? 0.0f : 1.0f; // inverse mass
          float wB = b.pinned ? 0.0f : 1.0f;
          float lambda = -C / (wA + wB + alpha);
          glm::vec3 correction = lambda * (delta / dist);

          if (!a.pinned)
            a.pos -= correction * wA;
          if (!b.pinned)
            b.pos += correction * wB;

        } else {
          float correction = (dist - restLength) / dist;
          glm::vec3 offset = delta * 0.5f * correction;

          if (!a.pinned)
            a.pos += offset;
          if (!b.pinned)
            b.pos -= offset;
        }
      }
    }
  }

  // MARK: solve plane collision
  float floor_y = -0.5f;
  float restitution = 0.3f; // bounciness, 0 = no bounce

  void solvePlaneCollision() {
    for (auto &p : particles) {
      if (p.pinned)
        continue;
      if (p.pos.y < floor_y) {
        // reflect prevPos across the floor to fake bounce
        float vel_y = (p.pos.y - p.prevPos.y);
        p.pos.y = floor_y;
        p.prevPos.y = floor_y + vel_y * restitution;
      }
    }
  }

  // MARK: solve sphere collision
  glm::vec3 sphereCenter = glm::vec3(0.0f, 0.0f, 0.0f);
  float sphereRadius = 0.2f;

  std::vector<glm::vec3> genSphereMesh(int stacks, int slices) {
    std::vector<glm::vec3> data;
    for (int i = 0; i < stacks; i++) {
      float phi0 = M_PI * i / stacks - M_PI / 2.0f;
      float phi1 = M_PI * (i + 1) / stacks - M_PI / 2.0f;
      for (int j = 0; j < slices; j++) {
        float theta0 = 2.0f * M_PI * j / slices;
        float theta1 = 2.0f * M_PI * (j + 1) / slices;

        auto pushVert = [&](float phi, float theta) {
          glm::vec3 p =
              sphereCenter + glm::vec3(sphereRadius * cos(phi) * cos(theta),
                                       sphereRadius * sin(phi),
                                       sphereRadius * cos(phi) * sin(theta));
          data.push_back(p);                                // Position
          data.push_back(glm::normalize(p - sphereCenter)); // Normal
        };

        // two triangles per quad
        pushVert(phi0, theta0);
        pushVert(phi1, theta0);
        pushVert(phi1, theta1);

        pushVert(phi0, theta0);
        pushVert(phi1, theta1);
        pushVert(phi0, theta1);
      }
    }
    return data;
  }

  void solveSphereCollision() {
    for (auto &p : particles) {
      if (p.pinned)
        continue;
      glm::vec3 delta = p.pos - sphereCenter;
      float dist = glm::length(delta);
      if (dist < sphereRadius) {
        // Push particle to sphere surface
        glm::vec3 surface = sphereCenter + (delta / dist) * sphereRadius;
        
        // Calculate velocity and normal at collision point
        glm::vec3 vel = p.pos - p.prevPos;
        glm::vec3 n = delta / dist; // This is the normal
        
        // Separate velocity into components
        glm::vec3 vNormal = glm::dot(vel, n) * n;
        glm::vec3 vTangential = vel - vNormal;
        
        // Reflect only the normal component
        glm::vec3 vReflected = vTangential - vNormal * restitution;
        
        p.pos = surface;
        p.prevPos = surface - vReflected;
      }
    }
  }
};

#endif
