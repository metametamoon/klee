#include "klee/Core/FunctionAnnotation.h"

#include "klee/Support/ErrorHandling.h"

#include <fstream>

namespace klee {

// TODO: maybe for errors it's better to pass the function name.

FunctionAnnotation::StatementType parseStatementType(const std::string &type) {
  if (type == "Deref") {
    return FunctionAnnotation::StatementType::Deref;
  }
  if (type == "InitNull") {
    return FunctionAnnotation::StatementType::InitNull;
  }
  klee_error("Non-existent annotation type in annotations file.");
}

std::vector<std::string> parseStatementOffset(const std::string &offsetStr) {
  std::vector<std::string> offset;

  size_t pos = 0;
  while (pos < offsetStr.size()) {
    if (offsetStr[pos] == '*') {
      offset.emplace_back("*");
      pos++;
    }
    else if (offsetStr[pos] == '&') {
      offset.emplace_back("&");
      pos++;
    }
    else if (offsetStr[pos] == '[') {
      size_t posEndExpr = offsetStr.find(']', pos);
      if (posEndExpr == std::string::npos) {
        klee_error("Incorrect annotation offset format in annotations file.");
      }
      offset.push_back(offsetStr.substr(pos + 1, posEndExpr - 1 - pos));
      pos = posEndExpr + 1;
    }
    else {
      klee_error("Incorrect annotation offset format in annotations file.");
    }
  }

  return offset;
}

void from_json(const json &j, FunctionAnnotation::Statement &statement) {
  if (!j.is_string()) {
    klee_error("Incorrect annotation format in annotations file.");
  }
  std::string jStr = j.get<std::string>();

  size_t delPos = jStr.find(':');
  if (delPos == std::string::npos) {
    statement.type = parseStatementType(jStr);
    return;
  }

  statement.type = parseStatementType(jStr.substr(0, delPos));
  size_t delPosNext = jStr.find(':', delPos + 1);
  if (delPosNext == std::string::npos) {
    statement.offset = parseStatementOffset(jStr.substr(delPos + 1));
    return;
  }

  if (delPos + 1 != delPosNext) {
    statement.offset =
        parseStatementOffset(jStr.substr(delPos + 1, delPosNext - delPos - 1));
  }
  statement.data = jStr.substr(delPosNext + 1);
}

void from_json(const json &j, FunctionAnnotation::PropertyType &type) {
  if (!j.is_string()) {
    klee_error("Incorrect properties format in annotations file.");
  }

  std::string jStr = j.get<std::string>();
  if (jStr == "determ") {
    type = FunctionAnnotation::PropertyType::Determ;
    return;
  }
  if (jStr == "noreturn") {
    type = FunctionAnnotation::PropertyType::Noreturn;
    return;
  }
  klee_error("Non-existent property type in annotations file.");
}

void from_json(const json &j, FunctionAnnotation &annotation)
{
  if (!j.is_object() || j.size() != 2 ||
      !j.contains("annotation") || !j.contains("properties")) {
    klee_error("Incorrect annotations file format.");
  }

  annotation.properties =
      j.at("properties").get<std::vector<FunctionAnnotation::PropertyType>>();
  annotation.statementsOfParams =
      j.at("annotation").get<std::vector<FunctionAnnotation::Statements>>();
}

FunctionAnnotations parseAnnotationsFile(std::string &path) {
  std::ifstream annotationsFile(path);
  if (!annotationsFile.good()) {
    klee_error("Opening %s failed.", path.c_str());
  }

  json annotationsJson = json::parse(annotationsFile, nullptr, false);
  if (annotationsJson.is_discarded()) {
    klee_error("Parsing JSON %s failed.", path.c_str());
  }

  FunctionAnnotations annotations;
  for (auto& item : annotationsJson.items()) {
    FunctionAnnotation annotation;
    annotation.name = item.key();
    from_json(item.value(), annotation);
    annotations[item.key()] = annotation;
  }

  return annotations;
}

} // namespace klee
