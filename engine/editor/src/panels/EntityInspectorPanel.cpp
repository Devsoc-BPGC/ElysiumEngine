#include "elypch.hpp"
#include "panels/EntityInspectorPanel.hpp"
#include "AssetManager.hpp"
#include "AudioSourceComponent.hpp"
#include "CameraComponent.hpp"
#include "Collider.hpp"
#include "FileDialog.hpp"
#include "Project.hpp"
#include "SpriteRenderer.hpp"
#include <cstring>
#include <imgui-SFML.h>

namespace Elysium {

static constexpr float PI_VAL = 3.14159265358979323846f;

EntityInspectorPanel::EntityInspectorPanel()
    : EditorPanel("Entity Inspector") {}

void EntityInspectorPanel::OnImGuiRender(EditorContext &context, Scene &scene,
                                         SimpleRenderer &renderer) {
  (void)renderer;

  if (ImGui::Begin("Entity Inspector", &isOpen)) {
    if (!context.selectedEntity) {
      ImGui::Spacing();
      ImGui::TextDisabled("No Entity Selected.");
      ImGui::TextWrapped("Select an entity from the Scene Hierarchy to view "
                         "and edit its components.");
      ImGui::End();
      return;
    }

    auto &entity = *context.selectedEntity;

    // Entity Header Info
    char nameBuffer[256];
    std::strncpy(nameBuffer, entity.name.c_str(), sizeof(nameBuffer) - 1);
    nameBuffer[sizeof(nameBuffer) - 1] = '\0';
    if (ImGui::InputText("Entity Name", nameBuffer, sizeof(nameBuffer))) {
      entity.name = nameBuffer;
    }

    ImGui::SameLine();
    ImGui::Checkbox("Active", &entity.active);

    ImGui::TextDisabled("Entity ID: %llu",
                        static_cast<unsigned long long>(static_cast<uint64_t>(entity.GetUUID())));
    ImGui::Separator();

    // 1. Transform Component
    DrawTransformComponent(context, entity);

    // 2. RigidBody2D Component
    DrawRigidBodyComponent(context, scene, entity);

    // 3. Collider2D Component
    DrawCollidersComponent(context, entity);

    // 4. SpriteRenderer Component
    DrawSpriteRendererComponent(context, entity);

    // 5. AudioSource Component
    DrawAudioSourceComponent(context, entity);

    // 6. Camera Component
    DrawCameraComponent(context, scene, entity);

    // 7. Custom Components
    DrawCustomComponents(context, entity);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Add Component Button
    if (ImGui::Button("+ Add Component", ImVec2(-1.0f, 0.0f))) {
      ImGui::OpenPopup("AddComponentPopup");
    }

    if (ImGui::BeginPopup("AddComponentPopup")) {
      if (!entity.rigidBody && ImGui::MenuItem("RigidBody2D")) {
        auto &rb = entity.CreateRigidBody();
        scene.physicsWorld.AddBody(&rb);
        ELYSIUM_INFO("Added RigidBody2D to entity: {}", entity.name);
      }

      if (entity.rigidBody && ImGui::MenuItem("Sphere Collider")) {
        entity.rigidBody->AddColliders(Collider::CreateSphere(0.5f, 1.0f));
        ELYSIUM_INFO("Added Sphere Collider to entity: {}", entity.name);
      }

      if (entity.rigidBody && ImGui::MenuItem("Box Collider")) {
        entity.rigidBody->AddColliders(
            Collider::CreateBox(Vec3(0.5f, 0.5f, 0.0f), 1.0f));
        ELYSIUM_INFO("Added Box Collider to entity: {}", entity.name);
      }

      if (!entity.GetComponent<SpriteRenderer>() &&
          ImGui::MenuItem("Sprite Renderer")) {
        entity.AddComponent<SpriteRenderer>(sf::Color(255, 100, 100),
                                            sf::Vector2f(1.0f, 1.0f));
        ELYSIUM_INFO("Added SpriteRenderer to entity: {}", entity.name);
      }

      if (!entity.GetComponent<AudioSourceComponent>() &&
          ImGui::MenuItem("Audio Source")) {
        entity.AddComponent<AudioSourceComponent>();
        ELYSIUM_INFO("Added AudioSourceComponent to entity: {}", entity.name);
      }

      if (!entity.GetComponent<CameraComponent>() &&
          ImGui::MenuItem("Camera")) {
        entity.AddComponent<CameraComponent>();
        ELYSIUM_INFO("Added CameraComponent to entity: {}", entity.name);
      }

      ImGui::EndPopup();
    }
  }
  ImGui::End();
}

void EntityInspectorPanel::DrawTransformComponent(EditorContext &context,
                                                  GameObject &entity) {
  (void)context;
  if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
    bool changed = false;

    // Position (meters)
    float pos[3] = {entity.position.x, entity.position.y, entity.position.z};
    if (ImGui::DragFloat3("Position (m)", pos, 0.05f)) {
      entity.position = Vec3(pos[0], pos[1], pos[2]);
      changed = true;
    }

    // Rotation (degrees around Z axis in 2D)
    Quat q = entity.rotation;
    float angleDeg = 2.0f * std::atan2(q.z, q.w) * 180.0f / PI_VAL;
    if (ImGui::DragFloat("Rotation (deg)", &angleDeg, 1.0f, -360.0f, 360.0f)) {
      float rad = angleDeg * PI_VAL / 180.0f;
      entity.rotation = Quat(Vec3(0, 0, 1), rad);
      changed = true;
    }

    // Scale
    float scale[3] = {entity.scale.x, entity.scale.y, entity.scale.z};
    if (ImGui::DragFloat3("Scale", scale, 0.05f, 0.01f, 100.0f)) {
      entity.scale = Vec3(scale[0], scale[1], scale[2]);
      changed = true;
    }

    // Immediately synchronize physics state with the transform changes
    if (changed) {
      entity.SyncTransform();
    }
  }
}

void EntityInspectorPanel::DrawRigidBodyComponent(EditorContext &context,
                                                  Scene &scene,
                                                  GameObject &entity) {
  (void)context;
  if (!entity.rigidBody)
    return;

  auto &body = entity.rigidBody;

  if (ImGui::CollapsingHeader("RigidBody2D", ImGuiTreeNodeFlags_DefaultOpen)) {
    // Body Type
    bool isStatic = body->isStatic;
    if (ImGui::Checkbox("Is Static", &isStatic)) {
      body->isStatic = isStatic;
      if (isStatic) {
        body->mass = 0.0f;
        body->inverseMass = 0.0f;
        body->linearVelocity = Vec3(0, 0, 0);
        body->angularVelocity = Vec3(0, 0, 0);
      } else {
        body->RecalculateMassProperties();
        if (body->mass <= 0.0f) {
          body->mass = 1.0f;
          body->inverseMass = 1.0f;
        }
      }
    }

    if (!body->isStatic) {
      // Mass
      if (ImGui::DragFloat("Mass (kg)", &body->mass, 0.1f, 0.01f, 5000.0f)) {
        if (body->mass > 0.0f) {
          body->inverseMass = 1.0f / body->mass;
        }
      }

      // Linear Velocity
      float vel[2] = {body->linearVelocity.x, body->linearVelocity.y};
      if (ImGui::DragFloat2("Linear Velocity (m/s)", vel, 0.1f)) {
        body->linearVelocity.x = vel[0];
        body->linearVelocity.y = vel[1];
      }

      // Angular Velocity
      float angVel = body->angularVelocity.z;
      if (ImGui::DragFloat("Angular Velocity (rad/s)", &angVel, 0.1f)) {
        body->angularVelocity.z = angVel;
      }
    }

    // Physics Material Properties
    ImGui::SliderFloat("Restitution", &body->restitution, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Friction", &body->friction, 0.0f, 1.0f, "%.2f");

    // Remove RigidBody Button
    if (ImGui::Button("Remove RigidBody", ImVec2(-1.0f, 0.0f))) {
      scene.physicsWorld.RemoveBody(body.get());
      entity.rigidBody.reset();
      ELYSIUM_INFO("Removed RigidBody from entity: {}", entity.name);
    }
  }
}

void EntityInspectorPanel::DrawCollidersComponent(EditorContext &context,
                                                  GameObject &entity) {
  (void)context;
  if (!entity.rigidBody)
    return;

  if (ImGui::CollapsingHeader("Colliders", ImGuiTreeNodeFlags_DefaultOpen)) {
    auto &colliders = entity.rigidBody->colliders;
    if (colliders.empty()) {
      ImGui::TextDisabled("No Colliders Attached.");
      return;
    }

    int colliderToDelete = -1;
    bool propertiesChanged = false;

    for (size_t i = 0; i < colliders.size(); ++i) {
      auto &col = colliders[i];
      ImGui::PushID(static_cast<int>(i));

      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                            ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                            ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
      if (ImGui::SmallButton("X")) {
        colliderToDelete = static_cast<int>(i);
      }
      ImGui::PopStyleColor(3);

      ImGui::SameLine();
      if (col.type == ColliderType::Sphere) {
        ImGui::Text("Sphere Collider #%zu", i + 1);
        if (ImGui::DragFloat("Radius (m)", &col.radius, 0.02f, 0.01f, 50.0f)) {
          col.RecalculateProperties();
          propertiesChanged = true;
        }
      } else if (col.type == ColliderType::Box) {
        ImGui::Text("Box Collider #%zu", i + 1);
        float ext[2] = {col.halfExtents.x, col.halfExtents.y};
        if (ImGui::DragFloat2("Half Extents (m)", ext, 0.02f, 0.01f, 50.0f)) {
          col.halfExtents.x = ext[0];
          col.halfExtents.y = ext[1];
          col.RecalculateProperties();
          propertiesChanged = true;
        }
      }

      // Local Offset (x, y)
      float offset[2] = {col.offset.x, col.offset.y};
      if (ImGui::DragFloat2("Offset (m)", offset, 0.02f)) {
        col.SetOffset(Vec2(offset[0], offset[1]));
        propertiesChanged = true;
      }

      ImGui::PopID();
      ImGui::Separator();
    }

    if (colliderToDelete >= 0) {
      entity.rigidBody->RemoveCollider(static_cast<size_t>(colliderToDelete));
      ELYSIUM_INFO("Removed Collider #{} from entity: {}", colliderToDelete + 1,
                   entity.name);
    } else if (propertiesChanged) {
      entity.rigidBody->RecalculateMassProperties();
    }
  }
}

void EntityInspectorPanel::DrawSpriteRendererComponent(EditorContext &context,
                                                       GameObject &entity) {
  (void)context;
  auto *sprite = entity.GetComponent<SpriteRenderer>();
  if (!sprite)
    return;

  if (ImGui::CollapsingHeader("Sprite Renderer",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    // Transform Offsets
    if (ImGui::TreeNodeEx("Transform Offsets", ImGuiTreeNodeFlags_DefaultOpen)) {
      float posOffset[2] = {sprite->LocalPositionOffset.x, sprite->LocalPositionOffset.y};
      if (ImGui::DragFloat2("Position Offset", posOffset, 0.02f)) {
        sprite->LocalPositionOffset = Vec2(posOffset[0], posOffset[1]);
      }

      ImGui::DragFloat("Rotation Offset", &sprite->LocalRotationOffset, 1.0f, -360.0f, 360.0f, "%.1f deg");

      float scaleOffset[2] = {sprite->LocalScaleOffset.x, sprite->LocalScaleOffset.y};
      if (ImGui::DragFloat2("Scale Offset", scaleOffset, 0.02f, 0.01f, 100.0f)) {
        sprite->LocalScaleOffset = Vec2(scaleOffset[0], scaleOffset[1]);
      }

      float pivotAnchor[2] = {sprite->Pivot.x, sprite->Pivot.y};
      if (ImGui::DragFloat2("Pivot Anchor", pivotAnchor, 0.01f, 0.0f, 1.0f, "%.2f")) {
        sprite->Pivot = Vec2(pivotAnchor[0], pivotAnchor[1]);
      }

      ImGui::TreePop();
      ImGui::Separator();
    }

    // Shape toggle
    const char *shapes[] = {"Box / Rectangle", "Circle / Sphere"};
    int currentShape = sprite->isCircle ? 1 : 0;
    if (ImGui::Combo("Shape", &currentShape, shapes, 2)) {
      sprite->isCircle = (currentShape == 1);
    }

    if (sprite->isCircle) {
      ImGui::DragFloat("Radius (m)", &sprite->radius, 0.02f, 0.01f, 50.0f);
    } else {
      float sz[2] = {sprite->size.x, sprite->size.y};
      if (ImGui::DragFloat2("Size (m)", sz, 0.02f, 0.01f, 50.0f)) {
        sprite->size.x = sz[0];
        sprite->size.y = sz[1];
      }
    }

    // Color picker
    float color[4] = {
        sprite->color.r / 255.0f,
        sprite->color.g / 255.0f,
        sprite->color.b / 255.0f,
        sprite->color.a / 255.0f,
    };
    if (ImGui::ColorEdit4("Tint Color", color)) {
      sprite->color = sf::Color(static_cast<uint8_t>(color[0] * 255.0f),
                                static_cast<uint8_t>(color[1] * 255.0f),
                                static_cast<uint8_t>(color[2] * 255.0f),
                                static_cast<uint8_t>(color[3] * 255.0f));
    }

    // Helper lambda to assign and synchronize texture on sprite renderer
    auto assignTexture = [&](AssetHandle handle, const std::filesystem::path &debugPath = "") {
      sprite->SetTextureHandle(handle);
      if (static_cast<uint64_t>(handle) != 0) {
        auto loadedTex = AssetManager::Get().Load<sf::Texture>(handle);
        if (loadedTex) {
          sprite->texture = std::make_shared<const Texture>(*loadedTex);
          sprite->isTextured = true;
        }
      } else {
        sprite->SetTextureHandle(0);
      }
      if (!debugPath.empty()) {
        ELYSIUM_INFO("Assigned texture '{0}' to entity", debugPath.string());
      }
    };

    // Ensure texture is loaded if a handle exists but texture is not yet cached on the sprite
    if (sprite->textureHandle != 0 && !sprite->texture) {
      auto loadedTex = AssetManager::Get().Load<sf::Texture>(sprite->textureHandle);
      if (loadedTex) {
        sprite->texture = std::make_shared<const Texture>(*loadedTex);
        sprite->isTextured = true;
      }
    }

    // Texture slot with Thumbnail Preview and Native File Browser Picking
    ImGui::Spacing();
    ImGui::Text("Texture Slot:");

    std::shared_ptr<sf::Texture> sfmlTexture = nullptr;
    std::string textureLabel = "[ None (Texture2D) ]";
    if (sprite->textureHandle != 0) {
      const auto &meta = AssetManager::Get().GetMetadata(sprite->textureHandle);
      if (static_cast<uint64_t>(meta.handle) != 0) {
        textureLabel = meta.filePath.filename().string() + " (" +
                       std::to_string(static_cast<uint64_t>(sprite->textureHandle)) + ")";
      } else {
        textureLabel = "Handle: " + std::to_string(static_cast<uint64_t>(sprite->textureHandle));
      }
      sfmlTexture = AssetManager::Get().Load<sf::Texture>(sprite->textureHandle);
    }

    bool openFilePicker = false;
    float previewWidth = ImGui::GetContentRegionAvail().x - 60.0f;
    if (previewWidth < 60.0f) previewWidth = 60.0f;

    if (sfmlTexture) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.16f, 0.20f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.32f, 0.45f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.30f, 0.40f, 0.58f, 1.0f));
      if (ImGui::ImageButton("##TextureThumb", *sfmlTexture, sf::Vector2f(32.0f, 32.0f))) {
        openFilePicker = true;
      }
      ImGui::PopStyleColor(3);

      ImGui::SameLine();
      ImVec2 slotSize(previewWidth - 40.0f, 32.0f);
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.16f, 0.20f, 1.0f));
      if (ImGui::Button(textureLabel.c_str(), slotSize)) {
        openFilePicker = true;
      }
      ImGui::PopStyleColor();
    } else {
      ImVec2 slotSize(previewWidth, 32.0f);
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.16f, 0.20f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.32f, 0.45f, 1.0f));
      if (ImGui::Button(textureLabel.c_str(), slotSize)) {
        openFilePicker = true;
      }
      ImGui::PopStyleColor(2);
    }

    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Click to browse texture from disk or drag-and-drop from Content Browser");
    }

    // Drag-and-Drop Target for Texture
    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
        // Can be wide-string path or raw 64-bit uint64_t handle
        if (payload->DataSize > static_cast<int>(sizeof(uint64_t))) {
          const wchar_t *path = reinterpret_cast<const wchar_t *>(payload->Data);
          std::filesystem::path texturePath(path);
          if (AssetManager::IsTextureFile(texturePath)) {
            AssetHandle handle = AssetManager::GetOrCreateAssetHandle(texturePath);
            assignTexture(handle, texturePath);
          }
        } else if (payload->DataSize == sizeof(uint64_t)) {
          uint64_t droppedHandle = *reinterpret_cast<const uint64_t *>(payload->Data);
          const auto &meta = AssetManager::Get().GetMetadata(droppedHandle);
          assignTexture(droppedHandle, meta.filePath);
        }
      } else if (const ImGuiPayload *texPayload =
                     ImGui::AcceptDragDropPayload(AssetPayloads::TEXTURE)) {
        uint64_t droppedHandle = *reinterpret_cast<const uint64_t *>(texPayload->Data);
        const auto &meta = AssetManager::Get().GetMetadata(droppedHandle);
        assignTexture(droppedHandle, meta.filePath);
      }
      ImGui::EndDragDropTarget();
    }

    // Native File Browser Picking Trigger
    if (openFilePicker) {
      std::filesystem::path assetDir = Project::GetActiveAssetDirectory();
      std::vector<FileDialogFilter> filters = {
          {"Image Files (*.png *.jpg *.tga)", "*.png;*.jpg;*.jpeg;*.tga;*.bmp"},
          {"All Files (*.*)", "*.*"}};
      auto selectedFile = FileDialog::OpenFile("Select Texture", assetDir, filters);
      if (selectedFile.has_value() && !selectedFile->empty()) {
        std::filesystem::path absPath = *selectedFile;
        std::error_code ec;
        std::filesystem::path relPath = std::filesystem::relative(absPath, assetDir, ec);
        if (ec || relPath.empty()) {
          relPath = absPath;
        }

        if (AssetManager::IsTextureFile(absPath)) {
          AssetHandle handle = AssetManager::GetOrCreateAssetHandle(relPath);
          assignTexture(handle, relPath);
        } else {
          ELYSIUM_WARN("Selected file '{}' is not a recognized texture format", absPath.string());
        }
      }
    }

    ImGui::SameLine();
    if (ImGui::Button("X##ClearTex", ImVec2(24.0f, 32.0f)) || ImGui::Button("Clear##Tex")) {
      assignTexture(0);
    }

    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Clear assigned texture");
    }
  }
}

void EntityInspectorPanel::DrawAudioSourceComponent(EditorContext &context,
                                                    GameObject &entity) {
  (void)context;
  auto *audio = entity.GetComponent<AudioSourceComponent>();
  if (!audio)
    return;

  if (ImGui::CollapsingHeader("Audio Source", ImGuiTreeNodeFlags_DefaultOpen)) {
    // Filepath buffer input with drag-and-drop target
    char pathBuffer[512];
    std::strncpy(pathBuffer, audio->filepath.c_str(), sizeof(pathBuffer) - 1);
    pathBuffer[sizeof(pathBuffer) - 1] = '\0';
    if (ImGui::InputText("Sound File", pathBuffer, sizeof(pathBuffer))) {
      audio->filepath = pathBuffer;
    }

    // Drag-and-drop target for audio files
    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload *payload =
              ImGui::AcceptDragDropPayload(AssetPayloads::AUDIO)) {
        uint64_t droppedHandle = *reinterpret_cast<const uint64_t *>(payload->Data);
        const auto &meta = AssetManager::Get().GetMetadata(droppedHandle);
        if (static_cast<uint64_t>(meta.handle) != 0) {
          audio->filepath = meta.filePath.generic_string();
          ELYSIUM_INFO("EntityInspector: Dropped audio file '{}' onto entity '{}'",
                       audio->filepath, entity.name);
        }
      }
      ImGui::EndDragDropTarget();
    }

    // Playback preview controls
    ImGui::Spacing();
    if (ImGui::Button("Play")) {
      audio->Play();
    }
    ImGui::SameLine();
    if (ImGui::Button("Pause")) {
      audio->Pause();
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
      audio->Stop();
    }

    ImGui::SameLine();
    if (audio->IsPlaying()) {
      ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[Playing]");
    } else if (audio->IsPaused()) {
      ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "[Paused]");
    } else {
      ImGui::TextDisabled("[Stopped]");
    }

    ImGui::Spacing();
    ImGui::SliderFloat("Volume", &audio->volume, 0.0f, 100.0f, "%.1f");
    ImGui::SliderFloat("Pitch", &audio->pitch, 0.1f, 3.0f, "%.2f");

    ImGui::Checkbox("Play On Awake", &audio->playOnAwake);
    ImGui::SameLine();
    ImGui::Checkbox("Loop", &audio->loop);

    ImGui::Separator();
    ImGui::Checkbox("Spatial Audio (3D)", &audio->isSpatial);

    if (audio->isSpatial) {
      ImGui::DragFloat("Min Distance", &audio->minDistance, 0.1f, 0.1f, 100.0f);
      ImGui::DragFloat("Attenuation", &audio->attenuation, 0.05f, 0.0f, 10.0f);
      ImGui::TextDisabled("Source Pos: (%.2f, %.2f, %.2f)",
                          static_cast<double>(entity.position.x),
                          static_cast<double>(entity.position.y),
                          static_cast<double>(entity.position.z));
    }
  }
}

void EntityInspectorPanel::DrawCameraComponent(EditorContext &context,
                                               Scene &scene,
                                               GameObject &entity) {
  (void)context;
  (void)scene;
  auto *cam = entity.GetComponent<CameraComponent>();
  if (!cam)
    return;

  bool open = ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen);
  if (ImGui::BeginPopupContextItem("CameraComponentContext")) {
    if (ImGui::MenuItem("Remove Component")) {
      auto it = std::find_if(entity.components.begin(), entity.components.end(),
                             [](const std::unique_ptr<Component> &c) {
                               return dynamic_cast<CameraComponent *>(c.get()) != nullptr;
                             });
      if (it != entity.components.end()) {
        entity.components.erase(it);
        ELYSIUM_INFO("Removed CameraComponent from entity: {}", entity.name);
      }
      ImGui::EndPopup();
      return;
    }
    ImGui::EndPopup();
  }

  if (open) {
    // Orthographic Size
    float size = cam->Camera.GetOrthographicSize();
    if (ImGui::DragFloat("Size", &size, 0.1f, 0.1f, 1000.0f, "%.2f")) {
      cam->Camera.SetOrthographicSize(size);
    }

    // Near and Far clipping planes
    float nearClip = cam->Camera.GetNearClip();
    if (ImGui::DragFloat("Near Clip", &nearClip, 0.1f, -1000.0f, 1000.0f, "%.2f")) {
      cam->Camera.SetNearClip(nearClip);
    }

    float farClip = cam->Camera.GetFarClip();
    if (ImGui::DragFloat("Far Clip", &farClip, 0.1f, -1000.0f, 1000.0f, "%.2f")) {
      cam->Camera.SetFarClip(farClip);
    }

    // Primary Camera toggle
    ImGui::Checkbox("Primary", &cam->Primary);

    // Fixed Aspect Ratio toggle
    ImGui::Checkbox("Fixed Aspect Ratio", &cam->FixedAspectRatio);

    if (cam->FixedAspectRatio) {
      float aspect = cam->Camera.GetAspectRatio();
      if (ImGui::DragFloat("Aspect Ratio", &aspect, 0.01f, 0.1f, 10.0f, "%.3f")) {
        cam->Camera.SetAspectRatio(aspect);
      }
    }

    // Clear Color
    float clearColor[4] = {
      static_cast<float>(cam->ClearColor.r) / 255.0f,
      static_cast<float>(cam->ClearColor.g) / 255.0f,
      static_cast<float>(cam->ClearColor.b) / 255.0f,
      static_cast<float>(cam->ClearColor.a) / 255.0f
    };
    if (ImGui::ColorEdit4("Clear Color", clearColor)) {
      cam->ClearColor = sf::Color(
        static_cast<uint8_t>(std::clamp(clearColor[0] * 255.0f, 0.0f, 255.0f)),
        static_cast<uint8_t>(std::clamp(clearColor[1] * 255.0f, 0.0f, 255.0f)),
        static_cast<uint8_t>(std::clamp(clearColor[2] * 255.0f, 0.0f, 255.0f)),
        static_cast<uint8_t>(std::clamp(clearColor[3] * 255.0f, 0.0f, 255.0f))
      );
    }
  }
}

void EntityInspectorPanel::DrawCustomComponents(EditorContext &context,
                                                GameObject &entity) {
  (void)context;
  if (entity.components.empty())
    return;

  if (ImGui::CollapsingHeader("Attached Components",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    for (size_t i = 0; i < entity.components.size(); ++i) {
      ImGui::PushID(static_cast<int>(i));
      ImGui::BulletText("Custom Component #%zu", i + 1);
      ImGui::PopID();
    }
  }
}

} // namespace Elysium
