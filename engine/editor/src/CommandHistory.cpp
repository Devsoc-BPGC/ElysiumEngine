#include "elypch.hpp"
#include "CommandHistory.hpp"

namespace Elysium {

CommandHistory::CommandHistory(size_t maxDepth)
    : m_MaxHistoryDepth(maxDepth > 0 ? maxDepth : 1) {}

void CommandHistory::ExecuteCommand(std::unique_ptr<ICommand> cmd) {
  if (!cmd || m_Locked)
    return;

  cmd->Execute();
  m_UndoStack.push_back(std::move(cmd));
  m_RedoStack.clear();

  if (m_UndoStack.size() > m_MaxHistoryDepth) {
    m_UndoStack.erase(m_UndoStack.begin());
  }
}

void CommandHistory::PushExecutedCommand(std::unique_ptr<ICommand> cmd) {
  if (!cmd || m_Locked)
    return;

  m_UndoStack.push_back(std::move(cmd));
  m_RedoStack.clear();

  if (m_UndoStack.size() > m_MaxHistoryDepth) {
    m_UndoStack.erase(m_UndoStack.begin());
  }
}

bool CommandHistory::Undo() {
  if (!CanUndo())
    return false;

  auto cmd = std::move(m_UndoStack.back());
  m_UndoStack.pop_back();

  cmd->Undo();
  m_RedoStack.push_back(std::move(cmd));
  return true;
}

bool CommandHistory::Redo() {
  if (!CanRedo())
    return false;

  auto cmd = std::move(m_RedoStack.back());
  m_RedoStack.pop_back();

  cmd->Execute();
  m_UndoStack.push_back(std::move(cmd));
  return true;
}

void CommandHistory::Clear() {
  m_UndoStack.clear();
  m_RedoStack.clear();
}

bool CommandHistory::CanUndo() const {
  return !m_Locked && !m_UndoStack.empty();
}

bool CommandHistory::CanRedo() const {
  return !m_Locked && !m_RedoStack.empty();
}

std::string CommandHistory::GetUndoCommandName() const {
  if (m_UndoStack.empty())
    return "";
  return m_UndoStack.back()->GetName();
}

std::string CommandHistory::GetRedoCommandName() const {
  if (m_RedoStack.empty())
    return "";
  return m_RedoStack.back()->GetName();
}

void CommandHistory::SetMaxHistoryDepth(size_t depth) {
  m_MaxHistoryDepth = depth > 0 ? depth : 1;
  while (m_UndoStack.size() > m_MaxHistoryDepth) {
    m_UndoStack.erase(m_UndoStack.begin());
  }
  while (m_RedoStack.size() > m_MaxHistoryDepth) {
    m_RedoStack.erase(m_RedoStack.begin());
  }
}

} // namespace Elysium
