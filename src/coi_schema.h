// GENERATED FILE - DO NOT EDIT
// Minimal schema for coi compiler (no JS action strings)
#pragma once
#include <string>
#include <vector>

namespace coi {

struct SchemaParam {
    std::string type;        // For handles, this is the actual handle type (e.g., "DOMElement")
    std::string name;
};

struct SchemaEntry {
    std::string ns;
    std::string func_name;
    std::vector<SchemaParam> params;
    std::string return_type; // For handles, this is the actual handle type (e.g., "Canvas")
};

extern const SchemaEntry SCHEMA[];
extern const size_t SCHEMA_COUNT;

extern const char* HANDLES[];
extern const size_t HANDLE_COUNT;

// Handle inheritance: maps derived type -> base type
// e.g., Canvas -> DOMElement means Canvas can be used where DOMElement is expected
extern const std::pair<const char*, const char*> HANDLE_INHERITANCE[];

} // namespace coi
