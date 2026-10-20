#pragma once

#include "Component.hpp"
#include <SFML/Audio.hpp>
#include <memory>
#include <string>

namespace Elysium {

class AudioEngine;

/**
 * @brief Component that attaches an audio source to a GameObject.
 *
 * Supports both non-spatial and 3D spatialized playback linked to
 * the GameObject's Vec3 position in the Elysium physics/transform system.
 */
class AudioSourceComponent : public Component {
public:
  std::string filepath;
  float volume = 100.0f; // [0..100]
  float pitch = 1.0f;    // Fundamental pitch multiplier
  bool playOnAwake = false;
  bool loop = false;
  bool isSpatial = false;
  float minDistance = 1.0f; // Distance under which volume is at maximum
  float attenuation = 1.0f; // Attenuation factor

  AudioSourceComponent() = default;
  explicit AudioSourceComponent(std::string path, bool playAwake = false,
                                bool isLoop = false)
      : filepath(std::move(path)), playOnAwake(playAwake), loop(isLoop) {}

  ~AudioSourceComponent() override;

  void Init();
  void Start() override;
  void Update(float dt) override;

  void Play();
  void Stop();
  void Pause();

  [[nodiscard]] bool IsPlaying() const;
  [[nodiscard]] bool IsPaused() const;
  [[nodiscard]] bool IsStopped() const;

private:
  void EnsureSoundInstance();
  void SyncParameters();
  void UpdateSpatialPosition();

private:
  std::shared_ptr<sf::SoundBuffer> m_Buffer;
  std::unique_ptr<sf::Sound> m_Sound;
  std::string m_LoadedFilepath;
  bool m_Initialized = false;
};

} // namespace Elysium
