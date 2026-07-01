#include "knotwork/selection.hpp"
#include "knotwork/graph.hpp"
#include "knotwork/mesh.hpp"
#include "knotwork/query.hpp"

#include <algorithm>

namespace knotwork {

bool CommandHistory::execute(Scene& scene, Command command) {
  PatchResult result = apply_patch(scene, command.redo);
  if (patch_has_errors(result)) {
    return false;
  }
  scene = std::move(result.scene);
  undo_.push_back(std::move(command));
  redo_.clear();
  return true;
}

bool CommandHistory::undo(Scene& scene) {
  if (undo_.empty()) {
    return false;
  }
  Command command = std::move(undo_.back());
  undo_.pop_back();
  PatchResult result = apply_patch(scene, command.undo);
  if (patch_has_errors(result)) {
    undo_.push_back(std::move(command));
    return false;
  }
  scene = std::move(result.scene);
  redo_.push_back(std::move(command));
  return true;
}

bool CommandHistory::redo(Scene& scene) {
  if (redo_.empty()) {
    return false;
  }
  Command command = std::move(redo_.back());
  redo_.pop_back();
  PatchResult result = apply_patch(scene, command.redo);
  if (patch_has_errors(result)) {
    redo_.push_back(std::move(command));
    return false;
  }
  scene = std::move(result.scene);
  undo_.push_back(std::move(command));
  return true;
}

bool CommandHistory::can_undo() const {
  return !undo_.empty();
}

bool CommandHistory::can_redo() const {
  return !redo_.empty();
}

std::vector<std::string> CommandHistory::undo_stack_names() const {
  std::vector<std::string> names;
  for (const Command& command : undo_) {
    names.push_back(command.name);
  }
  return names;
}

std::vector<std::string> CommandHistory::redo_stack_names() const {
  std::vector<std::string> names;
  for (const Command& command : redo_) {
    names.push_back(command.name);
  }
  return names;
}

void CommandHistory::clear() {
  undo_.clear();
  redo_.clear();
}

void select(Selection& selection, std::uint32_t node) {
  selection.nodes.insert(node);
}

void deselect(Selection& selection, std::uint32_t node) {
  selection.nodes.erase(node);
}

void toggle(Selection& selection, std::uint32_t node) {
  if (selected(selection, node)) {
    deselect(selection, node);
  } else {
    select(selection, node);
  }
}

void clear(Selection& selection) {
  selection.nodes.clear();
}

bool selected(const Selection& selection, std::uint32_t node) {
  return selection.nodes.contains(node);
}

Selection select_descendants(const Scene& scene, std::uint32_t root) {
  Selection selection;
  for (std::uint32_t node : descendants(scene, root)) {
    selection.nodes.insert(node);
  }
  return selection;
}

Selection select_by_query(const Scene& scene, const NodeQuery& query) {
  Selection selection;
  for (std::uint32_t node : query_nodes(scene, query)) {
    selection.nodes.insert(node);
  }
  return selection;
}

ScenePatch patch_translate_selection(const Scene& scene, const Selection& selection, Vec3 delta) {
  ScenePatch patch;
  patch.name = "translate-selection";
  for (std::uint32_t node : selection.nodes) {
    if (node >= scene.nodes.size()) {
      continue;
    }
    Transform transform = scene.nodes[node].local;
    transform.translation = transform.translation + delta;
    PatchOp op;
    op.kind = PatchOpKind::SetTransform;
    op.index = node;
    op.transform = transform;
    patch.ops.push_back(op);
  }
  return patch;
}

Command make_transform_command(const Scene& scene, std::uint32_t node, Transform transform, std::string name) {
  Command command;
  command.name = std::move(name);
  if (node < scene.nodes.size()) {
    command.redo = transform_patch(node, transform);
    command.undo = transform_patch(node, scene.nodes[node].local);
  }
  return command;
}

}  // namespace knotwork
