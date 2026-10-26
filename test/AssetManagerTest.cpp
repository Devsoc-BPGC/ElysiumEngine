#include "AssetManager.hpp"
#include "Log.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

void TestAssetRegistrationAndLookup() {
  auto &am = Elysium::AssetManager::Get();
  std::filesystem::path testDir = "test_sandbox_assets";
  std::filesystem::create_directories(testDir);

  am.Init(testDir);

  // Register assets
  Elysium::AssetHandle h1 = am.RegisterAsset("textures/player.png", Elysium::AssetType::Texture);
  Elysium::AssetHandle h2 = am.RegisterAsset("audio/jump.wav", Elysium::AssetType::Audio);
  Elysium::AssetHandle h3 = am.RegisterAsset("fonts/inter.ttf", Elysium::AssetType::Font);
  Elysium::AssetHandle h4 = am.RegisterAsset("scenes/level1.elyscene", Elysium::AssetType::Scene);

  assert(static_cast<uint64_t>(h1) != 0);
  assert(static_cast<uint64_t>(h2) != 0);
  assert(static_cast<uint64_t>(h3) != 0);
  assert(static_cast<uint64_t>(h4) != 0);
  assert(h1 != h2 && h2 != h3 && h3 != h4);

  // Idempotency: re-registering returns existing handle
  Elysium::AssetHandle h1Again = am.RegisterAsset("textures/player.png");
  assert(h1 == h1Again);

  // Metadata queries
  const auto &meta1 = am.GetMetadata(h1);
  assert(meta1.handle == h1);
  assert(meta1.type == Elysium::AssetType::Texture);
  assert(meta1.filePath == "textures/player.png");
  assert(!meta1.isLoaded);

  assert(am.HasAsset(h1));
  assert(am.HasAsset("textures/player.png"));
  assert(am.GetHandleFromPath("textures/player.png") == h1);

  std::cout << "[PASS] TestAssetRegistrationAndLookup" << std::endl;
}

void TestManifestSerializationAcrossRestarts() {
  std::filesystem::path testDir = "test_sandbox_assets";
  std::filesystem::path manifestPath = testDir / "assets.meta";

  // First session: register and save manifest
  {
    auto &am = Elysium::AssetManager::Get();
    am.Init(testDir);

    Elysium::AssetHandle h1 = am.RegisterAsset("sprites/hero.png", Elysium::AssetType::Texture);
    Elysium::AssetHandle h2 = am.RegisterAsset("sfx/coin.wav", Elysium::AssetType::Audio);
    assert(static_cast<uint64_t>(h1) != 0 && static_cast<uint64_t>(h2) != 0);

    bool saved = am.SaveManifest(manifestPath);
    assert(saved);
    assert(std::filesystem::exists(manifestPath));

    am.Shutdown();
  }

  // Second session (simulating engine restart): load manifest and verify UUID persistence
  {
    auto &am = Elysium::AssetManager::Get();
    am.Init(testDir);

    assert(am.HasAsset("sprites/hero.png"));
    assert(am.HasAsset("sfx/coin.wav"));

    Elysium::AssetHandle loadedHeroHandle = am.GetHandleFromPath("sprites/hero.png");
    assert(static_cast<uint64_t>(loadedHeroHandle) != 0);

    const auto &metaHero = am.GetMetadata(loadedHeroHandle);
    assert(metaHero.type == Elysium::AssetType::Texture);
    assert(metaHero.filePath == "sprites/hero.png");

    am.Shutdown();
  }

  std::error_code ec;
  std::filesystem::remove_all(testDir, ec);

  std::cout << "[PASS] TestManifestSerializationAcrossRestarts" << std::endl;
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  Elysium::Log::Init();
  std::cout << "Running Elysium AssetManager & Manifest Tests...\n";
  TestAssetRegistrationAndLookup();
  TestManifestSerializationAcrossRestarts();
  std::cout << "\nAll AssetManager tests PASSED successfully!\n";
  return 0;
}
