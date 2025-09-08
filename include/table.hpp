#pragma once

#include <object.hpp>
#include <luao.hpp>
#include <string>
#include <vector>

namespace luao {

class LuaTable : public LuaGCObject {
public:
    LuaTable();
    ~LuaTable() override;

    LuaType getType() const override { return LuaType::TABLE; }
    std::string typeName() const override { return "table"; }

    LuaValue get(const LuaValue& key) const;
    void set(const LuaValue& key, const LuaValue& value);

    LuaTable* getMetatable() const { return m_metatable; }
    void setMetatable(LuaTable* mt);

private:
    struct Node {
        LuaValue key;
        LuaValue value;
        int next = -1; // index in m_nodes for chaining

        // Default constructor to allow vector resizing.
        Node() = default;
    };

    std::vector<LuaValue> m_array;
    std::vector<Node> m_nodes;
    size_t m_last_free_hint = 0;
    LuaTable* m_metatable = nullptr;

    // private helpers
    Node* main_position(const LuaValue& key);
    const Node* main_position(const LuaValue& key) const;
    size_t hash(const LuaValue& key) const;
    Node* get_free_node();
    void rehash();
    void raw_insert(const LuaValue& key, const LuaValue& value);
};

} // namespace luao