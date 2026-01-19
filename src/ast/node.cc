#include "node.h"
#include "../def_parser.h"
#include <cctype>
#include <algorithm>

std::string convert_type(const std::string& type) {
    if (type == "string") return "webcc::string";
    if (type == "float") return "double";    // float -> 64-bit (default, matches WASM/JS)
    if (type == "float32") return "float";   // explicit 32-bit
    // Handle Component.EnumName type syntax - convert to Component::EnumName
    if (type.find('.') != std::string::npos) {
        std::string result = type;
        size_t pos = result.find('.');
        result.replace(pos, 1, "::");
        return result;
    }
    // Handle dynamic arrays: T[]
    if (type.ends_with("[]")) {
        std::string inner = type.substr(0, type.length() - 2);
        return "webcc::vector<" + convert_type(inner) + ">";
    }
    // Handle fixed-size arrays: T[N]
    size_t bracket_pos = type.rfind('[');
    if (bracket_pos != std::string::npos && type.back() == ']') {
        std::string size_str = type.substr(bracket_pos + 1, type.length() - bracket_pos - 2);
        // Check if it's a number (fixed-size array)
        bool is_number = !size_str.empty() && std::all_of(size_str.begin(), size_str.end(), ::isdigit);
        if (is_number) {
            std::string inner = type.substr(0, bracket_pos);
            return "webcc::array<" + convert_type(inner) + ", " + size_str + ">";
        }
    }
    // Check if type is a webcc handle type and add prefix
    if (DefSchema::instance().is_handle(type)) {
        return "webcc::" + type;
    }
    return type;
}
