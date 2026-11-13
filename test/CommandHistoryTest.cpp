#include "Command.hpp"
#include "CommandHistory.hpp"
#include "EditorContext.hpp"
#include "EditorHotkeys.hpp"
#include "GameObject.hpp"
#include "Log.h"
#include "Scene.hpp"
#include <cmath>
#include <iostream>
#include <memory>

namespace {

int g_failures = 0;

void Check(bool ok, const char *what) {
  if (!ok) {
    std::cout << "FAILED: " << what << std::endl;
    ++g_failures;
  }
}

// Simple test command incrementing and decrementing an integer
class CounterCommand : public Elysium::ICommand {
public:
  CounterCommand(int &counter, int delta, std::string name = "Counter")
      : m_counter(counter), m_delta(delta), m_name(std::move(name)) {}

  void Execute() override { m_counter += m_delta; }
  void Undo() override { m_counter -= m_delta; }
  std::string GetName() const override { return m_name; }

private:
  int &m_counter;
  int m_delta;
  std::string m_name;
};

void TestBasicCommandHistory() {
  Elysium::CommandHistory history(3); // Max depth 3

  Check(!history.CanUndo(), "History cannot undo initially");
  Check(!history.CanRedo(), "History cannot redo initially");
  Check(history.GetUndoCount() == 0, "Undo count is 0");
  Check(history.GetRedoCount() == 0, "Redo count is 0");

  int counter = 0;

  // 1. Execute command
  history.ExecuteCommand(std::make_unique<CounterCommand>(counter, 5, "Add 5"));
  Check(counter == 5, "Counter is 5 after execute");
  Check(history.CanUndo(), "Can undo after execute");
  Check(!history.CanRedo(), "Cannot redo after execute");
  Check(history.GetUndoCommandName() == "Add 5", "Top undo name matches");

  // 2. Undo
  bool undone = history.Undo();
  Check(undone, "Undo succeeded");
  Check(counter == 0, "Counter is 0 after undo");
  Check(!history.CanUndo(), "Cannot undo after reverting all");
  Check(history.CanRedo(), "Can redo after undo");
  Check(history.GetRedoCommandName() == "Add 5", "Top redo name matches");

  // 3. Redo
  bool redone = history.Redo();
  Check(redone, "Redo succeeded");
  Check(counter == 5, "Counter is 5 after redo");
  Check(history.CanUndo(), "Can undo after redo");
  Check(!history.CanRedo(), "Cannot redo after redo completes");

  // 4. Max depth trimming: push 4 more commands with maxDepth=3
  history.ExecuteCommand(std::make_unique<CounterCommand>(counter, 1, "Add 1"));
  history.ExecuteCommand(std::make_unique<CounterCommand>(counter, 2, "Add 2"));
  history.ExecuteCommand(std::make_unique<CounterCommand>(counter, 3, "Add 3"));
  history.ExecuteCommand(std::make_unique<CounterCommand>(counter, 4, "Add 4"));

  Check(history.GetUndoCount() == 3, "Undo stack clamped to max depth 3");

  // 5. Locking in Play mode
  history.SetLocked(true);
  Check(!history.CanUndo(), "Cannot undo while locked");
  Check(!history.CanRedo(), "Cannot redo while locked");
  Check(!history.Undo(), "Undo returns false while locked");
  history.SetLocked(false);
  Check(history.CanUndo(), "Can undo after unlocking");
}

void TestTransformCommand() {
  Scene scene;
  auto entity = std::make_shared<GameObject>("Box");
  entity->position = Vec3(0.0f, 0.0f, 0.0f);
  entity->scale = Vec3(1.0f, 1.0f, 1.0f);
  entity->CreateRigidBody();
  scene.AddGameObject(entity);

  Elysium::CommandHistory history;

  Vec3 oldPos(0.0f, 0.0f, 0.0f);
  Vec3 newPos(10.0f, 5.0f, 0.0f);
  Quat rot;
  Vec3 scale(1.0f, 1.0f, 1.0f);

  auto cmd = std::make_unique<Elysium::TransformCommand>(
      scene, entity->GetUUID(), oldPos, rot, scale, newPos, rot, scale, "Translate Box");

  history.ExecuteCommand(std::move(cmd));

  Check(std::abs(entity->position.x - 10.0f) < 0.001f, "Entity X is 10 after execute");
  Check(std::abs(entity->position.y - 5.0f) < 0.001f, "Entity Y is 5 after execute");
  Check(std::abs(entity->rigidBody->position.x - 10.0f) < 0.001f, "RigidBody X synced to 10");

  history.Undo();
  Check(std::abs(entity->position.x - 0.0f) < 0.001f, "Entity X is 0 after undo");
  Check(std::abs(entity->rigidBody->position.x - 0.0f) < 0.001f, "RigidBody X restored to 0");

  history.Redo();
  Check(std::abs(entity->position.x - 10.0f) < 0.001f, "Entity X is 10 after redo");
  Check(std::abs(entity->rigidBody->position.x - 10.0f) < 0.001f, "RigidBody X redone to 10");
}

void TestPropertyChangeCommand() {
  Scene scene;
  auto entity = std::make_shared<GameObject>("Player");
  entity->CreateRigidBody();
  entity->rigidBody->mass = 2.0f;
  scene.AddGameObject(entity);

  Elysium::CommandHistory history;

  auto cmd = std::make_unique<Elysium::PropertyChangeCommand<float>>(
      scene, entity->GetUUID(), 2.0f, 15.0f,
      [](GameObject &g, const float &val) {
        if (g.rigidBody) g.rigidBody->mass = val;
      },
      "Change Mass");

  history.ExecuteCommand(std::move(cmd));
  Check(std::abs(entity->rigidBody->mass - 15.0f) < 0.001f, "Mass is 15 after execute");

  history.Undo();
  Check(std::abs(entity->rigidBody->mass - 2.0f) < 0.001f, "Mass is 2 after undo");

  history.Redo();
  Check(std::abs(entity->rigidBody->mass - 15.0f) < 0.001f, "Mass is 15 after redo");
}

void TestReparentEntityCommand() {
  Scene scene;
  auto parent = std::make_shared<GameObject>("Parent");
  auto child = std::make_shared<GameObject>("Child");
  scene.AddGameObject(parent);
  scene.AddGameObject(child);

  Elysium::CommandHistory history;

  auto cmd = std::make_unique<Elysium::ReparentEntityCommand>(
      scene, child->GetUUID(), 0, parent->GetUUID(), "Reparent Child under Parent");

  history.ExecuteCommand(std::move(cmd));
  Check(child->GetTransform().ParentHandle == parent->GetUUID(), "Child parent handle matches parent UUID");

  history.Undo();
  Check(child->GetTransform().ParentHandle == 0, "Child parent handle is 0 after undo");

  history.Redo();
  Check(child->GetTransform().ParentHandle == parent->GetUUID(), "Child parent handle restored after redo");
}

void TestCreateAndDeleteEntityCommand() {
  Scene scene;
  auto entity = std::make_shared<GameObject>("ToDelete");
  entity->position = Vec3(3.0f, 4.0f, 0.0f);
  entity->CreateRigidBody();
  scene.AddGameObject(entity);

  Elysium::UUID origUUID = entity->GetUUID();
  Elysium::CommandHistory history;

  // 1. Delete entity command
  auto deleteCmd = std::make_unique<Elysium::DeleteEntityCommand>(scene, entity, "Delete ToDelete");
  history.ExecuteCommand(std::move(deleteCmd));

  Check(scene.GetEntityByUUID(origUUID) == nullptr, "Entity is gone from scene after delete execute");

  // Undo delete
  history.Undo();
  GameObject *restored = scene.GetEntityByUUID(origUUID);
  Check(restored != nullptr, "Entity is restored in scene after undo");
  Check(restored->name == "ToDelete", "Restored entity has original name");
  Check(restored->rigidBody != nullptr, "Restored entity has attached RigidBody");

  // Redo delete
  history.Redo();
  Check(scene.GetEntityByUUID(origUUID) == nullptr, "Entity is gone again after redo delete");

  // 2. Create entity command
  auto newEntity = std::make_shared<GameObject>("NewBorn");
  Elysium::UUID newBornUUID = newEntity->GetUUID();
  auto createCmd = std::make_unique<Elysium::CreateEntityCommand>(scene, newEntity, "Create NewBorn");

  history.ExecuteCommand(std::move(createCmd));
  Check(scene.GetEntityByUUID(newBornUUID) != nullptr, "Entity exists after create execute");

  history.Undo();
  Check(scene.GetEntityByUUID(newBornUUID) == nullptr, "Entity removed after create undo");

  history.Redo();
  Check(scene.GetEntityByUUID(newBornUUID) != nullptr, "Entity restored after create redo");
}

void TestDragCoalescing() {
  Scene scene;
  auto entity = std::make_shared<GameObject>("Mover");
  entity->position = Vec3(1.0f, 1.0f, 0.0f);
  scene.AddGameObject(entity);

  Elysium::CommandHistory history;

  // Simulate live drag:
  // Initial captured: (1, 1, 0)
  Vec3 initialPos(1.0f, 1.0f, 0.0f);
  Quat rot;
  Vec3 scale(1.0f, 1.0f, 1.0f);

  // During 60 FPS drag, interim updates apply directly to entity:
  entity->position = Vec3(1.5f, 1.0f, 0.0f);
  entity->position = Vec3(2.0f, 1.0f, 0.0f);
  entity->position = Vec3(2.5f, 1.0f, 0.0f);
  entity->position = Vec3(4.0f, 2.0f, 0.0f);
  entity->SyncTransform();

  // Mouse released: Push single coalesced command
  auto cmd = std::make_unique<Elysium::TransformCommand>(
      scene, entity->GetUUID(), initialPos, rot, scale, entity->position, rot, scale, "Drag Mover");

  history.PushExecutedCommand(std::move(cmd));

  Check(history.GetUndoCount() == 1, "Only 1 command pushed for the entire drag");
  Check(std::abs(entity->position.x - 4.0f) < 0.001f, "Entity is at final drag position");

  // Single Undo restores initial position before the drag started
  history.Undo();
  Check(std::abs(entity->position.x - 1.0f) < 0.001f, "Single undo restores pre-drag position (1, 1, 0)");
  Check(std::abs(entity->position.y - 1.0f) < 0.001f, "Single undo restores pre-drag Y position");
}

} // namespace

int main() {
  Elysium::Log::Init();

  TestBasicCommandHistory();
  TestTransformCommand();
  TestPropertyChangeCommand();
  TestReparentEntityCommand();
  TestCreateAndDeleteEntityCommand();
  TestDragCoalescing();

  if (g_failures == 0) {
    std::cout << "All CommandHistoryTest assertions PASSED successfully." << std::endl;
    return 0;
  }

  std::cout << g_failures << " assertion(s) FAILED in CommandHistoryTest." << std::endl;
  return 1;
}
