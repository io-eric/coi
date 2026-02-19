#pragma once

#include "node.h"
#include "statements.h"

// A single element in a tuple return type: (string name, int count) -> [{string, name}, {int, count}]
struct TupleElement {
    std::string type;
    std::string name;
};

struct FunctionDef {
    std::string name;
    std::string return_type;  // Used for single returns, empty when tuple_returns is used
    std::vector<TupleElement> tuple_returns;  // For multiple return values: (string a, int b)
    bool is_public = false;
    struct Param {
        std::string type;
        std::string name;
        bool is_mutable = false;
        bool is_reference = false;
    };
    std::vector<Param> params;
    std::vector<std::unique_ptr<Statement>> body;

    // Check if function returns a tuple
    bool returns_tuple() const { return !tuple_returns.empty(); }
    
    // Get the full return type string (for display/error messages)
    std::string get_return_type_string() const;
    
    // Get the generated struct name for tuple returns (e.g., "_Tup_string_int32")
    std::string get_tuple_struct_name() const;

    std::string to_webcc(const std::string& injected_code = "");
    void collect_modifications(std::set<std::string>& mods) const;
};

struct DataField {
    std::string type;
    std::string name;
};

struct DataDef : ASTNode {
    std::string name;
    std::string module_name;  // Module this type belongs to
    std::string source_file;  // Absolute path to the file this type is defined in
    bool is_public = false;   // Requires pub keyword to be importable
    std::vector<DataField> fields;

    std::string to_webcc() override;
};

// Enum definition: enum Mode { Idle, Running, Paused }
struct EnumDef : ASTNode {
    std::string name;
    std::string module_name;  // Module this enum belongs to
    std::string source_file;  // Absolute path to the file this enum is defined in
    bool is_public = false;   // Requires pub keyword to be importable
    std::vector<std::string> values;
    bool is_shared = false;
    std::string owner_component;

    std::string to_webcc() override;
};
