#include "AudioEngine.hpp"
#include "Log.h"
#include <algorithm>
#include <filesystem>

namespace Elysium {

AudioEngine *AudioEngine::s_Instance = nullptr;

AudioEngine &AudioEngine::Get() {
  if (!s_Instance) {
    static AudioEngine defaultInstance;
    return defaultInstance;
  }
  return *s_Instance;
}

AudioEngine::~AudioEngine() {
  if (m_Initialized) {
    Shutdown();
  }
}

void AudioEngine::Init() {
  std::lock_guard<std::mutex> lock(m_AudioMutex);
  if (m_Initialized) {
    ELYSIUM_CORE_WARN("AudioEngine::Init called when already initialized.");
    return;
  }

  s_Instance = this;
  m_Initialized = true;
  m_Music = std::make_unique<sf::Music>();

  // Ensure listener default values in SFML 3
  sf::Listener::setGlobalVolume(100.0f);
  sf::Listener::setPosition({0.0f, 0.0f, 0.0f});

  ELYSIUM_CORE_INFO("AudioEngine initialized successfully (SFML 3 Audio).");
}

void AudioEngine::Shutdown() {
  std::lock_guard<std::mutex> lock(m_AudioMutex);
  if (!m_Initialized)
    return;

  ELYSIUM_CORE_INFO("AudioEngine shutting down...");

  if (m_Music) {
    m_Music->stop();
    m_Music.reset();
  }

  for (auto &sound : m_SoundPool) {
    if (sound) {
      sound->stop();
    }
  }
  m_SoundPool.clear();
  m_SoundBuffers.clear();

  m_Initialized = false;
  if (s_Instance == this) {
    s_Instance = nullptr;
  }
}

std::shared_ptr<sf::SoundBuffer>
AudioEngine::GetOrCreateBuffer(const std::string &filepath) {
  if (filepath.empty()) {
    return nullptr;
  }

  auto it = m_SoundBuffers.find(filepath);
  if (it != m_SoundBuffers.end()) {
    return it->second;
  }

  if (!std::filesystem::exists(filepath)) {
    ELYSIUM_CORE_WARN("AudioEngine: Sound file not found: '{}'", filepath);
    return nullptr;
  }

  auto buffer = std::make_shared<sf::SoundBuffer>();
  if (!buffer->loadFromFile(filepath)) {
    ELYSIUM_CORE_ERROR("AudioEngine: Failed to load sound buffer from '{}'",
                       filepath);
    return nullptr;
  }

  m_SoundBuffers[filepath] = buffer;
  ELYSIUM_CORE_INFO("AudioEngine: Cached sound buffer for '{}'", filepath);
  return buffer;
}

void AudioEngine::CleanupFinishedSounds() {
  // Reclaim sound instances that finished playing
  auto it = std::remove_if(m_SoundPool.begin(), m_SoundPool.end(),
                           [](const std::unique_ptr<sf::Sound> &s) {
                             return !s || s->getStatus() ==
                                              sf::SoundSource::Status::Stopped;
                           });
  m_SoundPool.erase(it, m_SoundPool.end());
}

sf::Sound &AudioEngine::AcquireSound(const sf::SoundBuffer &buffer) {
  CleanupFinishedSounds();

  // If pool reached maximum size, recycle or reuse the first available
  if (m_SoundPool.size() >= k_MaxSoundPoolSize) {
    auto &reused = m_SoundPool.front();
    reused->stop();
    reused->setBuffer(buffer);
    return *reused;
  }

  auto sound = std::make_unique<sf::Sound>(buffer);
  sf::Sound &ref = *sound;
  m_SoundPool.push_back(std::move(sound));
  return ref;
}

void AudioEngine::PlaySound(const std::string &filepath, float volume,
                            float pitch) {
  std::lock_guard<std::mutex> lock(m_AudioMutex);
  if (!m_Initialized) {
    ELYSIUM_CORE_WARN("AudioEngine::PlaySound called before Init()");
    return;
  }

  auto buffer = GetOrCreateBuffer(filepath);
  if (!buffer)
    return;

  sf::Sound &sound = AcquireSound(*buffer);
  sound.setVolume(std::clamp(volume, 0.0f, 100.0f));
  sound.setPitch(std::max(0.1f, pitch));
  sound.setSpatializationEnabled(false);
  sound.setRelativeToListener(true);
  sound.setLooping(false);
  sound.play();
}

void AudioEngine::PlayMusic(const std::string &filepath, bool loop) {
  std::lock_guard<std::mutex> lock(m_AudioMutex);
  if (!m_Initialized) {
    ELYSIUM_CORE_WARN("AudioEngine::PlayMusic called before Init()");
    return;
  }

  if (filepath.empty())
    return;

  if (!m_Music) {
    m_Music = std::make_unique<sf::Music>();
  }

  if (!std::filesystem::exists(filepath)) {
    ELYSIUM_CORE_WARN("AudioEngine: Music file does not exist: '{}'", filepath);
    return;
  }

  m_Music->stop();
  if (!m_Music->openFromFile(filepath)) {
    ELYSIUM_CORE_ERROR("AudioEngine: Failed to open music stream from '{}'",
                       filepath);
    return;
  }

  m_CurrentMusicPath = filepath;
  m_Music->setLooping(loop);
  m_Music->setSpatializationEnabled(false);
  m_Music->setRelativeToListener(true);
  m_Music->play();

  ELYSIUM_CORE_INFO("AudioEngine: Playing music '{}' (loop: {})", filepath,
                    loop);
}

void AudioEngine::StopMusic() {
  std::lock_guard<std::mutex> lock(m_AudioMutex);
  if (m_Music) {
    m_Music->stop();
  }
}

void AudioEngine::PauseMusic() {
  std::lock_guard<std::mutex> lock(m_AudioMutex);
  if (m_Music && m_Music->getStatus() == sf::SoundSource::Status::Playing) {
    m_Music->pause();
  }
}

void AudioEngine::ResumeMusic() {
  std::lock_guard<std::mutex> lock(m_AudioMutex);
  if (m_Music && m_Music->getStatus() == sf::SoundSource::Status::Paused) {
    m_Music->play();
  }
}

void AudioEngine::SetMasterVolume(float volume) {
  volume = std::clamp(volume, 0.0f, 100.0f);
  sf::Listener::setGlobalVolume(volume);
}

float AudioEngine::GetMasterVolume() const {
  return sf::Listener::getGlobalVolume();
}

} // namespace Elysium
