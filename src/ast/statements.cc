#include "statements.h"
#include "../schema_loader.h"

// Reference to per-component context for reference props
extern std::set<std::string> g_ref_props;

// Info for inlining DOM operations on component arrays used in for-each loops
struct ComponentArrayLoopInfo {
    int loop_id;
    std::string component_type;
    std::string parent_var;
    std::string var_name;
    std::string item_creation_code;
    bool is_member_ref_loop;
};
extern std::map<std::string, ComponentArrayLoopInfo> g_component_array_loops;

std::string VarDeclaration::to_webcc() {
    // Special handling for ArrayRepeatLiteral
    if (auto repeat = dynamic_cast<ArrayRepeatLiteral*>(initializer.get())) {
        std::string elem_type = type;
        if (elem_type.ends_with("[]")) {
            elem_type = elem_type.substr(0, elem_type.length() - 2);
        }
        std::string result = (is_mutable ? "" : "const ");
        result += "webcc::array<" + convert_type(elem_type) + ", " + std::to_string(repeat->count) + ">";
        result += " " + name + " = " + repeat->to_webcc() + ";";
        return result;
    }

    // Dynamic array literal initializer
    if (auto arr_lit = dynamic_cast<ArrayLiteral*>(initializer.get())) {
        if (type.ends_with("[]")) {
            std::string elem_type = type.substr(0, type.length() - 2);
            std::string vec_type = "webcc::vector<" + convert_type(elem_type) + ">";

            std::string result = (is_mutable ? "" : "const ") + vec_type;
            if (is_reference) result += "&";
            result += " " + name + " = [](){ " + vec_type + " _v; ";
            for (size_t i = 0; i < arr_lit->elements.size(); ++i) {
                result += "_v.push_back(" + arr_lit->elements[i]->to_webcc() + "); ";
            }
            result += "return _v; }();";
            return result;
        }
    }
    
    std::string result = (is_mutable ? "" : "const ") + convert_type(type);
    if(is_reference) result += "&";
    result += " " + name;
    if(initializer) {
        if (SchemaLoader::instance().is_handle(type)) {
            result += "{" + initializer->to_webcc() + "}";
        } else {
            result += " = " + initializer->to_webcc();
        }
    }
    result += ";";
    return result;
}

std::string ComponentParam::to_webcc() {
    return "";
}

std::string Assignment::to_webcc() {
    std::string lhs = name;
    if(g_ref_props.count(name)) {
        lhs = "(*" + name + ")";
    }
    
    std::string rhs = value->to_webcc();
    
    if (!target_type.empty() && SchemaLoader::instance().is_handle(target_type)) {
        rhs = convert_type(target_type) + "((int32_t)" + rhs + ")";
    }
    
    // For component array FULL REASSIGNMENT (arr = newArr), we need to:
    // 1. Remove views of all old items
    // 2. Assign the new array
    // 3. Render views of all new items
    // This is different from push/pop/clear which are single-item operations.
    // We do this inline, NOT via _sync_loop_X().
    auto it = g_component_array_loops.find(name);
    if (it != g_component_array_loops.end() && it->second.is_member_ref_loop) {
        const auto& info = it->second;
        std::string var = info.var_name;  // Use original loop variable name
        std::string result;
        // Destroy old items
        result += "for (auto& " + var + " : " + name + ") { " + var + "._remove_view(); }\n";
        // Do the assignment
        result += lhs + " = " + rhs + ";\n";
        // Render new items
        result += "g_view_depth++;\n";
        result += "for (auto& " + var + " : " + name + ") {\n";
        result += info.item_creation_code;
        result += "}\n";
        result += "if (--g_view_depth == 0) webcc::flush();";
        return result;
    }
    
    return lhs + " = " + rhs + ";";
}

void Assignment::collect_dependencies(std::set<std::string>& deps) {
    value->collect_dependencies(deps);
}

std::string IndexAssignment::to_webcc() {
    // Check if this is an index assignment on a component array with inline loop
    if (auto id = dynamic_cast<Identifier*>(array.get())) {
        auto it = g_component_array_loops.find(id->name);
        if (it != g_component_array_loops.end() && it->second.is_member_ref_loop) {
            // For component arrays, we need to:
            // 1. Do the data swap/assignment
            // 2. Move the DOM node to its correct position
            std::string arr = array->to_webcc();
            std::string idx = index->to_webcc();
            std::string parent_var = it->second.parent_var;
            
            std::string result;
            if (compound_op.empty()) {
                result = arr + "[" + idx + "] = " + value->to_webcc() + ";\n";
            } else {
                result = arr + "[" + idx + "] = " + arr + "[" + idx + "] " + compound_op + " " + value->to_webcc() + ";\n";
            }
            // Move the DOM node to correct position
            // Get the element that should be after this one (or null if at end)
            result += "{ int _idx = " + idx + ";\n";
            result += "  webcc::handle _node = " + arr + "[_idx]._get_root_element();\n";
            result += "  webcc::handle _ref = (_idx + 1 < (int)" + arr + ".size()) ? " + arr + "[_idx + 1]._get_root_element() : webcc::handle{0};\n";
            result += "  webcc::dom::move_before(" + parent_var + ", _node, _ref);\n";
            result += "}";
            return result;
        }
    }
    
    if (compound_op.empty()) {
        return array->to_webcc() + "[" + index->to_webcc() + "] = " + value->to_webcc() + ";";
    } else {
        std::string arr = array->to_webcc();
        std::string idx = index->to_webcc();
        return arr + "[" + idx + "] = " + arr + "[" + idx + "] " + compound_op + " " + value->to_webcc() + ";";
    }
}

void IndexAssignment::collect_dependencies(std::set<std::string>& deps) {
    array->collect_dependencies(deps);
    index->collect_dependencies(deps);
    value->collect_dependencies(deps);
}

std::string MemberAssignment::to_webcc() {
    if (compound_op.empty()) {
        return object->to_webcc() + "." + member + " = " + value->to_webcc() + ";";
    } else {
        std::string obj = object->to_webcc();
        return obj + "." + member + " = " + obj + "." + member + " " + compound_op + " " + value->to_webcc() + ";";
    }
}

void MemberAssignment::collect_dependencies(std::set<std::string>& deps) {
    object->collect_dependencies(deps);
    value->collect_dependencies(deps);
}

std::string ReturnStatement::to_webcc() {
    if (value) return "return " + value->to_webcc() + ";";
    return "return;";
}

void ReturnStatement::collect_dependencies(std::set<std::string>& deps) {
    if (value) value->collect_dependencies(deps);
}

std::string ExpressionStatement::to_webcc() {
    // Check for array method calls on component arrays used in loops
    if (auto call = dynamic_cast<FunctionCall*>(expression.get())) {
        size_t dot_pos = call->name.rfind('.');
        if (dot_pos != std::string::npos) {
            std::string arr_name = call->name.substr(0, dot_pos);
            std::string method = call->name.substr(dot_pos + 1);
            
            auto it = g_component_array_loops.find(arr_name);
            if (it != g_component_array_loops.end() && it->second.is_member_ref_loop) {
                const auto& info = it->second;
                std::string var = info.var_name;  // Use original loop variable name
                std::string result;
                
                if (method == "push" && call->args.size() == 1) {
                    // arr.push(item) -> add to array, bind callbacks, render view if parent exists
                    std::string item_expr = call->args[0]->to_webcc();
                    std::string parent_var = "_loop_" + std::to_string(info.loop_id) + "_parent";
                    result = arr_name + ".push_back(" + item_expr + ");\n";
                    // Only render view if parent container exists (not during init)
                    result += "if (" + parent_var + ".is_valid()) {\n";
                    result += "auto& " + var + " = " + arr_name + "[" + arr_name + ".size() - 1];\n";
                    // Inject the item creation code (callback bindings + view call)
                    result += info.item_creation_code;
                    result += "}\n";
                    return result;
                }
                else if (method == "pop" && call->args.empty()) {
                    // arr.pop() -> remove view then pop from array
                    result = "if (!" + arr_name + ".empty()) {\n";
                    result += "    " + arr_name + ".back()._remove_view();\n";
                    result += "    " + arr_name + ".pop_back();\n";
                    result += "}\n";
                    return result;
                }
                else if (method == "clear" && call->args.empty()) {
                    // arr.clear() -> destroy all views then clear array
                    result = "for (auto& " + var + " : " + arr_name + ") { " + var + "._remove_view(); }\n";
                    result += arr_name + ".clear();\n";
                    return result;
                }
            }
        }
    }
    return expression->to_webcc() + ";\n";
}

void ExpressionStatement::collect_dependencies(std::set<std::string>& deps) {
    expression->collect_dependencies(deps);
}

std::string BlockStatement::to_webcc() {
    std::string code = "{\n";
    for(auto& stmt : statements) code += stmt->to_webcc();
    code += "}\n";
    return code;
}

void BlockStatement::collect_dependencies(std::set<std::string>& deps) {
    for(auto& stmt : statements) stmt->collect_dependencies(deps);
}

std::string IfStatement::to_webcc() {
    std::string code = "if(" + condition->to_webcc() + ") ";
    code += then_branch->to_webcc();
    if(else_branch){
        code += " else ";
        code += else_branch->to_webcc();
    }
    return code;
}

void IfStatement::collect_dependencies(std::set<std::string>& deps) {
    condition->collect_dependencies(deps);
    then_branch->collect_dependencies(deps);
    if(else_branch) else_branch->collect_dependencies(deps);
}

std::string ForRangeStatement::to_webcc() {
    std::string code = "for(int " + var_name + " = " + start->to_webcc() + "; ";
    code += "(" + var_name + " < " + end->to_webcc() + "); ";
    code += var_name + "++) ";
    code += body->to_webcc();
    return code;
}

void ForRangeStatement::collect_dependencies(std::set<std::string>& deps) {
    start->collect_dependencies(deps);
    end->collect_dependencies(deps);
    body->collect_dependencies(deps);
}

std::string ForEachStatement::to_webcc() {
    std::string code = "for(auto& " + var_name + " : " + iterable->to_webcc() + ") ";
    code += body->to_webcc();
    return code;
}

void ForEachStatement::collect_dependencies(std::set<std::string>& deps) {
    iterable->collect_dependencies(deps);
    body->collect_dependencies(deps);
}

void collect_mods_recursive(Statement* stmt, std::set<std::string>& mods) {
    if(auto assign = dynamic_cast<Assignment*>(stmt)) {
        mods.insert(assign->name);
    }
    else if(auto idxAssign = dynamic_cast<IndexAssignment*>(stmt)) {
        if(auto id = dynamic_cast<Identifier*>(idxAssign->array.get())) {
            // Don't mark component arrays as modified for index assignment
            // Swapping components doesn't need DOM sync - they're already rendered
            if (g_component_array_loops.find(id->name) == g_component_array_loops.end()) {
                mods.insert(id->name);
            }
        }
    }
    else if(auto memberAssign = dynamic_cast<MemberAssignment*>(stmt)) {
        // Track the root object being modified
        Expression* obj = memberAssign->object.get();
        while(auto member = dynamic_cast<MemberAccess*>(obj)) {
            obj = member->object.get();
        }
        if(auto id = dynamic_cast<Identifier*>(obj)) {
            mods.insert(id->name);
        }
    }
    else if(auto exprStmt = dynamic_cast<ExpressionStatement*>(stmt)) {
        if(auto postfix = dynamic_cast<PostfixOp*>(exprStmt->expression.get())) {
            if(auto id = dynamic_cast<Identifier*>(postfix->operand.get())) {
                mods.insert(id->name);
            }
        }
        else if(auto unary = dynamic_cast<UnaryOp*>(exprStmt->expression.get())) {
             if(unary->op == "++" || unary->op == "--") {
                 if(auto id = dynamic_cast<Identifier*>(unary->operand.get())) {
                     mods.insert(id->name);
                 }
             }
        }
        else if(auto call = dynamic_cast<FunctionCall*>(exprStmt->expression.get())) {
            size_t dot_pos = call->name.rfind('.');
            if(dot_pos != std::string::npos) {
                std::string method = call->name.substr(dot_pos + 1);
                if(method == "push" || method == "push_back" || 
                   method == "pop" || method == "pop_back" || 
                   method == "clear") {
                    std::string obj = call->name.substr(0, dot_pos);
                    mods.insert(obj);
                }
            }
        }
    }
    else if(auto block = dynamic_cast<BlockStatement*>(stmt)) {
        for(auto& s : block->statements) {
            collect_mods_recursive(s.get(), mods);
        }
    }
    else if(auto ifStmt = dynamic_cast<IfStatement*>(stmt)) {
        collect_mods_recursive(ifStmt->then_branch.get(), mods);
        if(ifStmt->else_branch) {
            collect_mods_recursive(ifStmt->else_branch.get(), mods);
        }
    }
    else if(auto forRange = dynamic_cast<ForRangeStatement*>(stmt)) {
        collect_mods_recursive(forRange->body.get(), mods);
    }
    else if(auto forEach = dynamic_cast<ForEachStatement*>(stmt)) {
        collect_mods_recursive(forEach->body.get(), mods);
    }
}
