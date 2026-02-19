#include "definitions.h"
#include "node.h"

std::string FunctionDef::get_return_type_string() const {
    if (tuple_returns.empty()) {
        return return_type;
    }
    std::string result = "(";
    for (size_t i = 0; i < tuple_returns.size(); i++) {
        if (i > 0) result += ", ";
        result += tuple_returns[i].type + " " + tuple_returns[i].name;
    }
    result += ")";
    return result;
}

std::string FunctionDef::get_tuple_struct_name() const {
    if (tuple_returns.empty()) return "";
    
    std::string name = "_Tup";
    for (const auto& elem : tuple_returns) {
        std::string t = convert_type(elem.type);
        // Sanitize type name for struct identifier (replace ::, <, >, etc.)
        for (char& c : t) {
            if (c == ':' || c == '<' || c == '>' || c == ' ' || c == ',') c = '_';
        }
        name += "_" + t + "_" + elem.name;
    }
    return name;
}

std::string FunctionDef::to_webcc(const std::string& injected_code) {
    ComponentTypeContext::instance().begin_method_scope();

    std::string result;
    
    // Handle tuple return type - use generated struct name
    if (returns_tuple()) {
        result = get_tuple_struct_name() + " " + name + "(";
    } else {
        result = convert_type(return_type) + " " + name + "(";
    }
    
    for(size_t i = 0; i < params.size(); i++){
        if(i > 0) result += ", ";
        result += (params[i].is_mutable ? "" : "const ") + convert_type(params[i].type);
        if(params[i].is_reference) result += "&";
        result += " " + params[i].name;

        ComponentTypeContext::instance().set_method_symbol_type(params[i].name, params[i].type);
    }
    
    result += ") {\n";
    for(auto& stmt : body){
        result += "    " + stmt->to_webcc() + "\n";
    }
    if(!injected_code.empty()) {
        result += injected_code;
    }
    result += "}\n";

    ComponentTypeContext::instance().end_method_scope();
    return result;
}

void FunctionDef::collect_modifications(std::set<std::string>& mods) const {
    for(const auto& stmt : body) {
        collect_mods_recursive(stmt.get(), mods);
    }
}

std::string DataDef::to_webcc() {
    std::stringstream ss;
    ss << "struct " << qualified_name(module_name, name) << " {\n";
    for(const auto& field : fields){
        ss << "    " << convert_type(field.type) << " " << field.name << ";\n";
    }
    ss << "};\n";
    return ss.str();
}

std::string EnumDef::to_webcc() {
    std::stringstream ss;
    size_t total_values = values.size() + 1; // Including _COUNT

    // Explicitly select the smallest possible type for ALL sizes
    ss << "enum struct " << qualified_name(module_name, name) << " : ";
    
    if (total_values <= 256) {
        ss << "uint8_t";
    } else if (total_values <= 65536) {
        ss << "uint16_t";
    } else {
        ss << "uint32_t"; // Fallback for massive enums
    }
    
    ss << " {\n";

    // Generate entries
    for (const auto& val : values) {
        ss << "    " << val << ",\n";
    }

    // Add _COUNT and close
    ss << "    _COUNT\n";
    ss << "};\n";
    
    return ss.str();
}