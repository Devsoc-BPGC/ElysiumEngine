#include "EntityHandle.hpp"
#include "GameObject.hpp"
#include "Log.h"
#include "Scene.hpp"
#include "UUID.hpp"
#include <cassert>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

void TestUUIDGenerationAndUniqueness() {
  std::unordered_set<uint64_t> uniqueIDs;
  const size_t testCount = 50000;

  for (size_t i = 0; i < testCount; ++i) {
    Elysium::UUID uuid;
    uint64_t val = static_cast<uint64_t>(uuid);
    assert(val != 0 && "Generated UUID must be non-zero");
    auto [_, inserted] = uniqueIDs.insert(val);
    assert(inserted && "Generated duplicate UUID!");
  }

  std::cout << "[PASS] TestUUIDGenerationAndUniqueness (tested " << testCount
            << " UUIDs)" << std::endl;
}

void TestUUIDComparisonsAndHashing() {
  Elysium::UUID id1(100);
  Elysium::UUID id2(100);
  Elysium::UUID id3(200);

  assert(id1 == id2);
  assert(id1 != id3);
  assert(id1 < id3);
  assert(id3 > id1);
  assert(id1 <= id2);
  assert(id1 >= id2);

  std::unordered_map<Elysium::UUID, std::string> map;
  map[id1] = "Entity One";
  map[id3] = "Entity Three";

  assert(map.size() == 2);
  assert(map[id2] == "Entity One");
  assert(map.find(id3) != map.end());
  assert(map.find(Elysium::UUID(999)) == map.end());

  std::cout << "[PASS] TestUUIDComparisonsAndHashing" << std::endl;
}

void TestGameObjectUUIDIntegration() {
  auto go1 = std::make_shared<GameObject>("DefaultUUID");
  auto go2 = std::make_shared<GameObject>("AnotherDefaultUUID");
  assert(static_cast<uint64_t>(go1->GetUUID()) != 0);
  assert(static_cast<uint64_t>(go2->GetUUID()) != 0);
  assert(go1->GetUUID() != go2->GetUUID());

  // Test explicit UUID passing (essential for deserialization)
  Elysium::UUID explicitId(1234567890ULL);
  auto goDeserialized =
      std::make_shared<GameObject>(explicitId, "DeserializedEntity");
  assert(goDeserialized->GetUUID() == explicitId);
  assert(static_cast<uint64_t>(goDeserialized->GetUUID()) == 1234567890ULL);

  std::cout << "[PASS] TestGameObjectUUIDIntegration" << std::endl;
}

void TestSceneEntityLookupAndDeletion() {
  Scene scene;

  auto ent1 = std::make_shared<GameObject>("Entity A");
  auto ent2 = std::make_shared<GameObject>("Entity B");
  auto ent3 = std::make_shared<GameObject>("Entity C");

  Elysium::UUID id1 = ent1->GetUUID();
  Elysium::UUID id2 = ent2->GetUUID();
  Elysium::UUID id3 = ent3->GetUUID();

  scene.AddGameObject(ent1);
  scene.AddGameObject(ent2);
  scene.AddGameObject(ent3);

  assert(scene.objects.size() == 3);
  assert(scene.m_EntityMap.size() == 3);

  // Fast lookup by UUID
  assert(scene.GetEntityByUUID(id1) == ent1.get());
  assert(scene.GetEntityByUUID(id2) == ent2.get());
  assert(scene.GetEntityByUUID(id3) == ent3.get());
  assert(scene.GetEntityByUUID(Elysium::UUID(999999)) == nullptr);

  // Deletion by UUID
  bool destroyed = scene.DestroyEntityByUUID(id2);
  assert(destroyed == true);
  assert(scene.GetEntityByUUID(id2) == nullptr);
  assert(scene.objects.size() == 2);
  assert(scene.m_EntityMap.size() == 2);

  // Deleting already deleted or non-existent entity
  bool destroyedAgain = scene.DestroyEntityByUUID(id2);
  assert(destroyedAgain == false);

  std::cout << "[PASS] TestSceneEntityLookupAndDeletion" << std::endl;
}

void TestEntityHandleResolution() {
  Scene scene;

  auto player = std::make_shared<GameObject>("Player");
  Elysium::UUID playerId = player->GetUUID();
  scene.AddGameObject(player);

  // Default invalid handle
  Elysium::EntityHandle<GameObject> nullHandle;
  assert(!nullHandle.IsValid());
  assert(!nullHandle);
  assert(nullHandle.Resolve(&scene) == nullptr);

  // Valid entity handle
  Elysium::EntityHandle<GameObject> playerHandle(playerId);
  assert(playerHandle.IsValid());
  assert(playerHandle.GetUUID() == playerId);
  assert(playerHandle.Resolve(&scene) == player.get());

  // EntityHandle constructed from GameObject pointer
  Elysium::EntityHandle<GameObject> handleFromPtr(player.get());
  assert(handleFromPtr == playerHandle);
  assert(handleFromPtr.Resolve(&scene) == player.get());

  // Component resolution
  auto target = std::make_shared<GameObject>("Target");
  Elysium::UUID targetId = target->GetUUID();
  scene.AddGameObject(target);

  Elysium::EntityHandle<GameObject> targetHandle(targetId);
  assert(targetHandle.Resolve(&scene) == target.get());

  // Destroy target in scene
  scene.DestroyEntityByUUID(targetId);

  // Handle resolution safely returns nullptr without crashing
  assert(targetHandle.Resolve(&scene) == nullptr);

  std::cout << "[PASS] TestEntityHandleResolution" << std::endl;
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  Elysium::Log::Init();
  std::cout << "Running Elysium UUID and EntityHandle Tests...\n";
  TestUUIDGenerationAndUniqueness();
  TestUUIDComparisonsAndHashing();
  TestGameObjectUUIDIntegration();
  TestSceneEntityLookupAndDeletion();
  TestEntityHandleResolution();
  std::cout << "\nAll UUID and EntityHandle tests PASSED successfully!\n";
  return 0;
}
