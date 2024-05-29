#include "utils.h"
#include <fstream>

bool fileExists(const std::string& filePath) {
    std::ifstream file(filePath);
    return file.good();
}

std::string getFileName(const std::string& filePath) {
    size_t pos = filePath.find_last_of("\\/");
    if (pos == std::string::npos) {
        return filePath;
    } else {
        return filePath.substr(pos + 1);
    }
}
