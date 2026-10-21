#pragma once

#include <cstdint>
#include <functional>

namespace Elysium {

class UUID {
public:
  UUID();
  UUID(uint64_t uuid);
  UUID(const UUID &) = default;
  UUID &operator=(const UUID &) = default;

  operator uint64_t() const noexcept { return m_UUID; }

  bool operator==(const UUID &other) const noexcept {
    return m_UUID == other.m_UUID;
  }
  bool operator!=(const UUID &other) const noexcept {
    return m_UUID != other.m_UUID;
  }
  bool operator<(const UUID &other) const noexcept {
    return m_UUID < other.m_UUID;
  }
  bool operator<=(const UUID &other) const noexcept {
    return m_UUID <= other.m_UUID;
  }
  bool operator>(const UUID &other) const noexcept {
    return m_UUID > other.m_UUID;
  }
  bool operator>=(const UUID &other) const noexcept {
    return m_UUID >= other.m_UUID;
  }

private:
  uint64_t m_UUID;
};

} // namespace Elysium

namespace std {
template <> struct hash<Elysium::UUID> {
  std::size_t operator()(const Elysium::UUID &uuid) const noexcept {
    return std::hash<uint64_t>()(static_cast<uint64_t>(uuid));
  }
};
} // namespace std
