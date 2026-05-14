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
  float damping = 0.95f;
  glm::vec3 gravity = glm::vec3(0.0f, -9.8f, 0.0f);
  float restLength = 0.05f;
  bool pinLast = false;
  float timeScale = 1.0f; // Multiplier for simulation time step

  Rope() { reset(); }

  void reset() {
    particles.clear();
    for (int i = 0; i < numParticles; i++) {
      Particle p;
      p.pos = glm::vec3(i * restLength, 0.5f, i * 0.01f);
      p.prevPos = p.pos;
      p.pinned = (i == 0 || i == numParticles - 1 && pinLast);
      particles.push_back(p);
    }
  }

  // MARK: render tube
  float tubeRadius = 0.01f;
  int tubeSides = 10;

  std::vector<glm::vec3> generateTube(const std::vector<Particle> &particles) {
    std::vector<glm::vec3> verts;

    struct VertexData {
      glm::vec3 pos;
      glm::vec3 norm;
    };

    auto getRing = [&](int i) {
      // tangent along rope
      glm::vec3 tangent;
      if (i == 0)
        tangent = glm::normalize(particles[1].pos - particles[0].pos);
      else if (i == (int)particles.size() - 1)
        tangent = glm::normalize(particles[i].pos - particles[i - 1].pos);
      else
        tangent = glm::normalize(particles[i + 1].pos - particles[i - 1].pos);

      // perpendicular frame
      glm::vec3 up = glm::vec3(0, 1, 0);
      if (glm::abs(glm::dot(tangent, up)) > 0.99f)
        up = glm::vec3(1, 0, 0);
      glm::vec3 normal = glm::normalize(glm::cross(tangent, up));
      glm::vec3 binormal = glm::cross(tangent, normal);

      std::vector<VertexData> ring;
      for (int j = 0; j < tubeSides; j++) {
        float angle = 2.0f * M_PI * j / tubeSides;
        glm::vec3 unitOffset =
            (float(cos(angle)) * normal + float(sin(angle)) * binormal);
        ring.push_back(
            {particles[i].pos + tubeRadius * unitOffset, unitOffset});
      }
      return ring;
    };

    for (int i = 0; i < (int)particles.size() - 1; i++) {
      auto r0 = getRing(i);
      auto r1 = getRing(i + 1);
      for (int j = 0; j < tubeSides; j++) {
        int next = (j + 1) % tubeSides;
        // Triangle 1
        verts.push_back(r0[j].pos);
        verts.push_back(r0[j].norm);
        verts.push_back(r1[j].pos);
        verts.push_back(r1[j].norm);
        verts.push_back(r1[next].pos);
        verts.push_back(r1[next].norm);
        // Triangle 2
        verts.push_back(r0[j].pos);
        verts.push_back(r0[j].norm);
        verts.push_back(r1[next].pos);
        verts.push_back(r1[next].norm);
        verts.push_back(r0[next].pos);
        verts.push_back(r0[next].norm);
      }
    }
    return verts;
  }

  // MARK: update
  bool showFloor = true;
  bool showSphere = true;
  bool showBox = false;
  bool enableGravity = true;
  int subSteps = 10;

  void update(float dt) {
    float simDt = dt * timeScale;
    float subDt = simDt / subSteps;

    // Adjust damping so it behaves similarly regardless of sub-steps
    float subDamping = std::pow(damping, 1.0f / subSteps);

    for (int s = 0; s < subSteps; s++) {
      // 1. Verlet Integration
      for (auto &p : particles) {
        if (p.pinned)
          continue;
        glm::vec3 vel = (p.pos - p.prevPos) * subDamping;
        p.prevPos = p.pos;
        if (enableGravity)
          p.pos += vel + gravity * subDt * subDt;
        else
          p.pos += vel;
      }

      // 2. Solve Constraints (1 iteration per sub-step is standard for XPBD)
      solveConstraints(subDt, 1);

      // 3. Collisions
      if (showFloor) {
        solvePlaneCollision();
      }
      if (showSphere) {
        solveSphereCollision();
      }
      if (showBox) {
        solveBoxCollision();
      }
    }
  }
  // MARK: solve constrain
  bool xpbd = true;
  float compliance = 0.0001f;

  void solveConstraints(float dt, int iters) {
    float alpha = compliance / (dt * dt); // compliance / dt^2
    std::vector<float> lambdas(particles.size() - 1, 0.0f);

    for (int iter = 0; iter < iters; iter++) {
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

          // XPBD update: delta_lambda = (-C - alpha * lambda_total) / (sum_w +
          // alpha)
          float dLambda = (-C - alpha * lambdas[i]) / (wA + wB + alpha);
          lambdas[i] += dLambda;

          glm::vec3 correction = dLambda * (delta / dist);

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

  // MARK: solve box collision
  glm::vec3 boxMin = glm::vec3(-0.05f, -0.05f, -0.3f);
  glm::vec3 boxMax = glm::vec3(0.05f, 0.05f, 0.3f);

  void solveBoxCollision() {
    for (auto &p : particles) {
      if (p.pinned)
        continue;

      // not inside box, skip
      if (p.pos.x < boxMin.x || p.pos.x > boxMax.x)
        continue;
      if (p.pos.y < boxMin.y || p.pos.y > boxMax.y)
        continue;
      if (p.pos.z < boxMin.z || p.pos.z > boxMax.z)
        continue;

      // find shallowest penetration axis
      float dists[6] = {
          p.pos.x - boxMin.x, boxMax.x - p.pos.x, p.pos.y - boxMin.y,
          boxMax.y - p.pos.y, p.pos.z - boxMin.z, boxMax.z - p.pos.z,
      };

      int minAxis = 0;
      for (int i = 1; i < 6; i++)
        if (dists[i] < dists[minAxis])
          minAxis = i;

      // push out along shallowest axis
      glm::vec3 normal = glm::vec3(0.0f);
      float penetration = dists[minAxis];
      if (minAxis == 0) {
        normal = glm::vec3(-1, 0, 0);
        p.pos.x = boxMin.x;
      }
      if (minAxis == 1) {
        normal = glm::vec3(1, 0, 0);
        p.pos.x = boxMax.x;
      }
      if (minAxis == 2) {
        normal = glm::vec3(0, -1, 0);
        p.pos.y = boxMin.y;
      }
      if (minAxis == 3) {
        normal = glm::vec3(0, 1, 0);
        p.pos.y = boxMax.y;
      }
      if (minAxis == 4) {
        normal = glm::vec3(0, 0, -1);
        p.pos.z = boxMin.z;
      }
      if (minAxis == 5) {
        normal = glm::vec3(0, 0, 1);
        p.pos.z = boxMax.z;
      }

      // velocity reflection along normal (restitution)
      glm::vec3 vel = p.pos - p.prevPos;
      float velN = glm::dot(vel, normal);
      if (velN < 0.0f)
        p.prevPos += normal * velN * (1.0f + restitution);
    }
  }
};

#endif
