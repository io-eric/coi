// Generates a minimal schema header for the coi compiler
// Only includes fields coi actually needs (no JS action strings)
// Uses the already-parsed webcc_schema.h for consistency

#include <iostream>
#include <fstream>
#include <set>
#include <string>
#include <vector>

// Include webcc's schema definitions
#include "../deps/webcc/src/cli/schema.h"
#include "../deps/webcc/src/cli/webcc_schema.h"

int main() {
    std::set<std::string> handles;

    // Collect all handle types from commands
    for (const auto* c = webcc::SCHEMA_COMMANDS; !c->ns.empty(); ++c) {
        // Check return handle type
        if (!c->return_handle_type.empty()) {
            handles.insert(c->return_handle_type);
        }
        // Check param handle types
        for (const auto& p : c->params) {
            if (!p.handle_type.empty()) {
                handles.insert(p.handle_type);
            }
        }
    }

    // Collect from events too
    for (const auto* e = webcc::SCHEMA_EVENTS; !e->ns.empty(); ++e) {
        for (const auto& p : e->params) {
            if (!p.handle_type.empty()) {
                handles.insert(p.handle_type);
            }
        }
    }

    // Collect from inheritance
    for (const auto* kv = webcc::HANDLE_INHERITANCE; kv->first != nullptr; ++kv) {
        handles.insert(kv->first);
        handles.insert(kv->second);
    }

    // Generate Header
    {
        std::ofstream out("src/coi_schema.h");
        if (!out) {
            std::cerr << "[COI] Error: Cannot create src/coi_schema.h" << std::endl;
            return 1;
        }

        out << R"(// GENERATED FILE - DO NOT EDIT
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
)";
        out.close();
    }

    // Generate Source
    {
        std::ofstream out("src/coi_schema.cc");
        if (!out) {
            std::cerr << "[COI] Error: Cannot create src/coi_schema.cc" << std::endl;
            return 1;
        }

        out << R"(// GENERATED FILE - DO NOT EDIT
#include "coi_schema.h"

namespace coi {

const SchemaEntry SCHEMA[] = {
)";

        size_t count = 0;
        for (const auto* c = webcc::SCHEMA_COMMANDS; !c->ns.empty(); ++c) {
            out << "    {\"" << c->ns << "\", \"" << c->func_name << "\", {";
            
            bool first = true;
            for (const auto& p : c->params) {
                if (!first) out << ", ";
                first = false;
                
                // Use handle_type if it's a handle, otherwise use the base type
                std::string type = p.type;
                if (p.type == "handle" && !p.handle_type.empty()) {
                    type = p.handle_type;
                }
                out << "{\"" << type << "\", \"" << p.name << "\"}";
            }
            out << "}, ";
            
            // Return type: use handle_type if it's a handle return
            std::string ret_type = c->return_type;
            if (c->return_type == "handle" && !c->return_handle_type.empty()) {
                ret_type = c->return_handle_type;
            }
            out << "\"" << ret_type << "\"},\n";
            
            count++;
        }

        out << "};\n\n";
        out << "const size_t SCHEMA_COUNT = " << count << ";\n\n";

        out << "const char* HANDLES[] = {\n";
        for (const auto& h : handles) {
            out << "    \"" << h << "\",\n";
        }
        out << "};\n\n";
        out << "const size_t HANDLE_COUNT = " << handles.size() << ";\n\n";

        // Generate inheritance table
        out << "const std::pair<const char*, const char*> HANDLE_INHERITANCE[] = {\n";
        for (const auto* kv = webcc::HANDLE_INHERITANCE; kv->first != nullptr; ++kv) {
            out << "    { \"" << kv->first << "\", \"" << kv->second << "\" },\n";
        }
        out << "    { nullptr, nullptr }\n";
        out << "};\n\n";

        out << "} // namespace coi\n";
        out.close();
        
        std::cout << "[COI] Generated coi_schema.h and coi_schema.cc with " << count << " entries and " << handles.size() << " handles" << std::endl;
    }

    return 0;
}
