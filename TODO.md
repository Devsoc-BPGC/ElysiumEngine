# Elysium Engine TODO

## Physics Engine
- [x] Implement Box-Box collision resolution using Separating Axis Theorem (SAT).
- [x] Optimize BroadPhase with Spatial Hashing (current: $O(N)$ potential).
- [ ] Make world boundaries configurable in `PhysicsWorld` (current: hardcoded).
- [ ] Add support for Capsule colliders.
- [ ] Implement friction in impulse resolution.

## Core / Framework
- [ ] Improve Scene object cleanup to use `std::erase_if` (current: manual O(N) loop).
- [ ] Add a proper `Logger` system.
- [ ] Implement a `Prefab` or `Entity Factory` system.
- [ ] Add support for `FixedUpdate` to ensure stable physics simulation.

## Renderer
- [ ] Add debug drawing for colliders and AABBs.
- [ ] Implement a basic sprite batcher.
- [ ] Add particle system component.
