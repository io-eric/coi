#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <functional>
#include <filesystem>
#include <cstdlib>
#include <cstdio>
#include <queue>
#include <algorithm>

// =========================================================
// MAIN COMPILER
// =========================================================

void validate_mutability(const std::vector<Component>& components) {
    for (const auto& comp : components) {
        // Build set of mutable state variables
        std::set<std::string> mutable_vars;
        for (const auto& var : comp.state) {
            if (var->is_mutable) {
                mutable_vars.insert(var->name);
            }
        }
        for (const auto& prop : comp.props) {
            if (prop->is_mutable) {
                mutable_vars.insert(prop->name);
            }
        }

        // Check all methods for modifications to non-mutable variables
        for (const auto& method : comp.methods) {
            std::set<std::string> modified_vars;
            method.collect_modifications(modified_vars);
            
            for (const auto& var_name : modified_vars) {
                // Check if this variable exists in state and is not mutable
                bool is_known_var = false;
                bool is_mutable = false;
                bool is_prop = false;

                for (const auto& var : comp.state) {
                    if (var->name == var_name) {
                        is_known_var = true;
                        is_mutable = var->is_mutable;
                        break;
                    }
                }
                
                if (!is_known_var) {
                    for (const auto& prop : comp.props) {
                        if (prop->name == var_name) {
                            is_known_var = true;
                            is_prop = true;
                            is_mutable = prop->is_mutable;
                            break;
                        }
                    }
                }
                
                if (is_known_var && !is_mutable) {
                    if (is_prop) {
                        throw std::runtime_error("Cannot modify prop '" + var_name + "' in component '" + comp.name + 
                            "': prop is not mutable. Add 'mut' keyword to prop declaration: prop mut " + var_name);
                    } else {
                        throw std::runtime_error("Cannot modify '" + var_name + "' in component '" + comp.name + 
                            "': variable is not mutable. Add 'mut' keyword to make it mutable: mut " + var_name);
                    }
                }
            }
        }
    }
}

void validate_view_hierarchy(const std::vector<Component>& components) {
    std::map<std::string, const Component*> component_map;
    for (const auto& comp : components) {
        component_map[comp.name] = &comp;
    }

    std::function<void(ASTNode*, const std::string&)> validate_node = [&](ASTNode* node, const std::string& parent_comp_name) {
        if (!node) return;

        if (auto* comp_inst = dynamic_cast<ComponentInstantiation*>(node)) {
            auto it = component_map.find(comp_inst->component_name);
            if (it != component_map.end()) {
                if (it->second->render_roots.empty()) {
                     throw std::runtime_error("Component '" + comp_inst->component_name + "' is used in a view but has no view definition (logic-only component) at line " + std::to_string(comp_inst->line));
                }
                
                // Validate reference props
                const Component* target_comp = it->second;
                for (auto& passed_prop : comp_inst->props) {
                    // Find the prop declaration in the target component
                    for (const auto& declared_prop : target_comp->props) {
                        if (declared_prop->name == passed_prop.name) {
                            passed_prop.is_mutable_def = declared_prop->is_mutable;
                            if (declared_prop->is_reference && !passed_prop.is_reference) {
                                throw std::runtime_error(
                                    "Prop '" + passed_prop.name + "' in component '" + comp_inst->component_name + 
                                    "' expects a reference. Use '&" + passed_prop.name + "={...}' syntax at line " + 
                                    std::to_string(comp_inst->line));
                            }
                            if (!declared_prop->is_reference && passed_prop.is_reference) {
                                throw std::runtime_error(
                                    "Prop '" + passed_prop.name + "' in component '" + comp_inst->component_name + 
                                    "' does not expect a reference. Remove '&' prefix at line " + 
                                    std::to_string(comp_inst->line));
                            }
                            break;
                        }
                    }
                }
            }
        } else if (auto* el = dynamic_cast<HTMLElement*>(node)) {
            for (const auto& child : el->children) {
                validate_node(child.get(), parent_comp_name);
            }
        }
    };

    for (const auto& comp : components) {
        for (const auto& root : comp.render_roots) {
            validate_node(root.get(), comp.name);
        }
    }
}

// Collect child component names from a node
void collect_component_deps(ASTNode* node, std::set<std::string>& deps) {
    if (!node) return;
    if (auto* comp_inst = dynamic_cast<ComponentInstantiation*>(node)) {
        deps.insert(comp_inst->component_name);
    } else if (auto* el = dynamic_cast<HTMLElement*>(node)) {
        for (const auto& child : el->children) {
            collect_component_deps(child.get(), deps);
        }
    }
}

// Topologically sort components so dependencies come first
std::vector<Component*> topological_sort_components(std::vector<Component>& components) {
    std::map<std::string, Component*> comp_map;
    std::map<std::string, std::set<std::string>> dependencies;
    std::map<std::string, int> in_degree;
    
    for (auto& comp : components) {
        comp_map[comp.name] = &comp;
        in_degree[comp.name] = 0;
    }
    
    // Build dependency graph
    for (auto& comp : components) {
        std::set<std::string> deps;
        for (const auto& root : comp.render_roots) {
            collect_component_deps(root.get(), deps);
        }
        dependencies[comp.name] = deps;
    }
    
    // Calculate in-degrees
    for (auto& [name, deps] : dependencies) {
        for (auto& dep : deps) {
            if (comp_map.count(dep)) {
                in_degree[name]++;
            }
        }
    }
    
    // Kahn's algorithm
    std::queue<std::string> queue;
    for (auto& [name, degree] : in_degree) {
        if (degree == 0) {
            queue.push(name);
        }
    }
    
    std::vector<Component*> sorted;
    while (!queue.empty()) {
        std::string curr = queue.front();
        queue.pop();
        sorted.push_back(comp_map[curr]);
        
        // For each component that depends on curr, decrease in-degree
        for (auto& [name, deps] : dependencies) {
            if (deps.count(curr)) {
                in_degree[name]--;
                if (in_degree[name] == 0) {
                    queue.push(name);
                }
            }
        }
    }
    
    // Check for cycles
    if (sorted.size() != components.size()) {
        throw std::runtime_error("Circular dependency detected among components");
    }
    
    return sorted;
}

int main(int argc, char** argv){
    if(argc < 2){
        std::cerr << "Usage: " << argv[0] << " <input.coi> [--cc-only] [--keep-cc] [--out <dir> | -o <dir>]" << std::endl;
        return 1;
    }

    std::string input_file;
    std::string output_dir;
    bool cc_only = false;
    bool keep_cc = false;

    for(int i=1; i<argc; ++i) {
        std::string arg = argv[i];
        if(arg == "--cc-only") cc_only = true;
        else if(arg == "--keep-cc") keep_cc = true;
        else if(arg == "--out" || arg == "-o") {
            if(i+1 < argc) {
                output_dir = argv[++i];
            } else {
                std::cerr << "Error: --out requires an argument" << std::endl;
                return 1;
            }
        }
        else if(input_file.empty()) input_file = arg;
        else {
            std::cerr << "Unknown argument or multiple input files: " << arg << std::endl;
            return 1;
        }
    }

    if(input_file.empty()) {
        std::cerr << "No input file specified." << std::endl;
        return 1;
    }

    std::vector<Component> all_components;
    AppConfig final_app_config;
    std::set<std::string> processed_files;
    std::queue<std::string> file_queue;

    namespace fs = std::filesystem;
    try {
        file_queue.push(fs::canonical(input_file).string());
    } catch (const std::exception& e) {
        std::cerr << "Error resolving input file path: " << e.what() << std::endl;
        return 1;
    }

    try {
        while(!file_queue.empty()){
            std::string current_file_path = file_queue.front();
            file_queue.pop();

            if(processed_files.count(current_file_path)) continue;
            processed_files.insert(current_file_path);

            std::cerr << "Processing " << current_file_path << "..." << std::endl;

            std::ifstream file(current_file_path);
            if(!file){
                std::cerr << "Error: Could not open file " << current_file_path << std::endl;
                return 1;
            }
            std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

            // Lexical analysis
            Lexer lexer(source);
            auto tokens = lexer.tokenize();

            // Parsing
            Parser parser(tokens);
            parser.parse_file();
            
            all_components.insert(all_components.end(), std::make_move_iterator(parser.components.begin()), std::make_move_iterator(parser.components.end()));
            
            if(!parser.app_config.root_component.empty()){
                 final_app_config = parser.app_config;
            }
            
            fs::path current_path(current_file_path);
            fs::path parent_path = current_path.parent_path();

            for(const auto& import_path_str : parser.imports){
                fs::path import_path = parent_path / import_path_str;
                try {
                    std::string abs_path = fs::canonical(import_path).string();
                    if(processed_files.find(abs_path) == processed_files.end()){
                        file_queue.push(abs_path);
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error resolving import path " << import_path_str << ": " << e.what() << std::endl;
                    return 1;
                }
            }
        }

        std::cerr << "All files processed. Total components: " << all_components.size() << std::endl;

        validate_view_hierarchy(all_components);
        validate_mutability(all_components);

        // Determine output filename
        namespace fs = std::filesystem;
        fs::path input_path(input_file);
        fs::path output_path;

        if(!output_dir.empty()) {
            fs::path out_dir_path(output_dir);
            try {
                fs::create_directories(out_dir_path);
            } catch(const fs::filesystem_error& e) {
                 std::cerr << "Error: Could not create output directory " << output_dir << ": " << e.what() << std::endl;
                 return 1;
            }
            
            output_path = out_dir_path / input_path.stem();
            output_path += ".cc";
        } else {
            output_path = input_path;
            output_path.replace_extension(".cc");
        }

        std::string output_cc = output_path.string();

        std::ofstream out(output_cc);
        if(!out) {
             std::cerr << "Error: Could not open output file " << output_cc << std::endl;
             return 1;
        }

        // Code generation
        // This should in best case be automated based on what is used in the coi source files
        out << "#include \"webcc/canvas.h\"\n";
        out << "#include \"webcc/dom.h\"\n";
        out << "#include \"webcc/system.h\"\n";
        out << "#include \"webcc/input.h\"\n";
        out << "#include \"webcc/core/function.h\"\n";
        out << "#include \"webcc/core/allocator.h\"\n";
        out << "#include \"webcc/core/new.h\"\n\n";

        out << "struct Listener {\n";
        out << "    int32_t handle;\n";
        out << "    webcc::function<void()> callback;\n";
        out << "};\n\n";

        out << "struct EventDispatcher {\n";
        out << "    static constexpr int MAX_LISTENERS = 128;\n";
        out << "    Listener listeners[MAX_LISTENERS];\n";
        out << "    int count = 0;\n";
        out << "    void register_click(webcc::handle h, webcc::function<void()> cb) {\n";
        out << "        if (count < MAX_LISTENERS) {\n";
        out << "            listeners[count].handle = (int32_t)h;\n";
        out << "            listeners[count].callback = cb;\n";
        out << "            count++;\n";
        out << "        }\n";
        out << "    }\n";
        out << "    void dispatch(const webcc::Event* events, uint32_t event_count) {\n";
        out << "        for(uint32_t i=0; i<event_count; ++i) {\n";
        out << "            const auto& e = events[i];\n";
        out << "            if (e.opcode == webcc::dom::ClickEvent::OPCODE) {\n";
        out << "                auto click = e.as<webcc::dom::ClickEvent>();\n";
        out << "                if (click) {\n";
        out << "                    for(int j=0; j<count; ++j) {\n";
        out << "                        if (listeners[j].handle == (int32_t)click->handle) {\n";
        out << "                            listeners[j].callback();\n";
        out << "                        }\n";
        out << "                    }\n";
        out << "                }\n";
        out << "            }\n";
        out << "        }\n";
        out << "    }\n";
        out << "};\n";
        out << "EventDispatcher g_dispatcher;\n\n";

        // Sort components topologically so dependencies come first
        auto sorted_components = topological_sort_components(all_components);

        // Forward declarations
        for(auto* comp : sorted_components) {
            out << "class " << comp->name << ";\n";
        }
        out << "\n";

        for(auto* comp : sorted_components) {
            out << comp->to_webcc();
        }

        if(final_app_config.root_component.empty()) {
             std::cerr << "Error: No root component defined. Use 'app { root = ComponentName }' to define the entry point." << std::endl;
             return 1;
        }

        out << "\n" << final_app_config.root_component << "* app = nullptr;\n";
        out << "void update_wrapper(float time) {\n";
            out << "    static float last_time = 0;\n";
            out << "    float dt = (time - last_time) / 1000.0f;\n";
            out << "    last_time = time;\n";
            out << "    if (dt > 0.1f) dt = 0.1f; // Cap dt to avoid huge jumps\n";
            out << "    static webcc::Event events[64];\n";
            out << "    uint32_t count = 0;\n";
            out << "    webcc::Event e;\n";
            out << "    while (webcc::poll_event(e) && count < 64) {\n";
            out << "        events[count++] = e;\n";
            out << "    }\n";
            out << "    g_dispatcher.dispatch(events, count);\n";
            out << "    if (app) app->tick(dt);\n";
            out << "    webcc::flush();\n";
            out << "}\n\n";

            out << "int main() {\n";
            out << "    // We allocate the app on the heap because the stack is destroyed when main() returns.\n";
            out << "    // The app needs to persist for the event loop (update_wrapper).\n";
            out << "    // We use webcc::malloc to ensure memory is tracked by the framework.\n";
            out << "    void* app_mem = webcc::malloc(sizeof(" << final_app_config.root_component << "));\n";
            out << "    app = new (app_mem) " << final_app_config.root_component << "();\n";
            
            // Inject CSS
            std::string all_css;
            for(const auto& comp : all_components) {
                if(!comp.global_css.empty()) {
                    all_css += comp.global_css + "\\n";
                }
                if(!comp.css.empty()) {
                    // Simple CSS scoping: prefix selectors with [coi-scope="ComponentName"]
                    std::string scoped_css;
                    std::string raw = comp.css;
                    size_t pos = 0;
                    while (pos < raw.length()) {
                        size_t brace = raw.find('{', pos);
                        if (brace == std::string::npos) {
                            scoped_css += raw.substr(pos);
                            break;
                        }
                        
                        std::string selector_group = raw.substr(pos, brace - pos);
                        std::stringstream ss_sel(selector_group);
                        std::string selector;
                        bool first = true;
                        while (std::getline(ss_sel, selector, ',')) {
                            if (!first) scoped_css += ",";
                            size_t start = selector.find_first_not_of(" \t\n\r");
                            size_t end = selector.find_last_not_of(" \t\n\r");
                            if (start != std::string::npos) {
                                std::string trimmed = selector.substr(start, end - start + 1);
                                size_t colon = trimmed.find(':');
                                if (colon != std::string::npos) {
                                    scoped_css += trimmed.substr(0, colon) + "[coi-scope=\"" + comp.name + "\"]" + trimmed.substr(colon);
                                } else {
                                    scoped_css += trimmed + "[coi-scope=\"" + comp.name + "\"]";
                                }
                            }
                            first = false;
                        }
                        
                        size_t end_brace = raw.find('}', brace);
                        if (end_brace == std::string::npos) {
                            scoped_css += raw.substr(brace);
                            break;
                        }
                        scoped_css += raw.substr(brace, end_brace - brace + 1);
                        pos = end_brace + 1;
                    }
                    all_css += scoped_css + "\\n";
                }
            }
            if(!all_css.empty()) {
                // Escape quotes in CSS string for C++ string literal
                std::string escaped_css;
                for(char c : all_css) {
                    if(c == '"') escaped_css += "\\\"";
                    else if(c == '\n') escaped_css += "\\n";
                    else escaped_css += c;
                }
                
                out << "    // Inject CSS\n";
                out << "    webcc::handle style_el = webcc::dom::create_element(\"style\");\n";
                out << "    webcc::dom::set_inner_text(style_el, \"" << escaped_css << "\");\n";
                out << "    webcc::dom::append_child(webcc::dom::get_body(), style_el);\n";
            }

            out << "    app->view();\n";
            out << "    webcc::system::set_main_loop(update_wrapper);\n";
            out << "    webcc::flush();\n";
            out << "    return 0;\n";
            out << "}\n";
        
        out.close();
        std::cerr << "Generated " << output_cc << std::endl;

        if (!cc_only) {
            namespace fs = std::filesystem;
            fs::path cwd = fs::current_path();
            fs::path abs_output_cc = fs::absolute(output_cc);
            fs::path abs_output_dir = output_dir.empty() ? cwd : fs::absolute(output_dir);

            fs::create_directories("build/.webcc_cache");
            std::string cmd = "webcc " + abs_output_cc.string();
            cmd += " --out " + abs_output_dir.string();
            cmd += " --cache-dir build/.webcc_cache";

            std::cerr << "Running: " << cmd << std::endl;
            int ret = system(cmd.c_str());
            if (ret != 0) {
                std::cerr << "Error: webcc compilation failed." << std::endl;
                return 1;
            }
            
            if (!keep_cc) {
                fs::remove(output_cc);
            }
        }

    } catch(const std::exception& e){
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
