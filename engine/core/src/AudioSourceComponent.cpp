#include "AudioSourceComponent.hpp"
#include "AudioEngine.hpp"
#include "GameObject.hpp"
#include "Log.h"
#include <algorithm>

namespace Elysium {

AudioSourceComponent::~AudioSourceComponent() { Stop(); }

void AudioSourceComponent::Init() {
  EnsureSoundInstance();
  m_Initialized = true;
}

void AudioSourceComponent::Start() {
  if (!m_Initialized) {
    Init();
  }

  if (playOnAwake) {
    Play();
  }
}

void AudioSourceComponent::EnsureSoundInstance() {
  if (filepath.empty()) {
    if (m_Sound) {
      m_Sound->stop();
      m_Sound.reset();
    }
    m_Buffer.reset();
    m_LoadedFilepath.clear();
    return;
  }

  // Reload buffer if filepath changed or sound not instantiated
  if (filepath != m_LoadedFilepath || !m_Sound) {
    auto buffer = AudioEngine::Get().GetOrCreateBuffer(filepath);
    if (!buffer) {
      ELYSIUM_WARN("AudioSourceComponent: Unable to load buffer for '{}'",
                   filepath);
      return;
    }

    m_Buffer = buffer;
    m_LoadedFilepath = filepath;

    if (!m_Sound) {
      m_Sound = std::make_unique<sf::Sound>(*m_Buffer);
    } else {
      m_Sound->setBuffer(*m_Buffer);
    }
  }

  SyncParameters();
}

void AudioSourceComponent::SyncParameters() {
  if (!m_Sound)
    return;

  m_Sound->setVolume(std::clamp(volume, 0.0f, 100.0f));
  m_Sound->setPitch(std::max(0.1f, pitch));
  m_Sound->setLooping(loop);

  m_Sound->setSpatializationEnabled(isSpatial);
  m_Sound->setRelativeToListener(!isSpatial);
  m_Sound->setMinDistance(std::max(0.001f, minDistance));
  m_Sound->setAttenuation(std::max(0.0f, attenuation));

  UpdateSpatialPosition();
}

void AudioSourceComponent::UpdateSpatialPosition() {
  if (!m_Sound)
    return;

  if (isSpatial && gameObject) {
    // Map Elysium Vec3 coordinates to SFML 3 sf::Vector3f
    m_Sound->setPosition({gameObject->position.x, gameObject->position.y,
                          gameObject->position.z});
  }
}

void AudioSourceComponent::Update(float dt) {
  (void)dt;

  if (filepath != m_LoadedFilepath) {
    EnsureSoundInstance();
  } else {
    SyncParameters();
  }

  // Continuously update spatial position as entity moves
  if (isSpatial && m_Sound) {
    UpdateSpatialPosition();
  }
}

void AudioSourceComponent::Play() {
  EnsureSoundInstance();
  if (m_Sound) {
    SyncParameters();
    m_Sound->play();
  }
}

void AudioSourceComponent::Stop() {
  if (m_Sound) {
    m_Sound->stop();
  }
}

void AudioSourceComponent::Pause() {
  if (m_Sound && m_Sound->getStatus() == sf::SoundSource::Status::Playing) {
    m_Sound->pause();
  }
}

bool AudioSourceComponent::IsPlaying() const {
  return m_Sound && m_Sound->getStatus() == sf::SoundSource::Status::Playing;
}

bool AudioSourceComponent::IsPaused() const {
  return m_Sound && m_Sound->getStatus() == sf::SoundSource::Status::Paused;
}

bool AudioSourceComponent::IsStopped() const {
  return !m_Sound || m_Sound->getStatus() == sf::SoundSource::Status::Stopped;
}

} // namespace Elysium
