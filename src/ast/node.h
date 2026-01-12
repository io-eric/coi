#pragma once

#include <string>
#include <vector>
#include <memory>
#include <set>
#include <map>
#include <sstream>
#include <functional>

// Forward declarations
struct Expression;
struct Statement;

// Cross-component state that persists across all components in one compilation
struct CompilerSession {
    std::set<std::string> components_with_tick;  // Components that have tick methods
};

// Represents a dependency on a member of an object (e.g., net.connected)
struct MemberDependency {
    std::string object;   // e.g., "net"
    std::string member;   // e.g., "connected"
    
    bool operator<(const MemberDependency& other) const {
        if (object != other.object) return object < other.object;
        return member < other.member;
    }
};

// Base AST node
struct ASTNode {
    virtual ~ASTNode() = default;
    virtual std::string to_webcc() = 0;
    virtual void collect_dependencies(std::set<std::string>& deps) {}
    virtual void collect_member_dependencies(std::set<MemberDependency>& member_deps) {}
    int line = 0;
};

// Base for expressions (things that return values)
struct Expression : ASTNode {
    virtual bool is_static() { return false; }
};

// Base for statements (actions)
struct Statement : ASTNode {};

// Type conversion utility
std::string convert_type(const std::string& type);
