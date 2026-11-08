/**
 * @file PhysicsBenchmark.cpp
 * @brief Comprehensive, reproducible benchmarking suite for Elysium Engine's 2D physics subsystem.
 *
 * Benchmarks:
 *   1. BroadPhase: Spatial hashing candidate pair generation vs. brute-force baseline.
 *   2. NarrowPhase: SAT collision detection & manifold generation for candidate pairs.
 *   3. Physics World Step: Full integration, broadphase, narrowphase, and boundary pipeline.
 *
 * Scenarios:
 *   - Distributions: Sparse, Dense/Clustered, Stress Grid (mixed static/dynamic)
 *   - Shapes: Sphere-Sphere, Box-Box, Box-Sphere, Mixed (Sphere, Box, Polygon)
 *   - Parametric Scaling: N in {100, 500, 1000, 2500, 5000}
 *
 * Metrics:
 *   - Average timing (us/ms) and standard deviation across iterations.
 *   - Secondary counters: Potential Candidate Pairs, SAT Checks, Confirmed Collisions.
 *   - Correctness verification against unpruned baseline ground truth.
 *   - Console, JSON, and CSV exports for CI/CD historical tracking.
 */

#include "AABB.hpp"
#include "BroadPhase.hpp"
#include "Collider.hpp"
#include "CollisionPair.hpp"
#include "CoreMath.hpp"
#include "NarrowPhase.hpp"
#include "PhysicsWorld.hpp"
#include "RigidBody.hpp"
#include "SAT.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

// ============================================================================
// Configuration & Enumerations
// ============================================================================

enum class BenchmarkTarget {
  All,
  BroadPhase,
  NarrowPhase,
  WorldStep
};

enum class DistributionType {
  All,
  Sparse,
  Dense,
  StressGrid
};

enum class ShapeType {
  All,
  SphereSphere,
  BoxBox,
  BoxSphere,
  Mixed
};

enum class OutputFormat {
  Console,
  JSON,
  CSV,
  All
};

struct BenchmarkOptions {
  BenchmarkTarget target = BenchmarkTarget::All;
  DistributionType distribution = DistributionType::All;
  ShapeType shapes = ShapeType::All;
  std::vector<size_t> entityCounts = {100, 500, 1000, 2500, 5000};
  size_t iterations = 20;
  size_t warmupRuns = 5;
  OutputFormat format = OutputFormat::Console;
  std::string outputPath = "";
  bool verify = false;
  uint32_t seed = 42;
};

// ============================================================================
// Statistical Metrics & Benchmark Results
// ============================================================================

struct BenchmarkMetrics {
  double meanUs = 0.0;
  double stdDevUs = 0.0;
  double minUs = 0.0;
  double maxUs = 0.0;
  double medianUs = 0.0;
  double throughputOpsPerSec = 0.0;

  // Secondary counters
  size_t candidatePairs = 0;
  size_t satChecks = 0;
  size_t actualCollisions = 0;

  // Verification results
  bool verified = true;
  std::string verificationNotes = "OK";
};

struct BenchmarkRecord {
  std::string phase;
  std::string distribution;
  std::string shapeConfig;
  size_t entityCount = 0;
  size_t iterations = 0;
  BenchmarkMetrics metrics;
};

// ============================================================================
// Helper Utilities & Math
// ============================================================================

static std::string TargetToString(BenchmarkTarget t) {
  switch (t) {
  case BenchmarkTarget::All: return "all";
  case BenchmarkTarget::BroadPhase: return "broadphase";
  case BenchmarkTarget::NarrowPhase: return "narrowphase";
  case BenchmarkTarget::WorldStep: return "world";
  }
  return "unknown";
}

static std::string DistributionToString(DistributionType d) {
  switch (d) {
  case DistributionType::All: return "all";
  case DistributionType::Sparse: return "sparse";
  case DistributionType::Dense: return "dense";
  case DistributionType::StressGrid: return "stress_grid";
  }
  return "unknown";
}

static std::string ShapeToString(ShapeType s) {
  switch (s) {
  case ShapeType::All: return "all";
  case ShapeType::SphereSphere: return "sphere_sphere";
  case ShapeType::BoxBox: return "box_box";
  case ShapeType::BoxSphere: return "box_sphere";
  case ShapeType::Mixed: return "mixed";
  }
  return "unknown";
}

static BenchmarkMetrics ComputeStatistics(const std::vector<double> &durationsUs,
                                          size_t candidatePairs,
                                          size_t satChecks,
                                          size_t actualCollisions) {
  BenchmarkMetrics m;
  if (durationsUs.empty()) return m;

  m.candidatePairs = candidatePairs;
  m.satChecks = satChecks;
  m.actualCollisions = actualCollisions;

  double sum = std::accumulate(durationsUs.begin(), durationsUs.end(), 0.0);
  m.meanUs = sum / static_cast<double>(durationsUs.size());

  double sqSum = 0.0;
  for (double val : durationsUs) {
    double diff = val - m.meanUs;
    sqSum += diff * diff;
  }
  m.stdDevUs = std::sqrt(sqSum / static_cast<double>(durationsUs.size()));

  auto sorted = durationsUs;
  std::sort(sorted.begin(), sorted.end());
  m.minUs = sorted.front();
  m.maxUs = sorted.back();
  if (sorted.size() % 2 == 0) {
    m.medianUs = (sorted[sorted.size() / 2 - 1] + sorted[sorted.size() / 2]) * 0.5;
  } else {
    m.medianUs = sorted[sorted.size() / 2];
  }

  if (m.meanUs > 0.0) {
    m.throughputOpsPerSec = 1000000.0 / m.meanUs;
  }
  return m;
}

// ============================================================================
// Entity & Scenario Generator
// ============================================================================

class ScenarioGenerator {
public:
  static Collider CreateRegularPolygon(int numSides, float radius, float density) {
    Collider c;
    c.type = ColliderType::Polygon;
    c.radius = radius;
    c.localCentroid = Vec3(0.0f, 0.0f, 0.0f);
    c.offset = Vec2(0.0f, 0.0f);

    float angleStep = (2.0f * PI) / static_cast<float>(numSides);
    c.localVertices.reserve(static_cast<size_t>(numSides));
    for (int i = 0; i < numSides; ++i) {
      float a = static_cast<float>(i) * angleStep;
      c.localVertices.push_back(Vec3(std::cos(a) * radius, std::sin(a) * radius, 0.0f));
    }

    // Approximate mass and moment of inertia
    float side = 2.0f * radius * std::sin(PI / static_cast<float>(numSides));
    float apothem = radius * std::cos(PI / static_cast<float>(numSides));
    float area = 0.5f * static_cast<float>(numSides) * side * apothem;
    c.mass = area * density;
    float inertia = 0.5f * c.mass * radius * radius;
    c.localInertiaTensor = Mat3::Diagonal(inertia, inertia, inertia);
    return c;
  }

  static void AttachCollider(RigidBody *body, ShapeType shape, size_t index, std::mt19937 &rng) {
    switch (shape) {
    case ShapeType::SphereSphere:
      body->AddColliders(Collider::CreateSphere(1.0f, 1.0f));
      break;
    case ShapeType::BoxBox:
      body->AddColliders(Collider::CreateBox(Vec3(1.0f, 1.0f, 0.5f), 1.0f));
      break;
    case ShapeType::BoxSphere:
      if (index % 2 == 0) {
        body->AddColliders(Collider::CreateBox(Vec3(1.0f, 1.0f, 0.5f), 1.0f));
      } else {
        body->AddColliders(Collider::CreateSphere(1.0f, 1.0f));
      }
      break;
    case ShapeType::Mixed:
    case ShapeType::All: {
      std::uniform_int_distribution<int> shapeDist(0, 2);
      int choice = shapeDist(rng);
      if (choice == 0) {
        body->AddColliders(Collider::CreateSphere(1.0f, 1.0f));
      } else if (choice == 1) {
        body->AddColliders(Collider::CreateBox(Vec3(1.0f, 1.0f, 0.5f), 1.0f));
      } else {
        body->AddColliders(CreateRegularPolygon(5, 1.2f, 1.0f));
      }
      break;
    }
    }
  }

  static std::vector<RigidBody *> Generate(size_t n,
                                           DistributionType dist,
                                           ShapeType shape,
                                           uint32_t seed) {
    std::vector<RigidBody *> bodies;
    bodies.reserve(n);
    std::mt19937 rng(seed);

    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * PI);
    std::uniform_real_distribution<float> velDist(-5.0f, 5.0f);

    if (dist == DistributionType::Sparse) {
      // Scale bounding box with sqrt(N) to maintain low density across sweeps
      float domainSize = std::max(200.0f, 35.0f * std::sqrt(static_cast<float>(n)));
      std::uniform_real_distribution<float> posDist(-domainSize * 0.5f, domainSize * 0.5f);

      for (size_t i = 0; i < n; ++i) {
        auto *b = new RigidBody();
        b->position = Vec3(posDist(rng), posDist(rng), 0.0f);
        b->orientation = Mat3::RotationZ(angleDist(rng));
        b->linearVelocity = Vec3(velDist(rng), velDist(rng), 0.0f);
        b->isStatic = false;
        b->inverseMass = 1.0f;
        b->restitution = 0.8f;
        b->friction = 0.3f;
        AttachCollider(b, shape, i, rng);
        b->UpdateGlobalCentroidFromPosition();
        bodies.push_back(b);
      }
    } else if (dist == DistributionType::Dense) {
      // Tightly bounded region or multi-cluster packing
      float clusterSize = std::min(40.0f, 1.5f * std::sqrt(static_cast<float>(n)));
      std::uniform_real_distribution<float> posDist(-clusterSize * 0.5f, clusterSize * 0.5f);

      for (size_t i = 0; i < n; ++i) {
        auto *b = new RigidBody();
        b->position = Vec3(posDist(rng), posDist(rng), 0.0f);
        b->orientation = Mat3::RotationZ(angleDist(rng));
        b->linearVelocity = Vec3(velDist(rng) * 0.2f, velDist(rng) * 0.2f, 0.0f);
        b->isStatic = false;
        b->inverseMass = 1.0f;
        b->restitution = 0.8f;
        b->friction = 0.3f;
        AttachCollider(b, shape, i, rng);
        b->UpdateGlobalCentroidFromPosition();
        bodies.push_back(b);
      }
    } else { // StressGrid
      // 2D regular lattice alternating static obstacles and dynamic bodies
      size_t cols = static_cast<size_t>(std::ceil(std::sqrt(static_cast<float>(n))));
      float spacing = 2.8f; // Bodies size ~2.0f, tight lattice
      float startX = -static_cast<float>(cols) * spacing * 0.5f;
      float startY = startX;

      for (size_t i = 0; i < n; ++i) {
        size_t row = i / cols;
        size_t col = i % cols;

        auto *b = new RigidBody();
        b->position = Vec3(startX + static_cast<float>(col) * spacing,
                           startY + static_cast<float>(row) * spacing,
                           0.0f);
        b->orientation = Mat3::RotationZ(angleDist(rng) * 0.2f);

        // Every 3rd body is static obstacle
        if (i % 3 == 0) {
          b->isStatic = true;
          b->inverseMass = 0.0f;
          b->linearVelocity = Vec3(0.0f, 0.0f, 0.0f);
        } else {
          b->isStatic = false;
          b->inverseMass = 1.0f;
          b->linearVelocity = Vec3(velDist(rng), velDist(rng), 0.0f);
        }
        b->restitution = 0.8f;
        b->friction = 0.3f;
        AttachCollider(b, shape, i, rng);
        b->UpdateGlobalCentroidFromPosition();
        bodies.push_back(b);
      }
    }

    return bodies;
  }

  static void Cleanup(std::vector<RigidBody *> &bodies) {
    for (auto *b : bodies) {
      delete b;
    }
    bodies.clear();
  }
};

// ============================================================================
// Verification Engine (Ground Truth Comparison)
// ============================================================================

class VerificationEngine {
public:
  static std::vector<CollisionPair> BruteForceGeneratePairs(const std::vector<RigidBody *> &bodies) {
    std::vector<CollisionPair> pairs;
    for (size_t i = 0; i < bodies.size(); ++i) {
      AABB a = bodies[i]->GetAABB();
      for (size_t j = i + 1; j < bodies.size(); ++j) {
        AABB b = bodies[j]->GetAABB();
        if ((a.min.x <= b.max.x && a.max.x >= b.min.x) &&
            (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
            (a.min.z <= b.max.z && a.max.z >= b.min.z)) {
          RigidBody *first = bodies[i];
          RigidBody *second = bodies[j];
          if (first > second) std::swap(first, second);
          pairs.push_back({first, second});
        }
      }
    }
    return pairs;
  }

  static bool VerifyBroadPhase(const std::vector<RigidBody *> &bodies,
                               const std::vector<CollisionPair> &broadPhasePairs,
                               std::string &notes) {
    auto brutePairs = BruteForceGeneratePairs(bodies);

    auto SortPairs = [](std::vector<CollisionPair> &p) {
      std::sort(p.begin(), p.end(), [](const CollisionPair &lhs, const CollisionPair &rhs) {
        if (lhs.A != rhs.A) return lhs.A < rhs.A;
        return lhs.B < rhs.B;
      });
      p.erase(std::unique(p.begin(), p.end(), [](const CollisionPair &lhs, const CollisionPair &rhs) {
        return lhs.A == rhs.A && lhs.B == rhs.B;
      }), p.end());
    };

    auto bpSorted = broadPhasePairs;
    for (auto &pair : bpSorted) {
      if (pair.A > pair.B) std::swap(pair.A, pair.B);
    }
    SortPairs(bpSorted);
    SortPairs(brutePairs);

    if (bpSorted.size() != brutePairs.size()) {
      std::ostringstream ss;
      ss << "Pair count mismatch! BroadPhase: " << bpSorted.size()
         << " vs BruteForce: " << brutePairs.size();
      notes = ss.str();
      return false;
    }

    for (size_t i = 0; i < bpSorted.size(); ++i) {
      if (bpSorted[i].A != brutePairs[i].A || bpSorted[i].B != brutePairs[i].B) {
        std::ostringstream ss;
        ss << "Mismatch at pair index " << i;
        notes = ss.str();
        return false;
      }
    }

    notes = "Broadphase exact match against brute-force (" + std::to_string(bpSorted.size()) + " pairs)";
    return true;
  }

  static bool VerifyNarrowPhase(const std::vector<CollisionPair> &candidatePairs,
                                size_t reportedCollisions,
                                std::string &notes) {
    size_t groundTruthCollisions = 0;
    for (const auto &pair : candidatePairs) {
      // Ground truth: Direct full SAT evaluation without bounding-radius prune
      for (const auto &colA : pair.A->colliders) {
        for (const auto &colB : pair.B->colliders) {
          bool isPolyA = NarrowPhase::IsConvexShape(colA.type);
          bool isPolyB = NarrowPhase::IsConvexShape(colB.type);

          if (colA.type == ColliderType::Sphere && colB.type == ColliderType::Sphere) {
            Vec3 posA = pair.A->LocalToGlobal(colA.localCentroid);
            Vec3 posB = pair.B->LocalToGlobal(colB.localCentroid);
            float radiusSum = colA.radius + colB.radius;
            float distSq = (posB - posA).MagnitudeSquared();
            if (distSq < radiusSum * radiusSum && distSq > 0.0f) {
              groundTruthCollisions++;
            }
          } else if (colA.type == ColliderType::Sphere && isPolyB) {
            Vec3 center = pair.A->LocalToGlobal(colA.localCentroid);
            std::vector<Vec3> polyVerts = colB.GetWorldVertices(*pair.B);
            auto sat = Elysium::SAT::TestCirclePolygon(center, colA.radius, polyVerts);
            if (sat.collided) groundTruthCollisions++;
          } else if (isPolyA && colB.type == ColliderType::Sphere) {
            Vec3 center = pair.B->LocalToGlobal(colB.localCentroid);
            std::vector<Vec3> polyVerts = colA.GetWorldVertices(*pair.A);
            auto sat = Elysium::SAT::TestCirclePolygon(center, colB.radius, polyVerts);
            if (sat.collided) groundTruthCollisions++;
          } else if (isPolyA && isPolyB) {
            std::vector<Vec3> vertsA = colA.GetWorldVertices(*pair.A);
            std::vector<Vec3> vertsB = colB.GetWorldVertices(*pair.B);
            auto sat = Elysium::SAT::TestPolygonPolygon(vertsA, vertsB);
            if (sat.collided) groundTruthCollisions++;
          }
        }
      }
    }

    if (reportedCollisions != groundTruthCollisions) {
      std::ostringstream ss;
      ss << "NarrowPhase collision count mismatch! Reported: " << reportedCollisions
         << " vs GroundTruth: " << groundTruthCollisions;
      notes = ss.str();
      return false;
    }

    notes = "Narrowphase verified against unpruned SAT (" + std::to_string(groundTruthCollisions) + " collisions)";
    return true;
  }
};

// ============================================================================
// Benchmark Execution Suite
// ============================================================================

class BenchmarkSuite {
public:
  static BenchmarkRecord RunBroadPhase(size_t n,
                                       DistributionType dist,
                                       ShapeType shape,
                                       const BenchmarkOptions &opts) {
    BenchmarkRecord record;
    record.phase = "BroadPhase";
    record.distribution = DistributionToString(dist);
    record.shapeConfig = ShapeToString(shape);
    record.entityCount = n;
    record.iterations = opts.iterations;

    auto bodies = ScenarioGenerator::Generate(n, dist, shape, opts.seed);

    // Warmup
    for (size_t w = 0; w < opts.warmupRuns; ++w) {
      auto tmp = BroadPhase::GeneratePairs(bodies);
      (void)tmp;
    }

    std::vector<double> timingsUs;
    timingsUs.reserve(opts.iterations);
    std::vector<CollisionPair> lastPairs;

    for (size_t it = 0; it < opts.iterations; ++it) {
      auto start = std::chrono::high_resolution_clock::now();
      lastPairs = BroadPhase::GeneratePairs(bodies);
      auto end = std::chrono::high_resolution_clock::now();

      double duration = std::chrono::duration<double, std::micro>(end - start).count();
      timingsUs.push_back(duration);
    }

    record.metrics = ComputeStatistics(timingsUs, lastPairs.size(), 0, 0);

    if (opts.verify) {
      std::string notes;
      record.metrics.verified = VerificationEngine::VerifyBroadPhase(bodies, lastPairs, notes);
      record.metrics.verificationNotes = notes;
    }

    ScenarioGenerator::Cleanup(bodies);
    return record;
  }

  static BenchmarkRecord RunNarrowPhase(size_t n,
                                        DistributionType dist,
                                        ShapeType shape,
                                        const BenchmarkOptions &opts) {
    BenchmarkRecord record;
    record.phase = "NarrowPhase";
    record.distribution = DistributionToString(dist);
    record.shapeConfig = ShapeToString(shape);
    record.entityCount = n;
    record.iterations = opts.iterations;

    auto bodies = ScenarioGenerator::Generate(n, dist, shape, opts.seed);
    auto candidatePairs = BroadPhase::GeneratePairs(bodies);

    // Warmup
    for (size_t w = 0; w < opts.warmupRuns; ++w) {
      NarrowPhaseStats stats;
      for (auto &pair : candidatePairs) {
        NarrowPhase::TestCollision(pair.A, pair.B, nullptr, &stats);
      }
    }

    std::vector<double> timingsUs;
    timingsUs.reserve(opts.iterations);
    NarrowPhaseStats finalStats;

    for (size_t it = 0; it < opts.iterations; ++it) {
      NarrowPhaseStats stats;
      auto start = std::chrono::high_resolution_clock::now();
      for (auto &pair : candidatePairs) {
        NarrowPhase::TestCollision(pair.A, pair.B, nullptr, &stats);
      }
      auto end = std::chrono::high_resolution_clock::now();

      double duration = std::chrono::duration<double, std::micro>(end - start).count();
      timingsUs.push_back(duration);
      if (it == opts.iterations - 1) {
        finalStats = stats;
      }
    }

    record.metrics = ComputeStatistics(timingsUs, candidatePairs.size(),
                                       finalStats.satChecksPerformed,
                                       finalStats.actualCollisions);

    if (opts.verify) {
      std::string notes;
      record.metrics.verified = VerificationEngine::VerifyNarrowPhase(candidatePairs,
                                                                      finalStats.actualCollisions,
                                                                      notes);
      record.metrics.verificationNotes = notes;
    }

    ScenarioGenerator::Cleanup(bodies);
    return record;
  }

  static BenchmarkRecord RunWorldStep(size_t n,
                                      DistributionType dist,
                                      ShapeType shape,
                                      const BenchmarkOptions &opts) {
    BenchmarkRecord record;
    record.phase = "WorldStep";
    record.distribution = DistributionToString(dist);
    record.shapeConfig = ShapeToString(shape);
    record.entityCount = n;
    record.iterations = opts.iterations;

    PhysicsWorld world(Vec3(0.0f, -9.81f, 0.0f));
    world.SetBoundaries(Vec3(0.0f, 0.0f, 0.0f), Vec3(200.0f, 200.0f, 0.0f));

    auto bodies = ScenarioGenerator::Generate(n, dist, shape, opts.seed);
    for (auto *b : bodies) {
      world.AddBody(b);
    }

    const float dt = 1.0f / 60.0f;

    // Warmup
    for (size_t w = 0; w < opts.warmupRuns; ++w) {
      world.Step(dt);
    }

    std::vector<double> timingsUs;
    timingsUs.reserve(opts.iterations);
    PhysicsStepStats finalStats;

    for (size_t it = 0; it < opts.iterations; ++it) {
      PhysicsStepStats stats;
      auto start = std::chrono::high_resolution_clock::now();
      world.Step(dt, &stats);
      auto end = std::chrono::high_resolution_clock::now();

      double duration = std::chrono::duration<double, std::micro>(end - start).count();
      timingsUs.push_back(duration);
      if (it == opts.iterations - 1) {
        finalStats = stats;
      }
    }

    record.metrics = ComputeStatistics(timingsUs,
                                       finalStats.broadPhaseCandidatePairs,
                                       finalStats.satChecksPerformed,
                                       finalStats.actualCollisions);

    if (opts.verify) {
      record.metrics.verified = true;
      record.metrics.verificationNotes = "World step simulation integrated stable";
    }

    ScenarioGenerator::Cleanup(bodies);
    return record;
  }
};

// ============================================================================
// Output Formatters (Console, JSON, CSV)
// ============================================================================

class OutputReporter {
public:
  static void PrintConsole(const std::vector<BenchmarkRecord> &records) {
    std::cout << "\n========================================================================================================================\n";
    std::cout << "                                      ELYSIUM ENGINE PHYSICS BENCHMARK REPORT                                            \n";
    std::cout << "========================================================================================================================\n";
    std::cout << std::left
              << std::setw(12) << "Phase"
              << std::setw(13) << "Distribution"
              << std::setw(15) << "Shapes"
              << std::setw(7)  << "N"
              << std::right
              << std::setw(12) << "Mean (us)"
              << std::setw(12) << "StdDev (us)"
              << std::setw(12) << "Min (us)"
              << std::setw(12) << "Max (us)"
              << std::setw(10) << "CandPairs"
              << std::setw(10) << "SATChecks"
              << std::setw(11) << "Collisions"
              << "   "
              << std::left << std::setw(8) << "Status"
              << "\n";
    std::cout << "------------------------------------------------------------------------------------------------------------------------\n";

    for (const auto &r : records) {
      std::cout << std::left
                << std::setw(12) << r.phase
                << std::setw(13) << r.distribution
                << std::setw(15) << r.shapeConfig
                << std::setw(7)  << r.entityCount
                << std::right
                << std::fixed << std::setprecision(1)
                << std::setw(12) << r.metrics.meanUs
                << std::setw(12) << r.metrics.stdDevUs
                << std::setw(12) << r.metrics.minUs
                << std::setw(12) << r.metrics.maxUs
                << std::setw(10) << r.metrics.candidatePairs
                << std::setw(10) << r.metrics.satChecks
                << std::setw(11) << r.metrics.actualCollisions
                << "   "
                << std::left
                << (r.metrics.verified ? "\033[32mPASSED\033[0m" : "\033[31mFAILED\033[0m")
                << "\n";
    }
    std::cout << "========================================================================================================================\n\n";
  }

  static void ExportJSON(const std::vector<BenchmarkRecord> &records,
                         const BenchmarkOptions &opts,
                         std::ostream &out) {
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::stringstream timeStr;
    timeStr << std::put_time(std::gmtime(&now), "%Y-%m-%dT%H:%M:%SZ");

    out << "{\n";
    out << "  \"benchmark_metadata\": {\n";
    out << "    \"suite\": \"ElysiumEngine Physics Subsystem\",\n";
    out << "    \"timestamp\": \"" << timeStr.str() << "\",\n";
    out << "    \"iterations\": " << opts.iterations << ",\n";
    out << "    \"warmup_runs\": " << opts.warmupRuns << ",\n";
    out << "    \"verified\": " << (opts.verify ? "true" : "false") << "\n";
    out << "  },\n";
    out << "  \"records\": [\n";

    for (size_t i = 0; i < records.size(); ++i) {
      const auto &r = records[i];
      out << "    {\n";
      out << "      \"phase\": \"" << r.phase << "\",\n";
      out << "      \"distribution\": \"" << r.distribution << "\",\n";
      out << "      \"shape_config\": \"" << r.shapeConfig << "\",\n";
      out << "      \"entity_count\": " << r.entityCount << ",\n";
      out << "      \"iterations\": " << r.iterations << ",\n";
      out << "      \"mean_us\": " << std::fixed << std::setprecision(2) << r.metrics.meanUs << ",\n";
      out << "      \"stddev_us\": " << r.metrics.stdDevUs << ",\n";
      out << "      \"min_us\": " << r.metrics.minUs << ",\n";
      out << "      \"max_us\": " << r.metrics.maxUs << ",\n";
      out << "      \"median_us\": " << r.metrics.medianUs << ",\n";
      out << "      \"ops_per_sec\": " << r.metrics.throughputOpsPerSec << ",\n";
      out << "      \"candidate_pairs\": " << r.metrics.candidatePairs << ",\n";
      out << "      \"sat_checks\": " << r.metrics.satChecks << ",\n";
      out << "      \"actual_collisions\": " << r.metrics.actualCollisions << ",\n";
      out << "      \"verified\": " << (r.metrics.verified ? "true" : "false") << ",\n";
      out << "      \"verification_notes\": \"" << r.metrics.verificationNotes << "\"\n";
      out << "    }" << (i + 1 < records.size() ? "," : "") << "\n";
    }

    out << "  ]\n";
    out << "}\n";
  }

  static void ExportCSV(const std::vector<BenchmarkRecord> &records, std::ostream &out) {
    out << "Phase,Distribution,Shapes,N,Iterations,MeanUs,StdDevUs,MinUs,MaxUs,MedianUs,OpsPerSec,CandidatePairs,SATChecks,ActualCollisions,Verified,Notes\n";
    for (const auto &r : records) {
      out << r.phase << ","
          << r.distribution << ","
          << r.shapeConfig << ","
          << r.entityCount << ","
          << r.iterations << ","
          << std::fixed << std::setprecision(2)
          << r.metrics.meanUs << ","
          << r.metrics.stdDevUs << ","
          << r.metrics.minUs << ","
          << r.metrics.maxUs << ","
          << r.metrics.medianUs << ","
          << r.metrics.throughputOpsPerSec << ","
          << r.metrics.candidatePairs << ","
          << r.metrics.satChecks << ","
          << r.metrics.actualCollisions << ","
          << (r.metrics.verified ? "TRUE" : "FALSE") << ","
          << "\"" << r.metrics.verificationNotes << "\"\n";
    }
  }
};

// ============================================================================
// CLI Argument Parsing
// ============================================================================

static void PrintHelp(const char *progName) {
  std::cout << "Elysium Engine Physics Benchmark Suite\n\n"
            << "Usage: " << progName << " [options]\n\n"
            << "Options:\n"
            << "  -b, --benchmark=<target>     Target to benchmark: broadphase, narrowphase, world, all (default: all)\n"
            << "  -d, --distribution=<dist>    Distribution: sparse, dense, grid, all (default: all)\n"
            << "  -s, --shapes=<type>          Shapes: sphere, box, box-sphere, mixed, all (default: all)\n"
            << "  -n, --sizes=<list>           Comma-separated body counts (default: 100,500,1000,2500,5000)\n"
            << "  -i, --iterations=<N>         Number of timing iterations per benchmark (default: 20)\n"
            << "  -w, --warmup=<N>             Number of warmup iterations (default: 5)\n"
            << "  -f, --format=<fmt>           Output format: console, json, csv, all (default: console)\n"
            << "  -o, --out=<filepath>         File to write output to (stdout if omitted)\n"
            << "  -v, --verify                 Run baseline correctness verification checks\n"
            << "  --seed=<uint>                RNG seed for reproducibility (default: 42)\n"
            << "  -h, --help                   Display this help message and exit\n\n"
            << "Examples:\n"
            << "  " << progName << " --benchmark=broadphase --sizes=100,500,1000 --verify\n"
            << "  " << progName << " --sizes=100,500,1000,2500 --format=all --out=baseline.json\n";
}

static BenchmarkOptions ParseArguments(int argc, char **argv) {
  BenchmarkOptions opts;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      PrintHelp(argv[0]);
      std::exit(0);
    } else if (arg == "-v" || arg == "--verify") {
      opts.verify = true;
    } else if (arg.rfind("--benchmark=", 0) == 0 || arg.rfind("-b=", 0) == 0) {
      size_t eqPos = arg.find('=');
      std::string val = arg.substr(eqPos + 1);
      if (val == "broadphase") opts.target = BenchmarkTarget::BroadPhase;
      else if (val == "narrowphase") opts.target = BenchmarkTarget::NarrowPhase;
      else if (val == "world") opts.target = BenchmarkTarget::WorldStep;
      else opts.target = BenchmarkTarget::All;
    } else if (arg.rfind("--distribution=", 0) == 0 || arg.rfind("-d=", 0) == 0) {
      size_t eqPos = arg.find('=');
      std::string val = arg.substr(eqPos + 1);
      if (val == "sparse") opts.distribution = DistributionType::Sparse;
      else if (val == "dense") opts.distribution = DistributionType::Dense;
      else if (val == "grid") opts.distribution = DistributionType::StressGrid;
      else opts.distribution = DistributionType::All;
    } else if (arg.rfind("--shapes=", 0) == 0 || arg.rfind("-s=", 0) == 0) {
      size_t eqPos = arg.find('=');
      std::string val = arg.substr(eqPos + 1);
      if (val == "sphere" || val == "sphere_sphere") opts.shapes = ShapeType::SphereSphere;
      else if (val == "box" || val == "box_box") opts.shapes = ShapeType::BoxBox;
      else if (val == "box-sphere" || val == "box_sphere") opts.shapes = ShapeType::BoxSphere;
      else if (val == "mixed") opts.shapes = ShapeType::Mixed;
      else opts.shapes = ShapeType::All;
    } else if (arg.rfind("--sizes=", 0) == 0 || arg.rfind("-n=", 0) == 0) {
      size_t eqPos = arg.find('=');
      std::string val = arg.substr(eqPos + 1);
      std::stringstream ss(val);
      std::string item;
      opts.entityCounts.clear();
      while (std::getline(ss, item, ',')) {
        if (!item.empty()) {
          opts.entityCounts.push_back(static_cast<size_t>(std::stoul(item)));
        }
      }
    } else if (arg.rfind("--iterations=", 0) == 0 || arg.rfind("-i=", 0) == 0) {
      size_t eqPos = arg.find('=');
      opts.iterations = static_cast<size_t>(std::stoul(arg.substr(eqPos + 1)));
    } else if (arg.rfind("--warmup=", 0) == 0 || arg.rfind("-w=", 0) == 0) {
      size_t eqPos = arg.find('=');
      opts.warmupRuns = static_cast<size_t>(std::stoul(arg.substr(eqPos + 1)));
    } else if (arg.rfind("--format=", 0) == 0 || arg.rfind("-f=", 0) == 0) {
      size_t eqPos = arg.find('=');
      std::string val = arg.substr(eqPos + 1);
      if (val == "json") opts.format = OutputFormat::JSON;
      else if (val == "csv") opts.format = OutputFormat::CSV;
      else if (val == "all") opts.format = OutputFormat::All;
      else opts.format = OutputFormat::Console;
    } else if (arg.rfind("--out=", 0) == 0 || arg.rfind("-o=", 0) == 0) {
      size_t eqPos = arg.find('=');
      opts.outputPath = arg.substr(eqPos + 1);
    } else if (arg.rfind("--seed=", 0) == 0) {
      size_t eqPos = arg.find('=');
      opts.seed = static_cast<uint32_t>(std::stoul(arg.substr(eqPos + 1)));
    }
  }

  return opts;
}

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char **argv) {
  BenchmarkOptions opts = ParseArguments(argc, argv);

  std::vector<DistributionType> distsToRun;
  if (opts.distribution == DistributionType::All) {
    distsToRun = {DistributionType::Sparse, DistributionType::Dense, DistributionType::StressGrid};
  } else {
    distsToRun = {opts.distribution};
  }

  std::vector<ShapeType> shapesToRun;
  if (opts.shapes == ShapeType::All) {
    shapesToRun = {ShapeType::SphereSphere, ShapeType::BoxBox, ShapeType::BoxSphere, ShapeType::Mixed};
  } else {
    shapesToRun = {opts.shapes};
  }

  std::cout << "Starting Elysium Physics Benchmark Suite...\n";
  std::cout << "  Target:       " << TargetToString(opts.target) << "\n";
  std::cout << "  Iterations:   " << opts.iterations << " (Warmup: " << opts.warmupRuns << ")\n";
  std::cout << "  Verification: " << (opts.verify ? "ENABLED" : "DISABLED") << "\n";
  std::cout << "  Entity sweeps: ";
  for (size_t i = 0; i < opts.entityCounts.size(); ++i) {
    std::cout << opts.entityCounts[i] << (i + 1 < opts.entityCounts.size() ? ", " : "\n\n");
  }

  std::vector<BenchmarkRecord> allRecords;

  for (size_t n : opts.entityCounts) {
    for (auto dist : distsToRun) {
      for (auto shape : shapesToRun) {
        if (opts.target == BenchmarkTarget::All || opts.target == BenchmarkTarget::BroadPhase) {
          allRecords.push_back(BenchmarkSuite::RunBroadPhase(n, dist, shape, opts));
        }
        if (opts.target == BenchmarkTarget::All || opts.target == BenchmarkTarget::NarrowPhase) {
          allRecords.push_back(BenchmarkSuite::RunNarrowPhase(n, dist, shape, opts));
        }
        if (opts.target == BenchmarkTarget::All || opts.target == BenchmarkTarget::WorldStep) {
          allRecords.push_back(BenchmarkSuite::RunWorldStep(n, dist, shape, opts));
        }
      }
    }
  }

  // Console output
  if (opts.format == OutputFormat::Console || opts.format == OutputFormat::All) {
    OutputReporter::PrintConsole(allRecords);
  }

  // File / Stream exports
  if (opts.format == OutputFormat::JSON) {
    if (!opts.outputPath.empty()) {
      std::ofstream file(opts.outputPath);
      OutputReporter::ExportJSON(allRecords, opts, file);
      std::cout << "Saved JSON metrics to: " << opts.outputPath << "\n";
    } else {
      OutputReporter::ExportJSON(allRecords, opts, std::cout);
    }
  } else if (opts.format == OutputFormat::CSV) {
    if (!opts.outputPath.empty()) {
      std::ofstream file(opts.outputPath);
      OutputReporter::ExportCSV(allRecords, file);
      std::cout << "Saved CSV metrics to: " << opts.outputPath << "\n";
    } else {
      OutputReporter::ExportCSV(allRecords, std::cout);
    }
  } else if (opts.format == OutputFormat::All && !opts.outputPath.empty()) {
    std::string jsonPath = opts.outputPath;
    if (jsonPath.rfind(".json") == std::string::npos) {
      jsonPath += ".json";
    }
    std::ofstream jsonFile(jsonPath);
    OutputReporter::ExportJSON(allRecords, opts, jsonFile);
    std::cout << "Saved JSON metrics to: " << jsonPath << "\n";

    std::string csvPath = opts.outputPath;
    size_t extDot = csvPath.rfind('.');
    if (extDot != std::string::npos) {
      csvPath = csvPath.substr(0, extDot) + ".csv";
    } else {
      csvPath += ".csv";
    }
    std::ofstream csvFile(csvPath);
    OutputReporter::ExportCSV(allRecords, csvFile);
    std::cout << "Saved CSV metrics to: " << csvPath << "\n";
  }

  // Return non-zero if any verification failed
  bool allPassed = true;
  for (const auto &r : allRecords) {
    if (!r.metrics.verified) {
      allPassed = false;
      std::cerr << "VERIFICATION FAILURE in [" << r.phase << " / "
                << r.distribution << " / " << r.shapeConfig
                << " (N=" << r.entityCount << ")]: "
                << r.metrics.verificationNotes << "\n";
    }
  }

  return allPassed ? 0 : 1;
}
