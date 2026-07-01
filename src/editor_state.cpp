#include "knotwork/editor_state.hpp"
#include "knotwork/graph.hpp"
#include "knotwork/report.hpp"

#include <sstream>

namespace knotwork {

EditorState::EditorState(Project project) : project_(std::move(project)) {}

const Project& EditorState::project() const {
  return project_;
}

Project& EditorState::project() {
  return project_;
}

const Selection& EditorState::selection() const {
  return selection_;
}

const std::vector<Bookmark>& EditorState::bookmarks() const {
  return bookmarks_;
}

const std::vector<EditorMessage>& EditorState::messages() const {
  return messages_;
}

void EditorState::set_project(Project project) {
  project_ = std::move(project);
  selection_ = {};
  history_.clear();
  messages_.clear();
}

void EditorState::set_scene(Scene scene) {
  project_.scene = std::move(scene);
  selection_ = {};
  history_.clear();
}

void EditorState::set_assets(AssetLibrary assets) {
  project_.assets = std::move(assets);
}

void EditorState::select_node(std::uint32_t node) {
  if (node < project_.scene.nodes.size()) {
    select(selection_, node);
  }
}

void EditorState::deselect_node(std::uint32_t node) {
  deselect(selection_, node);
}

void EditorState::toggle_node(std::uint32_t node) {
  if (node < project_.scene.nodes.size()) {
    toggle(selection_, node);
  }
}

void EditorState::clear_selection() {
  clear(selection_);
}

void EditorState::select_all() {
  clear(selection_);
  for (std::uint32_t i = 0; i < project_.scene.nodes.size(); ++i) {
    select(selection_, i);
  }
}

void EditorState::select_roots() {
  clear(selection_);
  for (std::uint32_t root : root_nodes(project_.scene)) {
    select(selection_, root);
  }
}

void EditorState::select_by_kind(NodeKind kind) {
  NodeQuery query;
  query.kind = kind;
  selection_ = knotwork::select_by_query(project_.scene, query);
}

bool EditorState::execute(Command command) {
  const bool ok = history_.execute(project_.scene, std::move(command));
  if (!ok) {
    push_message(Severity::Warning, "command failed");
  }
  return ok;
}

bool EditorState::undo() {
  const bool ok = history_.undo(project_.scene);
  if (!ok) {
    push_message(Severity::Info, "nothing to undo");
  }
  return ok;
}

bool EditorState::redo() {
  const bool ok = history_.redo(project_.scene);
  if (!ok) {
    push_message(Severity::Info, "nothing to redo");
  }
  return ok;
}

bool EditorState::transform_node(std::uint32_t node, Transform transform) {
  if (node >= project_.scene.nodes.size()) {
    push_message(Severity::Warning, "transform target is out of range");
    return false;
  }
  return execute(make_transform_command(project_.scene, node, transform, "transform-node"));
}

bool EditorState::translate_selection(Vec3 delta) {
  ScenePatch redo = patch_translate_selection(project_.scene, selection_, delta);
  return apply(std::move(redo), "translate-selection");
}

bool EditorState::apply(ScenePatch patch, std::string name) {
  Command command = patch_command(project_.scene, std::move(patch), std::move(name));
  return execute(std::move(command));
}

void EditorState::add_bookmark(std::string name, CameraView camera) {
  for (Bookmark& item : bookmarks_) {
    if (item.name == name) {
      item.camera = camera;
      return;
    }
  }
  bookmarks_.push_back({std::move(name), camera});
}

std::optional<CameraView> EditorState::bookmark(std::string_view name) const {
  for (const Bookmark& item : bookmarks_) {
    if (item.name == name) {
      return item.camera;
    }
  }
  return std::nullopt;
}

bool EditorState::remove_bookmark(std::string_view name) {
  const auto old_size = bookmarks_.size();
  bookmarks_.erase(std::remove_if(bookmarks_.begin(), bookmarks_.end(), [name](const Bookmark& item) {
                     return item.name == name;
                   }),
                   bookmarks_.end());
  return bookmarks_.size() != old_size;
}

void EditorState::push_message(Severity severity, std::string text) {
  messages_.push_back({severity, std::move(text)});
}

void EditorState::clear_messages() {
  messages_.clear();
}

SceneInspection EditorState::inspect() const {
  return inspect_project(project_);
}

std::string EditorState::status_text() const {
  std::ostringstream out;
  out << "project=" << project_.name << "\n";
  out << "nodes=" << project_.scene.nodes.size() << "\n";
  out << "assets=" << project_.assets.stats().mesh_count << "\n";
  out << "selection=" << selection_.nodes.size() << "\n";
  out << "bookmarks=" << bookmarks_.size() << "\n";
  out << "messages=" << messages_.size() << "\n";
  return out.str();
}

Command patch_command(const Scene& scene, ScenePatch redo, std::string name) {
  Command command;
  command.name = std::move(name);
  command.undo = inverse_patch_for_transform(scene, redo);
  command.redo = std::move(redo);
  return command;
}

ScenePatch inverse_patch_for_transform(const Scene& scene, const ScenePatch& patch) {
  ScenePatch inverse;
  inverse.name = "inverse-" + patch.name;
  for (const PatchOp& op : patch.ops) {
    if (op.kind == PatchOpKind::SetTransform && op.index < scene.nodes.size()) {
      PatchOp undo;
      undo.kind = PatchOpKind::SetTransform;
      undo.index = op.index;
      undo.transform = scene.nodes[op.index].local;
      inverse.ops.push_back(undo);
    } else if (op.kind == PatchOpKind::ReparentNode && op.index < scene.nodes.size()) {
      PatchOp undo;
      undo.kind = PatchOpKind::ReparentNode;
      undo.index = op.index;
      undo.other = scene.nodes[op.index].parent;
      inverse.ops.push_back(undo);
    } else if (op.kind == PatchOpKind::SetNodeMaterial && op.index < scene.nodes.size()) {
      PatchOp undo;
      undo.kind = PatchOpKind::SetNodeMaterial;
      undo.index = op.index;
      undo.other = scene.nodes[op.index].material;
      inverse.ops.push_back(undo);
    }
  }
  return inverse;
}

}  // namespace knotwork
