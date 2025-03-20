#ifndef STRINGUTIL_H
#define STRINGUTIL_H
#include <sstream>
#include <string>

inline std::string indentString(const std::string &inputString,
                                int indentLevel) {
  if (indentLevel <= 0) {
    return inputString;
  }
  std::string indentStr = "";
  for (int i = 0; i < indentLevel; ++i) {
    indentStr += '\t';
  }
  std::stringstream ss(inputString);
  std::string line;
  std::string indentedString = "";
  bool firstLine = true;
  while (std::getline(ss, line)) {
    if (!firstLine) {
      indentedString += "\n";
    } else if (!inputString.empty()) {
      firstLine = false;
    }
    indentedString += indentStr + line;
  }
  return indentedString;
}

#endif // STRINGUTIL_H
