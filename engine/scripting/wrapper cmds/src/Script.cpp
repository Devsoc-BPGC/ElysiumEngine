#include "Script.hpp"
#include "Entity.hpp"

namespace Elysium {

void Script::OnCloned(const std::shared_ptr<GameObject> &owner) {
    m_EntityHolder = std::make_shared<Entity>(owner);
    entity = m_EntityHolder.get();
}

} // namespace Elysium
