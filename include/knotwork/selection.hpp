#pragma once

#include "knotwork/patch.hpp"
#include "knotwork/query.hpp"

#include <set>

namespace knotwork {

struct Selection {
  std::set<std::uint32_t> nodes;
};

struct Command {
  std::string name;
  ScenePatch redo;
  ScenePatch undo;
};

class CommandHistory {
 public:
  [[nodiscard]] bool execute(Scene& scene, Command command);
  [[nodiscard]] bool undo(Scene& scene);
  [[nodiscard]] bool redo(Scene& scene);
  [[nodiscard]] bool can_undo() const;
  [[nodiscard]] bool can_redo() const;
  [[nodiscard]] std::vector<std::string> undo_stack_names() const;
  [[nodiscard]] std::vector<std::string> redo_stack_names() const;
  void clear();

 private:
  std::vector<Command> undo_;
  std::vector<Command> redo_;
};

void select(Selection& selection, std::uint32_t node);
void deselect(Selection& selection, std::uint32_t node);
void toggle(Selection& selection, std::uint32_t node);
void clear(Selection& selection);
[[nodiscard]] bool selected(const Selection& selection, std::uint32_t node);
[[nodiscard]] Selection select_descendants(const Scene& scene, std::uint32_t root);
[[nodiscard]] Selection select_by_query(const Scene& scene, const NodeQuery& query);
[[nodiscard]] ScenePatch patch_translate_selection(const Scene& scene, const Selection& selection, Vec3 delta);
[[nodiscard]] Command make_transform_command(const Scene& scene,
                                             std::uint32_t node,
                                             Transform transform,
                                             std::string name = "transform");

}  // namespace knotwork
