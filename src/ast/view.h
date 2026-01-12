#pragma once

#include "node.h"
#include "expressions.h"
#include <map>

struct TextNode : ASTNode {
    std::string text;
    TextNode(const std::string& t) : text(t) {}
    std::string to_webcc() override;
};

struct HTMLAttribute {
    std::string name;
    std::unique_ptr<Expression> value;
};

struct EventHandler {
    int element_id;
    std::string event_type;      // "click", "input", "change", "keydown"
    std::string handler_code;
    bool is_function_call;
};

struct Binding {
    int element_id;
    std::string type;            // "attr" or "text"
    std::string name;            // attribute name
    std::string value_code;
    std::set<std::string> dependencies;
    Expression* expr = nullptr;
    int if_region_id = -1;
    bool in_then_branch = true;
};

struct ComponentProp {
    std::string name;
    std::unique_ptr<Expression> value;
    bool is_reference = false;
    bool is_mutable_def = false;
};

// Struct to track reactive loop regions 
struct LoopRegion {
    int loop_id;
    std::string parent_element;
    std::string component_type;
    std::string start_expr;
    std::string end_expr;
    std::set<std::string> dependencies;
    std::string item_creation_code;
    std::string item_update_code;
    std::string var_name;
    std::string root_element_var;
    bool is_html_loop = false;
    bool is_keyed = false;
    std::string key_expr;
    std::string key_type;
    std::string iterable_expr;
};

// Struct to track reactive if/else regions
struct IfRegion {
    int if_id;
    std::string condition_code;
    std::set<std::string> dependencies;
    std::set<MemberDependency> member_dependencies;
    std::string then_creation_code;
    std::string else_creation_code;
    std::string then_destroy_code;
    std::string else_destroy_code;
    std::vector<int> then_element_ids;
    std::vector<int> else_element_ids;
    std::vector<std::pair<std::string, int>> then_components;
    std::vector<std::pair<std::string, int>> else_components;
    std::vector<int> then_loop_ids;
    std::vector<int> else_loop_ids;
    std::vector<int> then_if_ids;
    std::vector<int> else_if_ids;
};

struct ComponentInstantiation : ASTNode {
    std::string component_name;
    std::vector<ComponentProp> props;
    
    std::string to_webcc() override;

    void generate_code(std::stringstream& ss, const std::string& parent, int& counter, 
                      std::vector<EventHandler>& event_handlers,
                      std::vector<Binding>& bindings,
                      std::map<std::string, int>& component_counters,
                      const std::set<std::string>& method_names,
                      const std::string& parent_component_name,
                      bool in_loop = false,
                      std::vector<LoopRegion>* loop_regions = nullptr,
                      int* loop_counter = nullptr,
                      std::vector<IfRegion>* if_regions = nullptr,
                      int* if_counter = nullptr);
    
    void collect_dependencies(std::set<std::string>& deps) override;
};

struct HTMLElement : ASTNode {
    std::string tag;
    std::vector<HTMLAttribute> attributes;
    std::vector<std::unique_ptr<ASTNode>> children;
    std::string ref_binding;

    std::string to_webcc() override;

    void generate_code(std::stringstream& ss, const std::string& parent, int& counter, 
                      std::vector<EventHandler>& event_handlers,
                      std::vector<Binding>& bindings,
                      std::map<std::string, int>& component_counters,
                      const std::set<std::string>& method_names,
                      const std::string& parent_component_name,
                      bool in_loop = false,
                      std::vector<LoopRegion>* loop_regions = nullptr,
                      int* loop_counter = nullptr,
                      std::vector<IfRegion>* if_regions = nullptr,
                      int* if_counter = nullptr);
    void collect_dependencies(std::set<std::string>& deps) override;
};

// Conditional rendering in view (if/else)
struct ViewIfStatement : ASTNode {
    std::unique_ptr<Expression> condition;
    std::vector<std::unique_ptr<ASTNode>> then_children;
    std::vector<std::unique_ptr<ASTNode>> else_children;
    int if_id = -1;

    std::string to_webcc() override { return ""; }
    void generate_code(std::stringstream& ss, const std::string& parent, int& counter, 
                      std::vector<EventHandler>& event_handlers,
                      std::vector<Binding>& bindings,
                      std::map<std::string, int>& component_counters,
                      const std::set<std::string>& method_names,
                      const std::string& parent_component_name,
                      bool in_loop = false,
                      std::vector<LoopRegion>* loop_regions = nullptr,
                      int* loop_counter = nullptr,
                      std::vector<IfRegion>* if_regions = nullptr,
                      int* if_counter = nullptr);
    void collect_dependencies(std::set<std::string>& deps) override;
};

// For range loop in view (for i in 0:10)
struct ViewForRangeStatement : ASTNode {
    std::string var_name;
    std::unique_ptr<Expression> start;
    std::unique_ptr<Expression> end;
    std::vector<std::unique_ptr<ASTNode>> children;
    int loop_id = -1;

    std::string to_webcc() override { return ""; }
    void generate_code(std::stringstream& ss, const std::string& parent, int& counter, 
                      std::vector<EventHandler>& event_handlers,
                      std::vector<Binding>& bindings,
                      std::map<std::string, int>& component_counters,
                      const std::set<std::string>& method_names,
                      const std::string& parent_component_name,
                      bool in_loop = false,
                      std::vector<LoopRegion>* loop_regions = nullptr,
                      int* loop_counter = nullptr,
                      std::vector<IfRegion>* if_regions = nullptr,
                      int* if_counter = nullptr);
    void collect_dependencies(std::set<std::string>& deps) override;
};

// For each loop in view (for item in items)
struct ViewForEachStatement : ASTNode {
    std::string var_name;
    std::unique_ptr<Expression> iterable;
    std::unique_ptr<Expression> key_expr;
    std::vector<std::unique_ptr<ASTNode>> children;
    int loop_id = -1;

    std::string to_webcc() override { return ""; }
    void generate_code(std::stringstream& ss, const std::string& parent, int& counter, 
                      std::vector<EventHandler>& event_handlers,
                      std::vector<Binding>& bindings,
                      std::map<std::string, int>& component_counters,
                      const std::set<std::string>& method_names,
                      const std::string& parent_component_name,
                      bool in_loop = false,
                      std::vector<LoopRegion>* loop_regions = nullptr,
                      int* loop_counter = nullptr,
                      std::vector<IfRegion>* if_regions = nullptr,
                      int* if_counter = nullptr);
    void collect_dependencies(std::set<std::string>& deps) override;
};
