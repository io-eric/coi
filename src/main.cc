#include <endian.h>
#include <exception>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <sstream>
#include <fstream>
#include <cctype>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <cstdio>
#include <filesystem>

#include <set>

// =========================================================
// LEXER
// =========================================================

enum class TokenType {
    // Keywords
    COMPONENT, DEF, RETURN, STRUCT, VIEW, IF, ELSE, FOR, WHILE, TICK, PROP, STYLE,
    // Types
    INT, FLOAT, STRING, BOOL, VOID,
    // Literals
    INT_LITERAL, FLOAT_LITERAL, STRING_LITERAL, TRUE, FALSE,
    // Identifiers
    IDENTIFIER,
    // Operatos
    PLUS, MINUS, STAR, SLASH, PERCENT, ASSIGN, EQ, NEQ, LT, GT, LTE, GTE,
    PLUS_ASSIGN, MINUS_ASSIGN, PLUS_PLUS, MINUS_MINUS,
    // Delimiters
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET, LANGLE, RANGLE,
    SEMICOLON, COMMA, DOT, COLON, ARROW,
    // Special
    END_OF_FILE, UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
};

class Lexer {
    private:
        std::string source;
        size_t pos = 0;
        int line = 1;
        int column = 1;

        char current(){
            return pos < source.size() ? source[pos] : '\0';
        }

        char peek(int offset = 1){
            return (pos + offset) < source.size() ? source[pos + offset] : '\0';
        }

        void advance(){
            if(current() == '\n'){
                line++;
                column = 1;
            }else{
                column++;
            }
            pos++;
        }

        void skip_whitespace(){
            while(std::isspace(current())) advance();
        }

        void skip_comment(){
            if(current() == '/' && peek() == '/'){
                while(current() != '\n' && current() != '\0') advance();
            }
        }

        Token make_token(TokenType type, const  std::string& value = ""){
            return Token{type, value, line, column};
        }

        Token read_number(){
            int start_line = line;
            int start_column = column;
            std::string num;
            bool is_float = false;

            while(std::isdigit(current()) || current() == '.'){
                if(current() == '.'){
                    if(is_float) break;
                    is_float = true;
                }
                num += current();
                advance();
            }

            return Token{is_float ? TokenType::FLOAT_LITERAL : TokenType::INT_LITERAL, num, start_line, start_column};
        }

        Token read_string(){
            int start_line = line;
            int start_column = column;
            std::string str;
            advance(); // skip opening quote

            while(current() != '"' && current() != '\0'){
                if(current() == '\\'){
                    advance();
                    switch (current()) {
                        case 'n' : str += '\n'; break;
                        case 't' : str += '\t'; break;
                        case '\\' : str += '\\'; break;
                        case '"' : str += '"'; break;
                        case '{' : str += "\\{"; break;
                        case '}' : str += "\\}"; break;
                        default: str += current();
                    }
                }else{
                    str += current();
                }
                advance();
            }

            advance(); // skip closing quote
            return Token{TokenType::STRING_LITERAL, str, start_line, start_column};
        }

        Token read_identifier(){
            int start_line = line;
            int start_column = column;
            std::string id;

            while(std::isalnum(current()) || current() == '_'){
                id += current();
                advance();
            }

            // Check for keywords
            struct std::map<std::string, TokenType> keywords = {
                {"component", TokenType::COMPONENT},
                {"def", TokenType::DEF},
                {"return", TokenType::RETURN},
                {"struct", TokenType::STRUCT},
                {"view", TokenType::VIEW},
                {"tick", TokenType::TICK},
                {"prop", TokenType::PROP},
                {"style", TokenType::STYLE},
                {"if", TokenType::IF},
                {"else", TokenType::ELSE},
                {"for", TokenType::FOR},
                {"while", TokenType::WHILE},
                {"int", TokenType::INT},
                {"float", TokenType::FLOAT},
                {"string", TokenType::STRING},
                {"bool", TokenType::BOOL},
                {"void", TokenType::VOID},
                {"true", TokenType::TRUE},
                {"false", TokenType::FALSE},
            };

            auto it = keywords.find(id);
            if(it != keywords.end()){
                return Token{it->second, id, start_line, start_column};
            }

            return Token{TokenType::IDENTIFIER, id, start_line, start_column};
        }

    public:
        Lexer(const std::string& src) : source(src){}

        std::vector<Token> tokenize(){
            std::vector<Token> tokens;

            while(current() != '\0'){
                // Skip whitespace and comments
                while(std::isspace(current()) || (current() == '/' && peek() == '/')){
                    if(std::isspace(current())){
                        skip_whitespace();
                    } else {
                        skip_comment();
                    }
                }

                if(current() == '\0') break;

                // Numbers
                if(std::isdigit(current())){
                    tokens.push_back(read_number());
                    continue;
                }

                // Strings
                if(current() == '"'){
                    tokens.push_back(read_string());
                    continue;
                }

                // Identifiers and keywords
                if(std::isalpha(current()) || current() == '_'){
                    tokens.push_back(read_identifier());
                    continue;
                }

                // Two-character operators
                if(current() == '=' && peek() == '='){
                    tokens.push_back(make_token(TokenType::EQ, "=="));
                    advance(); advance();
                    continue;
                }
                if (current() == '!' && peek() == '=') {
                    tokens.push_back(make_token(TokenType::NEQ, "!="));
                    advance(); advance();
                    continue;
                }
                if (current() == '<' && peek() == '=') {
                    tokens.push_back(make_token(TokenType::LTE, "<="));
                    advance(); advance();
                    continue;
                }
                if (current() == '>' && peek() == '=') {
                    tokens.push_back(make_token(TokenType::GTE, ">="));
                    advance(); advance();
                    continue;
                }
                if (current() == '+' && peek() == '=') {
                    tokens.push_back(make_token(TokenType::PLUS_ASSIGN, "+="));
                    advance(); advance();
                    continue;
                }
                if (current() == '-' && peek() == '=') {
                    tokens.push_back(make_token(TokenType::MINUS_ASSIGN, "-="));
                    advance(); advance();
                    continue;
                }
                if (current() == '+' && peek() == '+') {
                    tokens.push_back(make_token(TokenType::PLUS_PLUS, "++"));
                    advance(); advance();
                    continue;
                }
                if (current() == '-' && peek() == '-') {
                    tokens.push_back(make_token(TokenType::MINUS_MINUS, "--"));
                    advance(); advance();
                    continue;
                }
                if (current() == '=' && peek() == '>') {
                    tokens.push_back(make_token(TokenType::ARROW, "=>"));
                    advance(); advance();
                    continue;
                }

                // Single-character tokens
                switch (current()) {
                    case '+': tokens.push_back(make_token(TokenType::PLUS, "+")); break;
                    case '-': tokens.push_back(make_token(TokenType::MINUS, "-")); break;
                    case '*': tokens.push_back(make_token(TokenType::STAR, "*")); break;
                    case '/': tokens.push_back(make_token(TokenType::SLASH, "/")); break;
                    case '%': tokens.push_back(make_token(TokenType::PERCENT, "%")); break;
                    case '=': tokens.push_back(make_token(TokenType::ASSIGN, "=")); break;
                    case '<': tokens.push_back(make_token(TokenType::LT, "<")); break;
                    case '>': tokens.push_back(make_token(TokenType::GT, ">")); break;
                    case '(': tokens.push_back(make_token(TokenType::LPAREN, "(")); break;
                    case ')': tokens.push_back(make_token(TokenType::RPAREN, ")")); break;
                    case '{': tokens.push_back(make_token(TokenType::LBRACE, "{")); break;
                    case '}': tokens.push_back(make_token(TokenType::RBRACE, "}")); break;
                    case '[': tokens.push_back(make_token(TokenType::LBRACKET, "[")); break;
                    case ']': tokens.push_back(make_token(TokenType::RBRACKET, "]")); break;
                    case ';': tokens.push_back(make_token(TokenType::SEMICOLON, ";")); break;
                    case ',': tokens.push_back(make_token(TokenType::COMMA, ",")); break;
                    case '.': tokens.push_back(make_token(TokenType::DOT, ".")); break;
                    case ':': tokens.push_back(make_token(TokenType::COLON, ":")); break;
                    default:
                        tokens.push_back(make_token(TokenType::UNKNOWN, std::string(1, current())));
                }
                advance();
            }

            tokens.push_back(make_token(TokenType::END_OF_FILE, ""));
            return tokens;
    }
};

// =========================================================
// AST NODES
// =========================================================

std::string convert_type(const std::string& type) {
    if (type == "string") return "webcc::string";
    if (type.length() > 2 && type.substr(type.length() - 2) == "[]") {
        std::string inner = type.substr(0, type.length() - 2);
        return "SimpleVector<" + convert_type(inner) + ">";
    }
    return type;
}

struct ASTNode {
    virtual ~ASTNode() = default;
    virtual std::string to_webcc() = 0;
    virtual void collect_dependencies(std::set<std::string>& deps) {}
    int line = 0;
};

struct Expression : ASTNode {
    virtual bool is_static() { return false; }
};

struct Statement : ASTNode{};

struct IntLiteral : Expression {
    int value;
    IntLiteral(int v) : value(v){}
    std::string to_webcc() override {return std::to_string(value);}
    bool is_static() override { return true; }
};

struct FloatLiteral : Expression {
    float value;
    FloatLiteral(float v) : value(v){}
    std::string to_webcc() override {
        std::string s = std::to_string(value);
        if(s.find('.') != std::string::npos){
            s = s.substr(0, s.find_last_not_of('0')+1);
            if(s.back() == '.') s += "0";
        }
        return s + "f";
    }
    bool is_static() override { return true; }
};

struct StringLiteral : Expression {
    std::string value;
    StringLiteral(const std::string& v) : value(v){}
    
    struct Part {
        bool is_expr;
        std::string content;
    };

    std::vector<Part> parse() {
        std::vector<Part> parts;
        std::string current;
        for(size_t i=0; i<value.length(); ++i) {
            if(value[i] == '\\' && i + 1 < value.length() && (value[i+1] == '{' || value[i+1] == '}')) {
                current += value[i+1];
                i++;
            } else if(value[i] == '{') {
                if(!current.empty()) parts.push_back({false, current});
                current = "";
                i++;
                while(i < value.length() && value[i] != '}') {
                    current += value[i];
                    i++;
                }
                parts.push_back({true, current});
                current = "";
            } else {
                current += value[i];
            }
        }
        if(!current.empty()) parts.push_back({false, current});
        return parts;
    }

    std::string to_webcc() override {
        auto parts = parse();
        if(parts.empty()) return "\"\"";
        bool has_expr = false;
        for(auto& p : parts) if(p.is_expr) has_expr = true;
        
        if(!has_expr) {
            std::string content;
            for(auto& p : parts) content += p.content;
            
            std::string escaped;
            for(char c : content) {
                if(c == '"') escaped += "\\\"";
                else if(c == '\\') escaped += "\\\\";
                else if(c == '\n') escaped += "\\n";
                else if(c == '\t') escaped += "\\t";
                else escaped += c;
            }
            return "\"" + escaped + "\"";
        }

        std::string code = "webcc::string::concat(";
        for(size_t i=0; i<parts.size(); ++i) {
            if(i > 0) code += ", ";
            if(parts[i].is_expr) {
                code += parts[i].content;
            } else {
                std::string escaped;
                for(char c : parts[i].content) {
                    if(c == '"') escaped += "\\\"";
                    else if(c == '\\') escaped += "\\\\";
                    else if(c == '\n') escaped += "\\n";
                    else if(c == '\t') escaped += "\\t";
                    else escaped += c;
                }
                code += "\"" + escaped + "\"";
            }
        }
        code += ")";
        return code;
    }

    bool is_static() override {
        auto parts = parse();
        for(auto& p : parts) if(p.is_expr) return false;
        return true;
    }
    
    void collect_dependencies(std::set<std::string>& deps) override {
        auto parts = parse();
        for(auto& p : parts) {
            if(p.is_expr) {
                std::string expr = p.content;
                std::string id;
                for(char c : expr) {
                    if(isalnum(c) || c == '_') {
                        id += c;
                    } else {
                        if(!id.empty()) {
                            if(!isdigit(id[0])) deps.insert(id);
                            id = "";
                        }
                    }
                }
                if(!id.empty()) {
                    if(!isdigit(id[0])) deps.insert(id);
                }
            }
        }
    }
};

struct Identifier : Expression {
    std::string name;
    Identifier(const  std::string& n) : name(n) {}
    std::string to_webcc() override {return name;}
    void collect_dependencies(std::set<std::string>& deps) override {
        deps.insert(name);
    }
};


struct BinaryOp : Expression {
    std::unique_ptr<Expression> left;
    std::string op;
    std::unique_ptr<Expression> right;

    BinaryOp(std::unique_ptr<Expression> l, const std::string& o, std::unique_ptr<Expression> r)
        : left(std::move(l)), op(o), right(std::move(r)){}

    std::string to_webcc() override {
        return "(" + left->to_webcc() + " " + op + " " + right->to_webcc() + ")";
    }
    void collect_dependencies(std::set<std::string>& deps) override {
        left->collect_dependencies(deps);
        right->collect_dependencies(deps);
    }
};

struct FunctionCall : Expression {
    std::string name;
    std::vector<std::unique_ptr<Expression>> args;

    FunctionCall(const std::string& n) : name(n){}

    std::string args_to_string() {
        if (args.empty()) return "\"\"";
        
        std::string result = "webcc::string::concat(";
        for(size_t i = 0; i < args.size(); i++){
            if(i > 0) result += ", ";
            result += args[i]->to_webcc();
        }
        result += ")";
        return result;
    }

    std::string to_webcc() override {
        if (name == "log" || name == "log.info" || name == "log.warn" || name == "log.error" || name == "log.debug" || name == "log.event") {
            std::vector<Expression*> parts;
            std::function<void(Expression*)> flatten = [&](Expression* e) {
                if (auto bin = dynamic_cast<BinaryOp*>(e)) {
                    if (bin->op == "+") {
                        flatten(bin->left.get());
                        flatten(bin->right.get());
                        return;
                    }
                }
                parts.push_back(e);
            };

            for(auto& arg : args) {
                flatten(arg.get());
            }

            std::string code = "{ webcc::formatter<256> _fmt; ";
            for(auto* p : parts) {
                code += "_fmt << (" + p->to_webcc() + "); ";
            }

            if (name == "log" || name == "log.info") {
                code += "webcc::system::log(_fmt.c_str()); }";
            } else if (name == "log.warn") {
                code += "webcc::system::warn(_fmt.c_str()); }";
            } else if (name == "log.error") {
                code += "webcc::system::error(_fmt.c_str()); }";
            } else if (name == "log.debug") {
                code += "webcc::system::log(webcc::string::concat(\"[DEBUG] \", _fmt.c_str())); }";
            } else if (name == "log.event") {
                code += "webcc::system::log(webcc::string::concat(\"[EVENT] \", _fmt.c_str())); }";
            }
            return code;
        }

        std::string result = name + "(";
        for(size_t i = 0; i < args.size(); i++){
            if(i > 0) result += ", ";
            result += args[i]->to_webcc();
        }
        result += ")";
        return result;
    }
    void collect_dependencies(std::set<std::string>& deps) override {
        for(auto& arg : args) arg->collect_dependencies(deps);
    }
};

struct MemberAccess : Expression {
    std::unique_ptr<Expression> object;
    std::string member;

    MemberAccess(std::unique_ptr<Expression> obj, const std::string& mem)
        : object(std::move(obj)), member(mem) {}

    std::string to_webcc() override {
        return object->to_webcc() + "." + member;
    }
    void collect_dependencies(std::set<std::string>& deps) override {
        object->collect_dependencies(deps);
    }
};

struct PostfixOp : Expression {
    std::unique_ptr<Expression> operand;
    std::string op;

    PostfixOp(std::unique_ptr<Expression> expr, const std::string& o)
        : operand(std::move(expr)), op(o) {}

    std::string to_webcc() override {
        return operand->to_webcc() + op;
    }
    void collect_dependencies(std::set<std::string>& deps) override {
        operand->collect_dependencies(deps);
    }
    bool is_static() override { return false; }
};

struct UnaryOp : Expression {
    std::string op;
    std::unique_ptr<Expression> operand;

    UnaryOp(const std::string& o, std::unique_ptr<Expression> expr)
        : op(o), operand(std::move(expr)) {}

    std::string to_webcc() override {
        return op + operand->to_webcc();
    }
    void collect_dependencies(std::set<std::string>& deps) override {
        operand->collect_dependencies(deps);
    }
    bool is_static() override { return operand->is_static(); }
};

struct VarDeclaration : Statement {
    std::string type;
    std::string name;
    std::unique_ptr<Expression> initializer;

    std::string to_webcc() override {
        std::string result = convert_type(type) + " " + name;
        if(initializer) {
            result += " = " + initializer->to_webcc();
        }
        result += ";";
        return result;
    }
};

struct PropDeclaration : Statement {
    std::string type;
    std::string name;
    std::unique_ptr<Expression> default_value;

    std::string to_webcc() override {
        return "";
    }
};

struct Assignment : Statement {
    std::string name;
    std::unique_ptr<Expression> value;

    std::string to_webcc() override {
        return name + " = " + value->to_webcc() + ";";
    }
    void collect_dependencies(std::set<std::string>& deps) override {
        // For assignment, we are modifying 'name', not reading it (unless it's compound assignment, but here it's simple)
        // But we might read from 'value'
        value->collect_dependencies(deps);
    }
};

struct ReturnStatement : Statement {
    std::unique_ptr<Expression> value;
    std::string to_webcc() override {
        return "return " + value->to_webcc() + ";";
    }
    void collect_dependencies(std::set<std::string>& deps) override {
        value->collect_dependencies(deps);
    }
};

struct ExpressionStatement : Statement {
    std::unique_ptr<Expression> expression;
    std::string to_webcc() override {
        return expression->to_webcc() + ";\n";
    }
    void collect_dependencies(std::set<std::string>& deps) override {
        expression->collect_dependencies(deps);
    }
};

struct BlockStatement : Statement {
    std::vector<std::unique_ptr<Statement>> statements;
    
    std::string to_webcc() override {
        std::string code = "{\n";
        for(auto& stmt : statements) code += stmt->to_webcc();
        code += "}\n";
        return code;
    }
    
    void collect_dependencies(std::set<std::string>& deps) override {
        for(auto& stmt : statements) stmt->collect_dependencies(deps);
    }
};

struct IfStatement : Statement {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> then_branch;
    std::unique_ptr<Statement> else_branch;

    std::string to_webcc() override {
        std::string code = "if(" + condition->to_webcc() + ") ";
        code += then_branch->to_webcc();
        if(else_branch){
            code += " else ";
            code += else_branch->to_webcc();
        }
        return code;
    }
    
    void collect_dependencies(std::set<std::string>& deps) override {
        condition->collect_dependencies(deps);
        then_branch->collect_dependencies(deps);
        if(else_branch) else_branch->collect_dependencies(deps);
    }
};

void collect_mods_recursive(Statement* stmt, std::set<std::string>& mods) {
    if(auto assign = dynamic_cast<Assignment*>(stmt)) {
        mods.insert(assign->name);
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
}

struct FunctionDef {
    std::string name;
    std::string return_type;
    std::vector<std::pair<std::string, std::string>> params;
    std::vector<std::unique_ptr<Statement>> body;

    std::string to_webcc(const std::string& injected_code = ""){
        std::string result = convert_type(return_type) + " " + name + "(";
        for(size_t i = 0; i < params.size(); i++){
            if(i > 0) result += ", ";
            result += convert_type(params[i].first) + " " + params[i].second;
        }
        result += ") {\n";
        for(auto& stmt : body){
            result += "    " + stmt->to_webcc() + "\n";
        }
        if(!injected_code.empty()) {
            result += injected_code;
        }
        result += "}\n";
        return result;
    }
    
    void collect_modifications(std::set<std::string>& mods) {
        for(auto& stmt : body) {
            collect_mods_recursive(stmt.get(), mods);
        }
    }
};

struct StructField {
    std::string type;
    std::string name;
};

struct StructDef : ASTNode {
    std::string name;
    std::vector<StructField> fields;

    std::string to_webcc() override {
        std::stringstream ss;
        ss << "struct " << name << " {\n";
        for(const auto& field : fields){
            ss << "    " << convert_type(field.type) << " " << field.name << ";\n";
        }
        // Constructor
        ss << "    " << name << "(";
        for(size_t i = 0; i < fields.size(); i++){
            if(i > 0) ss << ", ";
            ss << convert_type(fields[i].type) << " " << fields[i].name;
        }
        ss << ") : ";
        for(size_t i = 0; i < fields.size(); i++){
            if(i > 0) ss << ", ";
            ss << fields[i].name << "(" << fields[i].name << ")";
        }
        ss << " {}\n";
        ss << "    " << name << "() {}\n"; // Default constructor
        ss << "};\n";
        return ss.str();
    }
};

struct TextNode : ASTNode {
    std::string text;
    TextNode(const std::string& t) : text(t) {}
    std::string to_webcc() override { return "\"" + text + "\""; }
};

struct HTMLAttribute {
    std::string name;
    std::unique_ptr<Expression> value;
};

struct Binding {
    int element_id;
    std::string type; // "attr" or "text"
    std::string name; // attribute name
    std::string value_code;
    std::set<std::string> dependencies;
    Expression* expr = nullptr;
};

struct ComponentInstantiation : ASTNode {
    std::string component_name;
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> props;
    
    std::string to_webcc() override { return ""; }

    void generate_code(std::stringstream& ss, const std::string& parent, int& counter, 
                      std::vector<std::pair<int, std::string>>& click_handlers,
                      std::vector<Binding>& bindings,
                      std::map<std::string, int>& component_counters,
                      const std::set<std::string>& method_names,
                      const std::string& parent_component_name) {
        
        int id = component_counters[component_name]++;
        std::string instance_name = component_name + "_" + std::to_string(id);
        
        // Set props
        for(auto& prop : props) {
            std::string val = prop.second->to_webcc();
            // Check if val is a method name
            if(method_names.count(val)) {
                // Wrap in function
                ss << "        " << instance_name << "." << prop.first << " = [this]() { this->" << val << "(); };\n";
            } else {
                ss << "        " << instance_name << "." << prop.first << " = " << val << ";\n";
            }
        }
        
        // Call view
        if(!parent.empty()) {
            ss << "        " << instance_name << ".view(" << parent << ");\n";
        } else {
            ss << "        " << instance_name << ".view();\n";
        }
    }
    
    void collect_dependencies(std::set<std::string>& deps) override {
        for(auto& prop : props) {
            prop.second->collect_dependencies(deps);
        }
    }
};

struct HTMLElement : ASTNode {
    std::string tag;
    std::vector<HTMLAttribute> attributes;
    std::vector<std::unique_ptr<ASTNode>> children;

    std::string to_webcc() override { return ""; }

    void generate_code(std::stringstream& ss, const std::string& parent, int& counter, 
                      std::vector<std::pair<int, std::string>>& click_handlers,
                      std::vector<Binding>& bindings,
                      std::map<std::string, int>& component_counters,
                      const std::set<std::string>& method_names,
                      const std::string& parent_component_name) {
        int my_id = counter++;
        std::string var = "el_" + std::to_string(my_id);
        
        // Creation block
        ss << "        " << var << " = webcc::dom::create_element(\"" << tag << "\");\n";
        ss << "        webcc::dom::set_attribute(" << var << ", \"coi-scope\", \"" << parent_component_name << "\");\n";
        
        // Attributes
        for(auto& attr : attributes){
            if(attr.name == "onclick"){
                 ss << "        webcc::dom::add_click_listener(" << var << ");\n";
                 // Store handler for later generation
                 click_handlers.push_back({my_id, attr.value->to_webcc()});
            } else {
                 std::string val = attr.value->to_webcc();
                 ss << "        webcc::dom::set_attribute(" << var << ", \"" << attr.name << "\", " << val << ");\n";
                 
                 if(!attr.value->is_static()) {
                     Binding b;
                     b.element_id = my_id;
                     b.type = "attr";
                     b.name = attr.name;
                     b.value_code = val;
                     b.expr = attr.value.get();
                     attr.value->collect_dependencies(b.dependencies);
                     bindings.push_back(b);
                 }
            }
        }

        // Append to parent
        if(!parent.empty()){
            ss << "        webcc::dom::append_child(" << parent << ", " << var << ");\n";
        }

        // Children
        bool has_elements = false;
        for(auto& child : children) {
            if(dynamic_cast<HTMLElement*>(child.get()) || dynamic_cast<ComponentInstantiation*>(child.get())) has_elements = true;
        }

        if(has_elements){
             for(auto& child : children){
                 if(auto el = dynamic_cast<HTMLElement*>(child.get())){
                     el->generate_code(ss, var, counter, click_handlers, bindings, component_counters, method_names, parent_component_name);
                 } else if(auto comp = dynamic_cast<ComponentInstantiation*>(child.get())){
                     comp->generate_code(ss, var, counter, click_handlers, bindings, component_counters, method_names, parent_component_name);
                 }
             }
        } else {
             // Text content
             std::string code;
             bool all_static = true;
             
             if (children.size() == 1) {
                 code = children[0]->to_webcc();
                 if (!(code.size() >= 2 && code.front() == '"' && code.back() == '"')) {
                     all_static = false;
                     code = "webcc::string::concat(" + code + ")";
                 }
             } else if (children.size() > 1) {
                 std::string args;
                 bool first = true;
                 for(auto& child : children){
                     if(!first) args += ", ";
                     std::string c = child->to_webcc();
                     args += c;
                     if (!(c.size() >= 2 && c.front() == '"' && c.back() == '"')) all_static = false;
                     first = false;
                 }
                 code = "webcc::string::concat(" + args + ")";
             }

             if(!code.empty()) {
                 ss << "        webcc::dom::set_inner_text(" << var << ", " << code << ");\n";
                 
                 if(!all_static) {
                     Binding b;
                     b.element_id = my_id;
                     b.type = "text";
                     b.value_code = code;
                     for(auto& child : children) child->collect_dependencies(b.dependencies);
                     bindings.push_back(b);
                 }
             }
        }
    }
    void collect_dependencies(std::set<std::string>& deps) override {
        for(auto& attr : attributes) {
            if(attr.value) attr.value->collect_dependencies(deps);
        }
        for(auto& child : children) {
            child->collect_dependencies(deps);
        }
    }
};



struct Component : ASTNode {
    std::string name;
    std::string css;
    std::string global_css;
    std::vector<std::unique_ptr<StructDef>> structs;
    std::vector<std::unique_ptr<VarDeclaration>> state;
    std::vector<std::unique_ptr<PropDeclaration>> props;
    std::vector<FunctionDef> methods;
    std::unique_ptr<ASTNode> render_root;

    // Helper to collect child components
    void collect_child_components(ASTNode* node, std::map<std::string, int>& counts) {
        if(auto comp = dynamic_cast<ComponentInstantiation*>(node)) {
            counts[comp->component_name]++;
        }
        if(auto el = dynamic_cast<HTMLElement*>(node)) {
            for(auto& child : el->children) {
                collect_child_components(child.get(), counts);
            }
        }
    }

    std::string to_webcc() override {
        std::stringstream ss;
        std::vector<std::pair<int, std::string>> click_handlers;
        std::vector<Binding> bindings;
        std::map<std::string, int> component_counters; // For generating code
        std::map<std::string, int> component_members; // For declaring members
        int element_count = 0;
        
        // Collect child components to declare members
        if(render_root) {
            collect_child_components(render_root.get(), component_members);
        }

        // Collect method names
        std::set<std::string> method_names;
        for(auto& m : methods) method_names.insert(m.name);

        std::stringstream ss_render;
        if(render_root){
            if(auto el = dynamic_cast<HTMLElement*>(render_root.get())){
                // Auto-bind onclick if not present
                bool has_onclick_attr = false;
                for(auto& attr : el->attributes) if(attr.name == "onclick") has_onclick_attr = true;
                
                if(!has_onclick_attr) {
                    HTMLAttribute attr;
                    attr.name = "onclick";
                    attr.value = std::make_unique<Identifier>("onclick");
                    el->attributes.push_back(std::move(attr));
                }

                el->generate_code(ss_render, "parent", element_count, click_handlers, bindings, component_counters, method_names, name);
            } else if(auto comp = dynamic_cast<ComponentInstantiation*>(render_root.get())){
                comp->generate_code(ss_render, "parent", element_count, click_handlers, bindings, component_counters, method_names, name);
            }
        }

        // Generate component as a class
        ss << "class " << name << " {\n";
        ss << "public:\n";

        // Structs
        for(auto& s : structs){
            ss << s->to_webcc() << "\n";
        }
        
        // Props
        for(auto& prop : props){
             ss << "    " << convert_type(prop->type) << " " << prop->name;
             if(prop->default_value){
                 ss << " = " << prop->default_value->to_webcc();
             }
             ss << ";\n";
        }
        
        // Special prop for onclick if not present
        bool has_onclick = false;
        for(auto& prop : props) if(prop->name == "onclick") has_onclick = true;
        if(!has_onclick) {
            ss << "    webcc::function<void()> onclick;\n";
        }

        // State variables
        for(auto& var : state){
            ss << "    " << convert_type(var->type) << " " << var->name;
            if(var->initializer){
                ss << " = " << var->initializer->to_webcc();
            }
            ss << ";\n";
        }

        ss << "private:\n";

        // Element handles
        for(int i=0; i<element_count; ++i) {
            ss << "    webcc::handle el_" << i << ";\n";
        }
        
        // Child component members
        for(auto const& [comp_name, count] : component_members) {
            for(int i=0; i<count; ++i) {
                ss << "    " << comp_name << " " << comp_name << "_" << i << ";\n";
            }
        }

        ss << "\npublic:\n";

        // Methods
        for(auto& method : methods){
            std::set<std::string> modified_vars;
            method.collect_modifications(modified_vars);
            
            std::string updates;
            // Deduplicate updates by iterating over bindings and checking if any dependency is modified
            for(const auto& binding : bindings) {
                bool needs_update = false;
                for(const auto& mod : modified_vars) {
                    if(binding.dependencies.count(mod)) {
                        needs_update = true;
                        break;
                    }
                }
                
                if(needs_update) {
                    std::string el_var = "el_" + std::to_string(binding.element_id);
                    
                    bool optimized = false;
                    if(binding.expr) {
                        if(auto strLit = dynamic_cast<StringLiteral*>(binding.expr)) {
                             std::string fmt_code = "{ webcc::formatter<256> _fmt; ";
                             auto parts = strLit->parse();
                             for(auto& p : parts) {
                                 if(p.is_expr) fmt_code += "_fmt << (" + p.content + "); ";
                                 else fmt_code += "_fmt << \"" + p.content + "\"; ";
                             }
                             
                             if(binding.type == "attr") {
                                 fmt_code += "webcc::dom::set_attribute(" + el_var + ", \"" + binding.name + "\", _fmt.c_str()); }";
                             } else {
                                 fmt_code += "webcc::dom::set_inner_text(" + el_var + ", _fmt.c_str()); }";
                             }
                             updates += "        " + fmt_code + "\n";
                             optimized = true;
                        }
                    }

                    if(!optimized) {
                        if(binding.type == "attr") {
                            updates += "        webcc::dom::set_attribute(" + el_var + ", \"" + binding.name + "\", " + binding.value_code + ");\n";
                        } else if(binding.type == "text") {
                            updates += "        webcc::dom::set_inner_text(" + el_var + ", " + binding.value_code + ");\n";
                        }
                    }
                }
            }
            
            std::string original_name = method.name;
            if (method.name == "tick") {
                method.name = "_user_tick";
            }
            ss << "    " << method.to_webcc(updates);
            if (original_name == "tick") {
                method.name = original_name;
            }
        }

        // Generated handlers
        for(auto& handler : click_handlers) {
            ss << "    void _handler_" << handler.first << "() {\n";
            ss << "        " << handler.second << "();\n";
            ss << "    }\n";
        }

        // View method (Initialization only)
        ss << "    void view(webcc::handle parent = webcc::dom::get_body()) {\n";
        if(render_root){
            ss << ss_render.str();
        }
        // Register handlers
        for(auto& handler : click_handlers) {
            ss << "        g_dispatcher.register_click(el_" << handler.first << ", [this]() { this->_handler_" << handler.first << "(); });\n";
        }
        ss << "        webcc::flush();\n";
        ss << "    }\n";

        // Update method for event loop
        ss << "    void tick(float dt) {\n";
        
        // Call user tick if exists
        bool has_tick = false;
        for(auto& m : methods) if(m.name == "tick") has_tick = true;
        if(has_tick) ss << "        _user_tick(dt);\n";

        // Update children
        for(auto const& [comp_name, count] : component_members) {
            for(int i=0; i<count; ++i) {
                ss << "        " << comp_name << "_" << i << ".tick(dt);\n";
            }
        }
        ss << "    }\n";

        ss << "};\n";

        return ss.str();
    }
};


// =========================================================
// APP CONFIG
// =========================================================

struct AppConfig {
    std::string root_component;
    std::map<std::string, std::string> routes;
};

// =========================================================
// PARSER
// =========================================================

class Parser{
    private:
        std::vector<Token> tokens;
        size_t pos = 0;

        Token current(){
            return pos < tokens.size() ? tokens[pos] : tokens.back();
        }

        Token peek(int offset = 1){
            return (pos + offset) < tokens.size() ? tokens[pos + offset] : tokens.back();
        }

        void advance() { pos++; }

        bool match(TokenType type){
            if(current().type == type){
                advance();
                return true;
            }
            return false;
        }

        void expect(TokenType type, const std::string& msg){
            if(!match(type)){
                throw std::runtime_error(msg + " at line " + std::to_string(current().line));
            }
        }

        std::unique_ptr<Expression> parse_expression(){
            return parse_equality();
        }

        std::unique_ptr<Expression> parse_equality(){
            auto left = parse_comparison();

            while(current().type == TokenType::EQ || current().type == TokenType::NEQ){
                std::string op = current().value;
                advance();
                auto right = parse_comparison();
                left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right));
            }

            return left;
        }

        std::unique_ptr<Expression> parse_comparison(){
            auto left = parse_additive();

            while(current().type == TokenType::LT || current().type == TokenType::GT ||
                  current().type == TokenType::LTE || current().type == TokenType::GTE){
                std::string op = current().value;
                advance();
                auto right = parse_additive();
                left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right));
            }

            return left;
        }

        std::unique_ptr<Expression> parse_additive(){
            auto left = parse_multiplicative();

            while(current().type == TokenType::PLUS || current().type == TokenType::MINUS){
                std::string op = current().value;
                advance();
                auto right = parse_multiplicative();
                left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right));
            }

            return left;
        }

        std::unique_ptr<Expression> parse_postfix() {
            auto expr = parse_primary();
            
            while(true) {
                if (current().type == TokenType::PLUS_PLUS) {
                    advance();
                    expr = std::make_unique<PostfixOp>(std::move(expr), "++");
                } else if (current().type == TokenType::MINUS_MINUS) {
                    advance();
                    expr = std::make_unique<PostfixOp>(std::move(expr), "--");
                } else {
                    break;
                }
            }
            return expr;
        }

        std::unique_ptr<Expression> parse_unary() {
            if (current().type == TokenType::MINUS || current().type == TokenType::PLUS) {
                std::string op = current().value;
                advance();
                auto operand = parse_unary();
                return std::make_unique<UnaryOp>(op, std::move(operand));
            }
            return parse_postfix();
        }

        std::unique_ptr<Expression> parse_multiplicative(){
            auto left = parse_unary();

            while(current().type == TokenType::STAR || current().type == TokenType::SLASH || current().type == TokenType::PERCENT){
                std::string op = current().value;
                advance();
                auto right = parse_unary();
                left = std::make_unique<BinaryOp>(std::move(left), op, std::move(right));
            }

            return left;
        }

        std::unique_ptr<Expression> parse_primary(){
            // Integer literal
            if(current().type == TokenType::INT_LITERAL){
                int value = std::stoi(current().value);
                advance();
                return std::make_unique<IntLiteral>(value);
            }

            // Float literal
            if(current().type == TokenType::FLOAT_LITERAL){
                float value = std::stof(current().value);
                advance();
                return std::make_unique<FloatLiteral>(value);
            }

            // String literal
            if(current().type == TokenType::STRING_LITERAL){
                std::string value = current().value;
                advance();
                return std::make_unique<StringLiteral>(value);
            }

            // Identifer or function call
            if(current().type == TokenType::IDENTIFIER){
                std::string name = current().value;
                advance();
                std::unique_ptr<Expression> expr = std::make_unique<Identifier>(name);

                while(true) {
                    if(current().type == TokenType::LPAREN){
                        // Call
                        advance();
                        auto call = std::make_unique<FunctionCall>(expr->to_webcc());
                        
                        while(current().type != TokenType::RPAREN){
                            call->args.push_back(parse_expression());
                            if(current().type == TokenType::COMMA) advance();
                        }
                        expect(TokenType::RPAREN, "Expected ')'");
                        expr = std::move(call);
                    }
                    else if(current().type == TokenType::DOT){
                        advance();
                        std::string member = current().value;
                        expect(TokenType::IDENTIFIER, "Expected member name");
                        expr = std::make_unique<MemberAccess>(std::move(expr), member);
                    }
                    else {
                        break;
                    }
                }
                return expr;
            }

            // Parenthesized expression
            if(match(TokenType::LPAREN)){
                auto expr = parse_expression();
                expect(TokenType::RPAREN, "Expected ')'");
                return expr;
            }

            throw std::runtime_error("Unexpected token in expression: " + current().value + " (Type: " + std::to_string((int)current().type) + ") at line " + std::to_string(current().line));
        }

        std::unique_ptr<Statement> parse_statement(){
            // Block
            if(current().type == TokenType::LBRACE){
                advance();
                auto block = std::make_unique<BlockStatement>();
                while(current().type != TokenType::RBRACE && current().type != TokenType::END_OF_FILE){
                    block->statements.push_back(parse_statement());
                }
                expect(TokenType::RBRACE, "Expected '}'");
                return block;
            }

            // If
            if(current().type == TokenType::IF){
                advance();
                expect(TokenType::LPAREN, "Expected '('");
                auto cond = parse_expression();
                expect(TokenType::RPAREN, "Expected ')'");
                
                auto ifStmt = std::make_unique<IfStatement>();
                ifStmt->condition = std::move(cond);
                ifStmt->then_branch = parse_statement();
                
                if(match(TokenType::ELSE)){
                    ifStmt->else_branch = parse_statement();
                }
                return ifStmt;
            }

            // Return
            if(current().type == TokenType::RETURN){
                advance();
                auto ret = std::make_unique<ReturnStatement>();
                ret->value = parse_expression();
                expect(TokenType::SEMICOLON, "Expected ';'");
                return ret;
            }

            // Variable declaration
            if(current().type == TokenType::INT || current().type == TokenType::STRING ||
               current().type == TokenType::FLOAT || current().type == TokenType::BOOL){

                std::string type = current().value;
                advance();

                // Handle array type
                if(current().type == TokenType::LBRACKET){
                    advance();
                    expect(TokenType::RBRACKET, "Expected ']'");
                    type += "[]";
                }

                std::string name = current().value;
                expect(TokenType::IDENTIFIER, "Expected variable name");

                auto var_decl = std::make_unique<VarDeclaration>();
                var_decl->type = type;
                var_decl->name = name;

                if(match(TokenType::ASSIGN)){
                    var_decl->initializer = parse_expression();
                }

                expect(TokenType::SEMICOLON, "Expected ';'");
                return var_decl;
            }

            // Assignment
            if(current().type == TokenType::IDENTIFIER && 
               (peek().type == TokenType::ASSIGN || 
                peek().type == TokenType::PLUS_ASSIGN || 
                peek().type == TokenType::MINUS_ASSIGN)){
                
                std::string name = current().value;
                advance();
                
                TokenType opType = current().type;
                advance(); // skip op

                auto assign = std::make_unique<Assignment>();
                assign->name = name;
                
                auto val = parse_expression();
                
                if (opType == TokenType::PLUS_ASSIGN) {
                    auto left = std::make_unique<Identifier>(name);
                    auto binOp = std::make_unique<BinaryOp>(std::move(left), "+", std::move(val));
                    assign->value = std::move(binOp);
                } else if (opType == TokenType::MINUS_ASSIGN) {
                    auto left = std::make_unique<Identifier>(name);
                    auto binOp = std::make_unique<BinaryOp>(std::move(left), "-", std::move(val));
                    assign->value = std::move(binOp);
                } else {
                    assign->value = std::move(val);
                }

                expect(TokenType::SEMICOLON, "Expected ';'");
                return assign;
            }

            // Expression statement
            // try {
                auto expr = parse_expression();
                expect(TokenType::SEMICOLON, "Expected ';'");
                auto stmt = std::make_unique<ExpressionStatement>();
                stmt->expression = std::move(expr);
                return stmt;
            // } catch (...) {
                // Fall through to error
            // }

            // throw std::runtime_error("Unexpected statement");
        }

        std::unique_ptr<StructDef> parse_struct(){
            expect(TokenType::STRUCT, "Expected 'struct'");
            std::string name = current().value;
            expect(TokenType::IDENTIFIER, "Expected struct name");
            expect(TokenType::LBRACE, "Expected '{'");

            auto def = std::make_unique<StructDef>();
            def->name = name;

            while(current().type != TokenType::RBRACE && current().type != TokenType::END_OF_FILE){
                std::string type = current().value;
                // Handle types
                if(current().type == TokenType::INT || current().type == TokenType::STRING || 
                   current().type == TokenType::FLOAT || current().type == TokenType::BOOL ||
                   current().type == TokenType::IDENTIFIER){
                    advance();
                } else {
                    throw std::runtime_error("Expected type in struct");
                }
                
                std::string fieldName = current().value;
                expect(TokenType::IDENTIFIER, "Expected field name");
                expect(TokenType::SEMICOLON, "Expected ';'");

                def->fields.push_back({type, fieldName});
            }
            expect(TokenType::RBRACE, "Expected '}'");
            return def;
        }

        std::string parse_style_block() {
            expect(TokenType::LBRACE, "Expected '{'");
            std::string css = "";
            int brace_count = 1;
            
            Token prev = tokens[pos-1]; // The '{' we just consumed
            
            while (current().type != TokenType::END_OF_FILE) {
                if (current().type == TokenType::RBRACE && brace_count == 1) {
                    advance(); // Consume closing '}'
                    break;
                }
                
                if (current().type == TokenType::LBRACE) brace_count++;
                if (current().type == TokenType::RBRACE) brace_count--;
                
                Token tok = current();
                
                int prev_len = prev.value.length();
                if (prev.type == TokenType::STRING_LITERAL) prev_len += 2;

                if (tok.line > prev.line) {
                    css += " ";
                } else if (tok.column > prev.column + prev_len) {
                    css += " ";
                }
                
                if (tok.type == TokenType::STRING_LITERAL) {
                     css += "\"" + tok.value + "\"";
                } else {
                     css += tok.value;
                }
                
                prev = tok;
                advance();
            }
            return css;
        }

        std::unique_ptr<ASTNode> parse_html_element(){
            expect(TokenType::LT, "Expected '<'");
            int start_line = current().line;
            std::string tag = current().value;
            expect(TokenType::IDENTIFIER, "Expected tag name");

            // Check if component
            if(std::isupper(tag[0])){
                auto comp = std::make_unique<ComponentInstantiation>();
                comp->line = start_line;
                comp->component_name = tag;

                // Props
                while(current().type == TokenType::IDENTIFIER){
                    std::string prop_name = current().value;
                    advance();
                    
                    std::unique_ptr<Expression> prop_value;
                    if(match(TokenType::ASSIGN)){
                        if(current().type == TokenType::STRING_LITERAL){
                            prop_value = std::make_unique<StringLiteral>(current().value);
                            advance();
                        } else if(current().type == TokenType::INT_LITERAL){
                            prop_value = std::make_unique<IntLiteral>(std::stoi(current().value));
                            advance();
                        } else if(current().type == TokenType::FLOAT_LITERAL){
                            prop_value = std::make_unique<FloatLiteral>(std::stof(current().value));
                            advance();
                        } else if(match(TokenType::MINUS)){
                            if(current().type == TokenType::INT_LITERAL){
                                prop_value = std::make_unique<IntLiteral>(-std::stoi(current().value));
                                advance();
                            } else if(current().type == TokenType::FLOAT_LITERAL){
                                prop_value = std::make_unique<FloatLiteral>(-std::stof(current().value));
                                advance();
                            } else {
                                throw std::runtime_error("Expected number after '-' in prop value");
                            }
                        } else if(match(TokenType::LBRACE)){
                            prop_value = parse_expression();
                            expect(TokenType::RBRACE, "Expected '}'");
                        } else {
                            throw std::runtime_error("Expected prop value");
                        }
                    } else {
                        // Boolean prop?
                        prop_value = std::make_unique<StringLiteral>("true");
                    }
                    comp->props.push_back({prop_name, std::move(prop_value)});
                }

                // Self-closing
                if(match(TokenType::SLASH)){
                    expect(TokenType::GT, "Expected '>'");
                    return comp;
                }

                expect(TokenType::GT, "Expected '>'");
                throw std::runtime_error("Custom components must be self-closing for now: " + tag);
            }

            auto el = std::make_unique<HTMLElement>();
            el->line = start_line;
            el->tag = tag;

            // Attributes
            while(current().type == TokenType::IDENTIFIER || current().type == TokenType::STYLE){
                std::string attrName = current().value;
                advance();
                
                std::unique_ptr<Expression> attrValue;
                if(match(TokenType::ASSIGN)){
                    if(current().type == TokenType::STRING_LITERAL){
                        attrValue = std::make_unique<StringLiteral>(current().value);
                        advance();
                    } else if(match(TokenType::LBRACE)){
                        attrValue = parse_expression();
                        expect(TokenType::RBRACE, "Expected '}'");
                    } else {
                        throw std::runtime_error("Expected attribute value");
                    }
                } else {
                    // Boolean attribute? Treat as "true"
                    attrValue = std::make_unique<StringLiteral>("true");
                }
                el->attributes.push_back({attrName, std::move(attrValue)});
            }

            // Self-closing
            if(match(TokenType::SLASH)){
                expect(TokenType::GT, "Expected '>'");
                return el;
            }

            expect(TokenType::GT, "Expected '>'");

            // Children
            while(true){
                if(current().type == TokenType::LT){
                    if(peek().type == TokenType::SLASH){
                        // Closing tag
                        break;
                    }
                    // Child element
                    el->children.push_back(parse_html_element());
                } else if(current().type == TokenType::LBRACE){
                    // Expression
                    advance();
                    el->children.push_back(parse_expression());
                    expect(TokenType::RBRACE, "Expected '}'");
                } else {
                    // Text content
                    std::string text;
                    bool first = true;
                    Token prev_token = current();
                    while(current().type != TokenType::LT && current().type != TokenType::LBRACE && current().type != TokenType::END_OF_FILE){
                        if(!first){
                            int prev_len = prev_token.value.length();
                            if (prev_token.type == TokenType::STRING_LITERAL) prev_len += 2;
                            
                            if (prev_token.line != current().line || prev_token.column + prev_len != current().column) {
                                text += " ";
                            }
                        }
                        if(current().type == TokenType::STRING_LITERAL) text += current().value;
                        else text += current().value;
                        
                        prev_token = current();
                        advance();
                        first = false;
                    }
                    if(!text.empty()){
                        el->children.push_back(std::make_unique<TextNode>(text));
                    }
                    
                    if(current().type == TokenType::END_OF_FILE) break;
                }
            }

            expect(TokenType::LT, "Expected '<'");
            expect(TokenType::SLASH, "Expected '/'");
            if(current().value != tag){
                throw std::runtime_error("Mismatched closing tag: expected " + tag + ", got " + current().value);
            }
            expect(TokenType::IDENTIFIER, "Expected tag name");
            expect(TokenType::GT, "Expected '>'");

            return el;
        }

    public:
        std::vector<Component> components;
        AppConfig app_config;
        Parser(const std::vector<Token>& toks) : tokens(toks){}

        Component parse_component(){
            Component comp;

            expect(TokenType::COMPONENT, "Expected 'component'");
            comp.name = current().value;
            expect(TokenType::IDENTIFIER, "Expected component name");
            expect(TokenType::LBRACE, "Expected '{'");

            // Parse state variables and methods
            while(current().type != TokenType::VIEW && current().type != TokenType::RBRACE && current().type != TokenType::END_OF_FILE){
                // Variable declaration
                if(current().type == TokenType::INT || current().type == TokenType::STRING || 
                   current().type == TokenType::FLOAT || current().type == TokenType::BOOL || 
                   current().type == TokenType::IDENTIFIER){
                    auto var_decl = std::make_unique<VarDeclaration>();
                    var_decl->type = current().value;
                    advance();

                    if(current().type == TokenType::LBRACKET){
                        advance();
                        expect(TokenType::RBRACKET, "Expected ']'");
                        var_decl->type += "[]";
                    }

                    var_decl->name = current().value;
                    expect(TokenType::IDENTIFIER, "Expected variable name");

                    if(match(TokenType::ASSIGN)){
                        var_decl->initializer = parse_expression();
                    }

                    expect(TokenType::SEMICOLON, "Expected ';'");
                    comp.state.push_back(std::move(var_decl));
                }
                // Struct definition
                else if(current().type == TokenType::STRUCT){
                    comp.structs.push_back(parse_struct());
                }
                // Function definition
                else if(current().type == TokenType::DEF){
                    advance();
                    FunctionDef func;
                    func.name  = current().value;
                    expect(TokenType::IDENTIFIER, "Expected function name");
                    expect(TokenType::LPAREN, "Expected '('");
                    
                    // Parse parameters
                    while(current().type != TokenType::RPAREN){
                        std::string paramType = current().value;
                        if(current().type == TokenType::INT || current().type == TokenType::FLOAT || 
                           current().type == TokenType::STRING || current().type == TokenType::BOOL || 
                           current().type == TokenType::IDENTIFIER){
                            advance();
                        } else {
                             throw std::runtime_error("Expected parameter type");
                        }

                        std::string paramName = current().value;
                        expect(TokenType::IDENTIFIER, "Expected parameter name");
                        
                        func.params.push_back({paramType, paramName});

                        if(current().type == TokenType::COMMA){
                            advance();
                        }
                    }

                    expect(TokenType::RPAREN, "Expected ')'");
                    expect(TokenType::COLON, "Expected ':'");
                    func.return_type = current().value;
                    advance();
                    expect(TokenType::LBRACE, "Expected '{'");

                    while(current().type != TokenType::RBRACE){
                        func.body.push_back(parse_statement());
                    }

                    expect(TokenType::RBRACE, "Expected '}'");
                    comp.methods.push_back(std::move(func));
                }
                // Tick definition
                else if(current().type == TokenType::TICK){
                    advance();
                    FunctionDef func;
                    func.name = "tick";
                    func.return_type = "void";
                    expect(TokenType::LPAREN, "Expected '('");
                    
                    // Parse parameters
                    while(current().type != TokenType::RPAREN){
                        std::string paramType = current().value;
                        if(current().type == TokenType::INT || current().type == TokenType::FLOAT || 
                           current().type == TokenType::STRING || current().type == TokenType::BOOL || 
                           current().type == TokenType::IDENTIFIER){
                            advance();
                        } else {
                             throw std::runtime_error("Expected parameter type");
                        }

                        std::string paramName = current().value;
                        expect(TokenType::IDENTIFIER, "Expected parameter name");
                        
                        func.params.push_back({paramType, paramName});

                        if(current().type == TokenType::COMMA){
                            advance();
                        }
                    }

                    expect(TokenType::RPAREN, "Expected ')'");
                    expect(TokenType::LBRACE, "Expected '{'");

                    while(current().type != TokenType::RBRACE){
                        func.body.push_back(parse_statement());
                    }

                    expect(TokenType::RBRACE, "Expected '}'");
                    comp.methods.push_back(std::move(func));
                }
                // Prop declaration
                else if(current().type == TokenType::PROP){
                    advance();
                    auto prop_decl = std::make_unique<PropDeclaration>();
                    
                    prop_decl->type = current().value;
                    // Check type
                    if(current().type == TokenType::INT || current().type == TokenType::STRING || 
                       current().type == TokenType::FLOAT || current().type == TokenType::BOOL || 
                       current().type == TokenType::IDENTIFIER){
                        advance();
                    } else {
                         throw std::runtime_error("Expected prop type");
                    }

                    if(current().type == TokenType::LBRACKET){
                        advance();
                        expect(TokenType::RBRACKET, "Expected ']'");
                        prop_decl->type += "[]";
                    }

                    prop_decl->name = current().value;
                    expect(TokenType::IDENTIFIER, "Expected prop name");

                    if(match(TokenType::ASSIGN)){
                        prop_decl->default_value = parse_expression();
                    }

                    expect(TokenType::SEMICOLON, "Expected ';'");
                    comp.props.push_back(std::move(prop_decl));
                }
                // Style block
                else if(current().type == TokenType::STYLE){
                    advance();
                    bool is_global = false;
                    if (current().type == TokenType::IDENTIFIER && current().value == "global") {
                        is_global = true;
                        advance();
                    }
                    std::string css = parse_style_block();
                    if (is_global) {
                        comp.global_css += css + "\n";
                    } else {
                        comp.css += css + "\n";
                    }
                }
                else {
                    advance();
                }
            }

            // Parse render block
            if(match(TokenType::VIEW)){
                expect(TokenType::LBRACE, "Expected '{'");
                comp.render_root = parse_html_element();
                expect(TokenType::RBRACE, "Expected '}'");
            }

            return comp;
        }

        void parse_app() {
             expect(TokenType::LBRACE, "Expected '{'");
             
             while(current().type != TokenType::RBRACE && current().type != TokenType::END_OF_FILE){
                 std::string key = current().value;
                 expect(TokenType::IDENTIFIER, "Expected key");
                 expect(TokenType::ASSIGN, "Expected '='");
                 
                 if(key == "root"){
                     app_config.root_component = current().value;
                     expect(TokenType::IDENTIFIER, "Expected component name");
                 } else if(key == "routes"){
                     expect(TokenType::LBRACE, "Expected '{'");
                     while(current().type != TokenType::RBRACE){
                         std::string route = current().value;
                         expect(TokenType::STRING_LITERAL, "Expected route string");
                         expect(TokenType::COLON, "Expected ':'");
                         std::string comp = current().value;
                         expect(TokenType::IDENTIFIER, "Expected component name");
                         app_config.routes[route] = comp;
                         
                         if(current().type == TokenType::COMMA) advance();
                     }
                     expect(TokenType::RBRACE, "Expected '}'");
                 }
             }
             expect(TokenType::RBRACE, "Expected '}'");
        }

        void parse_file(){
            while(current().type != TokenType::END_OF_FILE){
                if(current().type == TokenType::COMPONENT){
                    components.push_back(parse_component());
                } else if(current().type == TokenType::IDENTIFIER && current().value == "app"){
                    advance();
                    parse_app();
                } else {
                    advance();
                }
            }

            // Default to Main if no app config
            if(app_config.root_component.empty()){
                for(const auto& comp : components){
                    if(comp.name == "Main"){
                        app_config.root_component = "Main";
                        break;
                    }
                }
            }
        }
};

// =========================================================
// MAIN COMPILER
// =========================================================

void validate_view_hierarchy(const std::vector<Component>& components) {
    std::map<std::string, const Component*> component_map;
    for (const auto& comp : components) {
        component_map[comp.name] = &comp;
    }

    std::function<void(ASTNode*)> validate_node = [&](ASTNode* node) {
        if (!node) return;

        if (auto* comp_inst = dynamic_cast<ComponentInstantiation*>(node)) {
            auto it = component_map.find(comp_inst->component_name);
            if (it != component_map.end()) {
                if (!it->second->render_root) {
                     throw std::runtime_error("Component '" + comp_inst->component_name + "' is used in a view but has no view definition (logic-only component) at line " + std::to_string(comp_inst->line));
                }
            }
        } else if (auto* el = dynamic_cast<HTMLElement*>(node)) {
            for (const auto& child : el->children) {
                validate_node(child.get());
            }
        }
    };

    for (const auto& comp : components) {
        if (comp.render_root) {
            validate_node(comp.render_root.get());
        }
    }
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

    // Read source file
    std::ifstream file(input_file);
    if(!file){
        std::cerr << "Error: Could not open file " << input_file << std::endl;
        return 1;
    }

    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    try {
        // Lexical analysis
        std::cerr << "Lexing..." << std::endl;
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        std::cerr << "Lexing done. Tokens: " << tokens.size() << std::endl;

        // Parsing
        std::cerr << "Parsing..." << std::endl;
        Parser parser(tokens);
        parser.parse_file();
        std::cerr << "Parsing done. Components: " << parser.components.size() << std::endl;

        validate_view_hierarchy(parser.components);

        // Determine output filename
        std::string output_cc;
        if(!output_dir.empty()) {
            // Create directory
            std::string cmd = "mkdir -p " + output_dir;
            if(system(cmd.c_str()) != 0){
                 std::cerr << "Error: Could not create output directory " << output_dir << std::endl;
                 return 1;
            }
            
            // Extract filename from input_file
            size_t last_slash = input_file.find_last_of("/\\");
            std::string filename = (last_slash == std::string::npos) ? input_file : input_file.substr(last_slash + 1);
            size_t last_dot = filename.find_last_of('.');
            if(last_dot != std::string::npos) filename = filename.substr(0, last_dot);
            
            output_cc = output_dir;
            if(output_cc.back() != '/') output_cc += "/";
            output_cc += filename + ".cc";
        } else {
            output_cc = input_file.substr(0, input_file.find_last_of('.')) + ".cc";
        }

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

        // Forward declarations
        for(auto& comp : parser.components) {
            out << "class " << comp.name << ";\n";
        }
        out << "\n";

        for(auto& comp : parser.components) {
            out << comp.to_webcc();
        }

        if(parser.app_config.root_component.empty()) {
             std::cerr << "Error: No root component defined. Use 'app { root = ComponentName }' to define the entry point." << std::endl;
             return 1;
        }

        out << "\n" << parser.app_config.root_component << "* app = nullptr;\n";
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
            out << "    void* app_mem = webcc::malloc(sizeof(" << parser.app_config.root_component << "));\n";
            out << "    app = new (app_mem) " << parser.app_config.root_component << "();\n";
            
            // Inject CSS
            std::string all_css;
            for(const auto& comp : parser.components) {
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

            std::string cmd = "mkdir -p build/.webcc_cache && webcc " + abs_output_cc.string();
            cmd += " --out " + abs_output_dir.string();
            cmd += " --cache-dir build/.webcc_cache";

            std::cerr << "Running: " << cmd << std::endl;
            int ret = system(cmd.c_str());
            if (ret != 0) {
                std::cerr << "Error: webcc compilation failed." << std::endl;
                return 1;
            }
            
            if (!keep_cc) {
                std::remove(output_cc.c_str());
            }
        }

    } catch(const std::exception& e){
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
