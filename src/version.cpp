#include "knotwork/version.hpp"

#include <sstream>

namespace knotwork {

VersionInfo version_info() {
  return {};
}

std::string version_string() {
  const VersionInfo info = version_info();
  std::ostringstream out;
  out << info.name << " " << info.major << "." << info.minor << "." << info.patch;
  return out.str();
}

std::string build_capabilities() {
  std::ostringstream out;
  out << "scene-format=binary\n";
  out << "mesh-format=binary,obj,procedural\n";
  out << "exports=json,text,yaml,csv,markdown\n";
  out << "tools=info,seed,doctor,convert,mesh,script,schema,debug\n";
  out << "validation=basic,detailed,audit,consistency\n";
  return out.str();
}

}  // namespace knotwork
