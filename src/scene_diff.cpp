#include "knotwork/scene_diff.hpp"

#include <sstream>

namespace knotwork {
namespace {

std::string node_summary(const Scene& scene, const Node& node) {
  std::ostringstream out;
  out << "id=" << node.id << " name=" << scene.string_at(node.name).value_or("<invalid>")
      << " kind=" << node_kind_name(node.kind) << " parent=" << node.parent << " material="
      << node.material << " instance=" << node.instance_target << " t=" << node.local.translation.to_string();
  return out.str();
}

std::string material_summary(const Scene& scene, const Material& material) {
  std::ostringstream out;
  out << "name=" << scene.string_at(material.name).value_or("<invalid>")
      << " color=" << material.base_color.to_string() << " roughness=" << material.roughness
      << " metallic=" << material.metallic;
  return out.str();
}

void add_entry(SceneDiff& diff, DiffKind kind, std::uint32_t index, std::string path, std::string before, std::string after) {
  diff.entries.push_back({kind, index, std::move(path), std::move(before), std::move(after)});
}

}  // namespace

const char* diff_kind_name(DiffKind kind) {
  switch (kind) {
    case DiffKind::StringAdded:
      return "string-added";
    case DiffKind::StringRemoved:
      return "string-removed";
    case DiffKind::StringChanged:
      return "string-changed";
    case DiffKind::MaterialAdded:
      return "material-added";
    case DiffKind::MaterialRemoved:
      return "material-removed";
    case DiffKind::MaterialChanged:
      return "material-changed";
    case DiffKind::NodeAdded:
      return "node-added";
    case DiffKind::NodeRemoved:
      return "node-removed";
    case DiffKind::NodeChanged:
      return "node-changed";
  }
  return "unknown";
}

SceneDiff diff_scenes(const Scene& before, const Scene& after) {
  SceneDiff diff;
  const std::size_t string_count = std::max(before.strings.size(), after.strings.size());
  for (std::uint32_t i = 0; i < string_count; ++i) {
    if (i >= before.strings.size()) {
      add_entry(diff, DiffKind::StringAdded, i, "strings", "", after.strings[i]);
    } else if (i >= after.strings.size()) {
      add_entry(diff, DiffKind::StringRemoved, i, "strings", before.strings[i], "");
    } else if (before.strings[i] != after.strings[i]) {
      add_entry(diff, DiffKind::StringChanged, i, "strings", before.strings[i], after.strings[i]);
    }
  }
  const std::size_t material_count = std::max(before.materials.size(), after.materials.size());
  for (std::uint32_t i = 0; i < material_count; ++i) {
    if (i >= before.materials.size()) {
      add_entry(diff, DiffKind::MaterialAdded, i, "materials", "", material_summary(after, after.materials[i]));
    } else if (i >= after.materials.size()) {
      add_entry(diff, DiffKind::MaterialRemoved, i, "materials", material_summary(before, before.materials[i]), "");
    } else {
      const std::string left = material_summary(before, before.materials[i]);
      const std::string right = material_summary(after, after.materials[i]);
      if (left != right) {
        add_entry(diff, DiffKind::MaterialChanged, i, "materials", left, right);
      }
    }
  }
  const std::size_t node_count = std::max(before.nodes.size(), after.nodes.size());
  for (std::uint32_t i = 0; i < node_count; ++i) {
    if (i >= before.nodes.size()) {
      add_entry(diff, DiffKind::NodeAdded, i, "nodes", "", node_summary(after, after.nodes[i]));
    } else if (i >= after.nodes.size()) {
      add_entry(diff, DiffKind::NodeRemoved, i, "nodes", node_summary(before, before.nodes[i]), "");
    } else {
      const std::string left = node_summary(before, before.nodes[i]);
      const std::string right = node_summary(after, after.nodes[i]);
      if (left != right) {
        add_entry(diff, DiffKind::NodeChanged, i, "nodes", left, right);
      }
    }
  }
  return diff;
}

bool empty(const SceneDiff& diff) {
  return diff.entries.empty();
}

std::string diff_to_text(const SceneDiff& diff) {
  std::ostringstream out;
  for (const DiffEntry& entry : diff.entries) {
    out << diff_kind_name(entry.kind) << " " << entry.path << "[" << entry.index << "]\n";
    if (!entry.before.empty()) {
      out << "  - " << entry.before << "\n";
    }
    if (!entry.after.empty()) {
      out << "  + " << entry.after << "\n";
    }
  }
  return out.str();
}

ScenePatch patch_from_diff(const Scene& before, const Scene& after) {
  ScenePatch patch;
  patch.name = "diff";
  for (std::uint32_t i = static_cast<std::uint32_t>(before.nodes.size()); i > after.nodes.size(); --i) {
    PatchOp op;
    op.kind = PatchOpKind::RemoveNode;
    op.index = i - 1;
    patch.ops.push_back(op);
  }
  for (std::uint32_t i = static_cast<std::uint32_t>(before.strings.size()); i < after.strings.size(); ++i) {
    PatchOp op;
    op.kind = PatchOpKind::AddString;
    op.text = after.strings[i];
    patch.ops.push_back(op);
  }
  for (std::uint32_t i = 0; i < std::min(before.strings.size(), after.strings.size()); ++i) {
    if (before.strings[i] != after.strings[i]) {
      PatchOp op;
      op.kind = PatchOpKind::RenameString;
      op.index = i;
      op.text = after.strings[i];
      patch.ops.push_back(op);
    }
  }
  for (std::uint32_t i = static_cast<std::uint32_t>(before.materials.size()); i < after.materials.size(); ++i) {
    PatchOp op;
    op.kind = PatchOpKind::AddMaterial;
    op.material = after.materials[i];
    patch.ops.push_back(op);
  }
  for (std::uint32_t i = static_cast<std::uint32_t>(before.nodes.size()); i < after.nodes.size(); ++i) {
    PatchOp op;
    op.kind = PatchOpKind::AddNode;
    op.node = after.nodes[i];
    patch.ops.push_back(op);
  }
  for (std::uint32_t i = 0; i < std::min(before.nodes.size(), after.nodes.size()); ++i) {
    if (!nearly_equal(before.nodes[i].local.matrix(), after.nodes[i].local.matrix())) {
      PatchOp op;
      op.kind = PatchOpKind::SetTransform;
      op.index = i;
      op.transform = after.nodes[i].local;
      patch.ops.push_back(op);
    }
    if (before.nodes[i].parent != after.nodes[i].parent) {
      PatchOp op;
      op.kind = PatchOpKind::ReparentNode;
      op.index = i;
      op.other = after.nodes[i].parent;
      patch.ops.push_back(op);
    }
    if (before.nodes[i].material != after.nodes[i].material) {
      PatchOp op;
      op.kind = PatchOpKind::SetNodeMaterial;
      op.index = i;
      op.other = after.nodes[i].material;
      patch.ops.push_back(op);
    }
  }
  return patch;
}

}  // namespace knotwork
