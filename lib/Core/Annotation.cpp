#include "klee/Core/Annotation.h"

#include "klee/Support/ErrorHandling.h"

#include <fstream>

namespace klee {

Annotation::StatementName parseStatementName(const std::string &name) {
  const auto statementName = statementsAsStr.find(name);
  if (statementName != statementsAsStr.end()) {
    return statementName->second;
  }
  return Annotation::StatementName::Unknown;
}

std::vector<std::string> parseStatementOffset(const std::string &offsetStr) {
  std::vector<std::string> offset;
  size_t pos = 0;
  while (pos < offsetStr.size()) {
    if (offsetStr[pos] == '*') {
      offset.emplace_back("*");
      pos++;
    } else if (offsetStr[pos] == '&') {
      offset.emplace_back("&");
      pos++;
    } else if (offsetStr[pos] == '[') {
      size_t posEndExpr = offsetStr.find(']', pos);
      if (posEndExpr == std::string::npos) {
        klee_error("Incorrect annotation offset format.");
      }
      offset.push_back(offsetStr.substr(pos + 1, posEndExpr - 1 - pos));
      pos = posEndExpr + 1;
    } else {
      klee_error("Incorrect annotation offset format.");
    }
  }
  return offset;
}

void from_json(const json &j, Annotation::Statement &statement) {
  if (!j.is_string()) {
    klee_error("Incorrect annotation format.");
  }
  std::string jStr = j.get<std::string>();

  size_t delimiterPos = jStr.find(':');
  if (delimiterPos == std::string::npos) {
    statement.name = parseStatementName(jStr);
    return;
  }

  statement.name = parseStatementName(jStr.substr(0, delimiterPos));
  size_t delimiterPosNext = jStr.find(':', delimiterPos + 1);
  if (delimiterPosNext == std::string::npos) {
    statement.offset = parseStatementOffset(jStr.substr(delimiterPos + 1));
    return;
  }

  if (delimiterPos + 1 != delimiterPosNext) {
    statement.offset = parseStatementOffset(
        jStr.substr(delimiterPos + 1, delimiterPosNext - delimiterPos - 1));
  }
  statement.data = jStr.substr(delimiterPosNext + 1);
}

void from_json(const json &j, Annotation::Property &property) {
  if (!j.is_string()) {
    klee_error("Incorrect properties format in annotations file.");
  }
  std::string jStr = j.get<std::string>();

  property = Annotation::Property::Unknown;
  const auto propertyPtr = propertiesAsStr.find(jStr);
  if (propertyPtr != propertiesAsStr.end()) {
    property = propertyPtr->second;
  }
}

void from_json(const json &j, Annotation &annotation) {
  if (!j.is_object() || !j.contains("annotation") ||
      !j.contains("properties")) {
    klee_error("Incorrect annotations file format.");
  }

  annotation.statements =
      j.at("annotation").get<std::vector<Annotation::Statements>>();
  annotation.properties =
      j.at("properties").get<std::vector<Annotation::Property>>();
}

Annotations parseAnnotationsFile(std::string &path) {
  std::ifstream annotationsFile(path);
  if (!annotationsFile.good()) {
    klee_error("Opening %s failed.", path.c_str());
  }

  json annotationsJson = json::parse(annotationsFile, nullptr, false);
  if (annotationsJson.is_discarded()) {
    klee_error("Parsing JSON %s failed.", path.c_str());
  }

  Annotations annotations;
  for (auto &item : annotationsJson.items()) {
    Annotation annotation;
    annotation.functionName = item.key();
    from_json(item.value(), annotation);
    annotations[item.key()] = annotation;
  }

  return annotations;
}

} // namespace klee
