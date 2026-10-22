#include "UUID.hpp"
#include <random>

namespace Elysium {

static uint64_t Generate64BitRandom() {
  thread_local std::random_device rd;
  thread_local std::mt19937_64 engine(
      (static_cast<uint64_t>(rd()) << 32) ^ static_cast<uint64_t>(rd()));
  thread_local std::uniform_int_distribution<uint64_t> distribution(
      1, std::numeric_limits<uint64_t>::max());
  return distribution(engine);
}

UUID::UUID() : m_UUID(Generate64BitRandom()) {}

UUID::UUID(uint64_t uuid) : m_UUID(uuid) {}

} // namespace Elysium
