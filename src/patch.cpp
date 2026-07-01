#include "knotwork/patch.hpp"
#include "knotwork/validate.hpp"

#include <sstream>

namespace knotwork {
namespace {

SceneError patch_error(std::string message) {
  return {SceneErrorCode::InvalidReference, std::move(message)};
}

bool valid_node(const Scene& scene, std::uint32_t index) {
  return index < scene.nodes.size();
}

bool valid_string_or_none(const Scene& scene, std::uint32_t index) {
  return index == kNoIndex || index < scene.strings.size();
}

bool valid_material_or_none(const Scene& scene, std::uint32_t index) {
  return index == kNoIndex || index < scene.materials.size();
}

}  // namespace

const char* patch_op_name(PatchOpKind kind) {
  switch (kind) {
    case PatchOpKind::AddString:
      return "add-string";
    case PatchOpKind::RenameString:
      return "rename-string";
    case PatchOpKind::AddMaterial:
      return "add-material";
    case PatchOpKind::SetMaterialColor:
      return "set-material-color";
    case PatchOpKind::AddNode:
      return "add-node";
    case PatchOpKind::RemoveNode:
      return "remove-node";
    case PatchOpKind::ReparentNode:
      return "reparent-node";
    case PatchOpKind::SetTransform:
      return "set-transform";
    case PatchOpKind::SetNodeMaterial:
      return "set-node-material";
    case PatchOpKind::SetInstanceTarget:
      return "set-instance-target";
  }
  return "unknown";
}

PatchResult apply_patch(const Scene& scene, const ScenePatch& patch) {
  PatchResult result;
  result.scene = scene;
  for (const PatchOp& op : patch.ops) {
    switch (op.kind) {
      case PatchOpKind::AddString:
        result.scene.strings.push_back(op.text);
        break;
      case PatchOpKind::RenameString:
        if (op.index >= result.scene.strings.size()) {
          result.errors.push_back(patch_error("string index is out of range"));
        } else {
          result.scene.strings[op.index] = op.text;
        }
        break;
      case PatchOpKind::AddMaterial:
        if (!valid_string_or_none(result.scene, op.material.name)) {
          result.errors.push_back(patch_error("material name is out of range"));
        } else {
          result.scene.materials.push_back(op.material);
        }
        break;
      case PatchOpKind::SetMaterialColor:
        if (op.index >= result.scene.materials.size()) {
          result.errors.push_back(patch_error("material index is out of range"));
        } else {
          result.scene.materials[op.index].base_color = op.color;
        }
        break;
      case PatchOpKind::AddNode:
        if (!valid_string_or_none(result.scene, op.node.name) ||
            (op.node.parent != kNoIndex && op.node.parent >= result.scene.nodes.size()) ||
            !valid_material_or_none(result.scene, op.node.material)) {
          result.errors.push_back(patch_error("new node contains invalid references"));
        } else {
          result.scene.nodes.push_back(op.node);
        }
        break;
      case PatchOpKind::RemoveNode:
        if (!valid_node(result.scene, op.index)) {
          result.errors.push_back(patch_error("node index is out of range"));
        } else {
          result.scene.nodes.erase(result.scene.nodes.begin() + op.index);
          for (Node& node : result.scene.nodes) {
            if (node.parent == op.index) {
              node.parent = kNoIndex;
            } else if (node.parent > op.index && node.parent != kNoIndex) {
              --node.parent;
            }
            if (node.instance_target == op.index) {
              node.instance_target = kNoIndex;
            } else if (node.instance_target > op.index && node.instance_target != kNoIndex) {
              --node.instance_target;
            }
          }
        }
        break;
      case PatchOpKind::ReparentNode:
        if (!valid_node(result.scene, op.index) || (op.other != kNoIndex && !valid_node(result.scene, op.other))) {
          result.errors.push_back(patch_error("reparent references an invalid node"));
        } else {
          result.scene.nodes[op.index].parent = op.other;
        }
        break;
      case PatchOpKind::SetTransform:
        if (!valid_node(result.scene, op.index)) {
          result.errors.push_back(patch_error("transform references an invalid node"));
        } else {
          result.scene.nodes[op.index].local = op.transform;
        }
        break;
      case PatchOpKind::SetNodeMaterial:
        if (!valid_node(result.scene, op.index) || !valid_material_or_none(result.scene, op.other)) {
          result.errors.push_back(patch_error("material assignment references an invalid table entry"));
        } else {
          result.scene.nodes[op.index].material = op.other;
        }
        break;
      case PatchOpKind::SetInstanceTarget:
        if (!valid_node(result.scene, op.index) || !valid_node(result.scene, op.other)) {
          result.errors.push_back(patch_error("instance target references an invalid node"));
        } else {
          result.scene.nodes[op.index].instance_target = op.other;
          result.scene.nodes[op.index].kind = NodeKind::Instance;
        }
        break;
    }
  }
  const SceneError validation = validate_scene(result.scene);
  if (validation.code != SceneErrorCode::None) {
    result.errors.push_back(validation);
  }
  return result;
}

bool patch_has_errors(const PatchResult& result) {
  return !result.errors.empty();
}

ScenePatch rename_node_patch(std::uint32_t node, std::string name) {
  ScenePatch patch;
  patch.name = "rename-node";
  PatchOp add;
  add.kind = PatchOpKind::AddString;
  add.text = name;
  patch.ops.push_back(add);
  PatchOp rename;
  rename.kind = PatchOpKind::RenameString;
  rename.index = node;
  rename.text = std::move(name);
  patch.ops.push_back(rename);
  return patch;
}

ScenePatch reparent_patch(std::uint32_t node, std::uint32_t parent) {
  ScenePatch patch;
  patch.name = "reparent";
  PatchOp op;
  op.kind = PatchOpKind::ReparentNode;
  op.index = node;
  op.other = parent;
  patch.ops.push_back(op);
  return patch;
}

ScenePatch transform_patch(std::uint32_t node, Transform transform) {
  ScenePatch patch;
  patch.name = "transform";
  PatchOp op;
  op.kind = PatchOpKind::SetTransform;
  op.index = node;
  op.transform = transform;
  patch.ops.push_back(op);
  return patch;
}

std::string patch_to_text(const ScenePatch& patch) {
  std::ostringstream out;
  out << "patch " << patch.name << "\n";
  for (const PatchOp& op : patch.ops) {
    out << patch_op_name(op.kind) << " index=" << op.index << " other=" << op.other;
    if (!op.text.empty()) {
      out << " text=" << op.text;
    }
    out << "\n";
  }
  return out.str();
}

}  // namespace knotwork
