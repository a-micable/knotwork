#pragma once

#include "knotwork/camera.hpp"
#include "knotwork/project.hpp"
#include "knotwork/selection.hpp"

#include <map>

namespace knotwork {

struct Bookmark {
  std::string name;
  CameraView camera;
};

struct EditorMessage {
  Severity severity = Severity::Info;
  std::string text;
};

class EditorState {
 public:
  explicit EditorState(Project project = {});

  [[nodiscard]] const Project& project() const;
  [[nodiscard]] Project& project();
  [[nodiscard]] const Selection& selection() const;
  [[nodiscard]] const std::vector<Bookmark>& bookmarks() const;
  [[nodiscard]] const std::vector<EditorMessage>& messages() const;

  void set_project(Project project);
  void set_scene(Scene scene);
  void set_assets(AssetLibrary assets);
  void select_node(std::uint32_t node);
  void deselect_node(std::uint32_t node);
  void toggle_node(std::uint32_t node);
  void clear_selection();
  void select_all();
  void select_roots();
  void select_by_kind(NodeKind kind);

  [[nodiscard]] bool execute(Command command);
  [[nodiscard]] bool undo();
  [[nodiscard]] bool redo();
  [[nodiscard]] bool transform_node(std::uint32_t node, Transform transform);
  [[nodiscard]] bool translate_selection(Vec3 delta);
  [[nodiscard]] bool apply(ScenePatch patch, std::string name = "patch");

  void add_bookmark(std::string name, CameraView camera);
  [[nodiscard]] std::optional<CameraView> bookmark(std::string_view name) const;
  [[nodiscard]] bool remove_bookmark(std::string_view name);

  void push_message(Severity severity, std::string text);
  void clear_messages();
  [[nodiscard]] SceneInspection inspect() const;
  [[nodiscard]] std::string status_text() const;

 private:
  Project project_;
  Selection selection_;
  CommandHistory history_;
  std::vector<Bookmark> bookmarks_;
  std::vector<EditorMessage> messages_;
};

[[nodiscard]] Command patch_command(const Scene& scene, ScenePatch redo, std::string name);
[[nodiscard]] ScenePatch inverse_patch_for_transform(const Scene& scene, const ScenePatch& patch);

}  // namespace knotwork
