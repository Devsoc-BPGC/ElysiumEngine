#pragma once

#include "Core.hpp"
#include <SFML/Audio.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace Elysium {

/**
 * @brief High-level Audio Subsystem for Elysium Engine.
 *
 * Manages streaming background music (sf::Music) and cached sound effects
 * (sf::SoundBuffer / sf::Sound pool) built on SFML 3.
 */
class ELYSIUM_API AudioEngine {
public:
  static AudioEngine &Get();

  AudioEngine() = default;
  ~AudioEngine();

  AudioEngine(const AudioEngine &) = delete;
  AudioEngine &operator=(const AudioEngine &) = delete;

  void Init();
  void Shutdown();

  void PlaySound(const std::string &filepath, float volume = 100.0f,
                 float pitch = 1.0f);

  void PlayMusic(const std::string &filepath, bool loop = true);
  void StopMusic();
  void PauseMusic();
  void ResumeMusic();

  void SetMasterVolume(float volume);
  [[nodiscard]] float GetMasterVolume() const;

  // Sound buffer caching helper
  std::shared_ptr<sf::SoundBuffer>
  GetOrCreateBuffer(const std::string &filepath);

private:
  sf::Sound &AcquireSound(const sf::SoundBuffer &buffer);
  void CleanupFinishedSounds();

private:
  static AudioEngine *s_Instance;
  bool m_Initialized = false;

  std::unique_ptr<sf::Music> m_Music;
  std::string m_CurrentMusicPath;

  // Sound buffer cache
  std::unordered_map<std::string, std::shared_ptr<sf::SoundBuffer>>
      m_SoundBuffers;

  // Sound playback pool (managed instances playing active buffers)
  std::vector<std::unique_ptr<sf::Sound>> m_SoundPool;
  static constexpr size_t k_MaxSoundPoolSize = 64;

  std::mutex m_AudioMutex;
};

} // namespace Elysium
