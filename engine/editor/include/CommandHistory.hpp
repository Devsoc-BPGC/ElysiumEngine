#ifndef COMMANDHISTORY_HPP
#define COMMANDHISTORY_HPP

#include "Command.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace Elysium {

/**
 * @brief Manages undo and redo stacks for editor interactions.
 */
class CommandHistory {
public:
  explicit CommandHistory(size_t maxDepth = 100);
  ~CommandHistory() = default;

  // Non-copyable, movable
  CommandHistory(const CommandHistory &) = delete;
  CommandHistory &operator=(const CommandHistory &) = delete;
  CommandHistory(CommandHistory &&) noexcept = default;
  CommandHistory &operator=(CommandHistory &&) noexcept = default;

  /**
   * @brief Executes a command for the first time and pushes it onto the undo stack.
   * Clears the redo stack.
   */
  void ExecuteCommand(std::unique_ptr<ICommand> cmd);

  /**
   * @brief Pushes a command that was already executed (e.g. during a live drag)
   * onto the undo stack. Clears the redo stack.
   */
  void PushExecutedCommand(std::unique_ptr<ICommand> cmd);

  /**
   * @brief Reverts the most recent command on the undo stack and moves it to redo stack.
   * @return true if an undo was performed.
   */
  bool Undo();

  /**
   * @brief Re-applies the most recent undone command and moves it back to undo stack.
   * @return true if a redo was performed.
   */
  bool Redo();

  /**
   * @brief Clears both undo and redo stacks.
   */
  void Clear();

  /**
   * @brief Returns true if an undo operation is available.
   */
  bool CanUndo() const;

  /**
   * @brief Returns true if a redo operation is available.
   */
  bool CanRedo() const;

  /**
   * @brief Returns the name of the top undo command, or empty string.
   */
  std::string GetUndoCommandName() const;

  /**
   * @brief Returns the name of the top redo command, or empty string.
   */
  std::string GetRedoCommandName() const;

  /**
   * @brief Locks or unlocks history (e.g. during Play mode).
   */
  void SetLocked(bool locked) { m_Locked = locked; }
  bool IsLocked() const { return m_Locked; }

  void SetMaxHistoryDepth(size_t depth);
  size_t GetMaxHistoryDepth() const { return m_MaxHistoryDepth; }

  size_t GetUndoCount() const { return m_UndoStack.size(); }
  size_t GetRedoCount() const { return m_RedoStack.size(); }

private:
  std::vector<std::unique_ptr<ICommand>> m_UndoStack;
  std::vector<std::unique_ptr<ICommand>> m_RedoStack;
  size_t m_MaxHistoryDepth = 100;
  bool m_Locked = false;
};

} // namespace Elysium

#endif // COMMANDHISTORY_HPP
