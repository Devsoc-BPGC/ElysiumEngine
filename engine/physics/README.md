# Elysium Engine — 2D Physics Subsystem

The `engine/physics` module provides continuous 2D rigid body dynamics, broadphase spatial acceleration, and narrowphase contact manifold detection.

---

## Directory Contents

| Header | Source | Description |
|---|---|---|
| [`RigidBody.hpp`](include/RigidBody.hpp) | [`RigidBody.cpp`](src/RigidBody.cpp) | RigidBody structure: mass, inverse mass, inertia tensors, linear/angular velocity, friction, restitution, and colliders. |
| [`Collider.hpp`](include/Collider.hpp) | [`Collider.cpp`](src/Collider.cpp) | Collider shapes (`Sphere` / `Box`), density, mass, and local centroid offsets. |
| [`PhysicsWorld.hpp`](include/PhysicsWorld.hpp) | [`PhysicsWorld.cpp`](src/PhysicsWorld.cpp) | Physics simulation manager: integration, gravity, boundary constraints, and contact resolution loop. |
| [`BroadPhase.hpp`](include/BroadPhase.hpp) | — | Uniform spatial hash grid for pruning non-colliding entity pairs in $O(N)$ expected time. |
| [`NarrowPhase.hpp`](include/NarrowPhase.hpp) | — | Separating Axis Theorem (SAT) collision tests for sphere-sphere, box-box, and sphere-box. |
| [`AABB.hpp`](include/AABB.hpp) | [`AABB.cpp`](src/AABB.cpp) | 2D Axis-Aligned Bounding Box calculations for shapes and bodies. |
| [`CollisionPair.hpp`](include/CollisionPair.hpp) | — | Potential collision pair container for broadphase generation. |
| [`CoreMath.hpp`](include/CoreMath.hpp) | — | Vector and matrix math primitives: `Vec2`, `Vec3`, `Mat3`, and `Quat`. |

---

## Physics Pipeline

1. **Force & Torque Integration**:
   - `RigidBody::Integrate(dt)` applies accumulated linear forces and torques.
   - For static bodies (`inverseMass == 0.0f` or `isStatic == true`), velocities and position remain stationary.
2. **BroadPhase Pruning**:
   - `BroadPhase` hashes all rigid bodies into a spatial grid (`BroadPhase::CELL_SIZE = 2.0m`).
   - Generates candidate `CollisionPair`s only for bodies sharing common grid cells.
3. **NarrowPhase SAT Testing**:
   - `NarrowPhase::ResolveCollision()` evaluates precise overlap, contact points, penetration depths, and collision normals.
   - Applies impulse resolution according to restitution and friction coefficients.
4. **Boundary Box Constraints**:
   - `PhysicsWorld` evaluates optional rotating bounding box constraints (`boundaryHalfExtents`, `boundaryRotation`).
5. **Transform Sync**:
   - After stepping, `GameObject::SyncPhysics()` updates entity positions and rotations to match the physics simulation.

---

## Benchmarking Suite (`PhysicsBenchmark`)

Elysium Engine includes a comprehensive, reproducible C++ benchmarking suite to establish baseline performance metrics and track optimizations across broadphase and narrowphase systems.

### Building the Benchmark Target
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=ON
cmake --build build --target PhysicsBenchmark -j$(nproc)
```

### Running Benchmarks
- **Quick Smoke Test with Verification**:
  ```bash
  ./build/bin/PhysicsBenchmark --sizes=100,500 --verify
  ```
- **Parametric Scaling Sweep with JSON & CSV Export**:
  ```bash
  ./build/bin/PhysicsBenchmark --sizes=100,500,1000,2500,5000 --format=all --out=physics_baseline.json --verify
  ```
- **Target Selection**:
  - `--benchmark=<all|broadphase|narrowphase|world>`
  - `--distribution=<all|sparse|dense|grid>`
  - `--shapes=<all|sphere|box|box-sphere|mixed>`

See [`docs/PHYSICS_BENCHMARK.md`](../../docs/PHYSICS_BENCHMARK.md) for full CLI flags, JSON schema, and CI/CD optimization comparison workflows.

