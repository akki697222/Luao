#include <table.hpp>
#include <object.hpp>
#include <functional>
#include <cmath>
#include <vector>
#include <algorithm>
#ifdef _MSC_VER
#include <intrin.h>
#endif

// --- Helper Functions ---

static luaNumber get_number_from_value(const LuaValue& val) {
    if (auto* num = dynamic_cast<const LuaNumber*>(val.getObject())) {
        return num->getValue();
    }
    if (auto* integer = dynamic_cast<const LuaInteger*>(val.getObject())) {
        return static_cast<luaNumber>(integer->getValue());
    }
    return 0.0;
}

static bool keys_equal(const LuaValue& k1, const LuaValue& k2) {
    if (k1.getType() != k2.getType()) return false;
    switch (k1.getType()) {
        case LuaType::NIL: return true;
        case LuaType::NUMBER: return get_number_from_value(k1) == get_number_from_value(k2);
        case LuaType::STRING: {
            auto* s1 = static_cast<const LuaString*>(k1.getObject());
            auto* s2 = static_cast<const LuaString*>(k2.getObject());
            return s1 && s2 && s1->getValue() == s2->getValue();
        }
        default: return k1.getObject() == k2.getObject();
    }
}

size_t LuaTable::hash(const LuaValue& key) const {
    switch (key.getType()) {
        case LuaType::NIL: return 0;
        case LuaType::NUMBER: return std::hash<luaNumber>{}(get_number_from_value(key));
        case LuaType::STRING: {
            auto* s = static_cast<const LuaString*>(key.getObject());
            return std::hash<std::string>{}(s ? s->getValue() : "");
        }
        default: return std::hash<const void*>{}(key.getObject());
    }
}

LuaTable::Node* LuaTable::main_position(const LuaValue& key) {
    if (m_nodes.empty()) return nullptr;
    size_t h = hash(key);
    return &m_nodes[h & (m_nodes.size() - 1)];
}

const LuaTable::Node* LuaTable::main_position(const LuaValue& key) const {
    if (m_nodes.empty()) return nullptr;
    size_t h = hash(key);
    return &m_nodes[h & (m_nodes.size() - 1)];
}

LuaTable::Node* LuaTable::get_free_node() {
    while (m_last_free_hint > 0) {
        m_last_free_hint--;
        if (m_nodes[m_last_free_hint].key.getType() == LuaType::NIL) {
            return &m_nodes[m_last_free_hint];
        }
    }
    return nullptr;
}

void LuaTable::raw_insert(const LuaValue& key, const LuaValue& value) {
    // This is a simplified insert that assumes there is enough space.
    // It's meant to be called only from rehash.
    Node* mp = main_position(key);
    if (mp->key.getType() == LuaType::NIL) {
        mp->key = key;
        mp->value = value;
        return;
    }

    // Collision
    Node* n = mp;
    while(n->next != -1) {
        n = &m_nodes[n->next];
    }

    Node* free_node = get_free_node();
    if (free_node) {
        free_node->key = key;
        free_node->value = value;
        n->next = (free_node - m_nodes.data());
    }
    // If no free node, something is wrong with rehash sizing.
}

// ltable.c -> computesizes + rehash
void LuaTable::rehash() {
    // 1. Collect all nodes into a temporary vector.
    // Note on memory management:
    // The `LuaValue` class uses RAII for reference counting. When a Node is
    // copied into `all_nodes`, the LuaValue copy constructor is called, which
    // increments the reference count of the underlying LuaGCObject.
    // When `all_nodes` is destroyed at the end of this function, the LuaValue
    // destructors are called, decrementing the reference counts.
    // This ensures the objects stay alive while they are being moved to the
    // new table storage.
    std::vector<Node> all_nodes;
    for (size_t i = 0; i < m_array.size(); ++i) {
        if (m_array[i].getType() != LuaType::NIL) {
            Node n;
            n.key = LuaValue(new LuaInteger(i + 1), LuaType::NUMBER);
            n.value = m_array[i];
            all_nodes.push_back(n);
        }
    }
    for (const auto& node : m_nodes) {
        if (node.key.getType() != LuaType::NIL) {
            all_nodes.push_back(node);
        }
    }

    if (all_nodes.empty()) {
        m_array.clear();
        m_nodes.clear();
        return;
    }

    // 2. Count integer keys in logarithmic bins
    constexpr int MAX_LOG_2 = sizeof(luaInt) * 8;
    std::vector<int> nums(MAX_LOG_2, 0);
    int total_hash_keys = 0;
    for (const auto& node : all_nodes) {
        if (node.key.getType() == LuaType::NUMBER) {
            if (auto* integer = dynamic_cast<LuaInteger*>(node.key.getObject())) {
                luaInt k = integer->getValue();
                if (k > 0) {
                    int log2_k = 0;
                    if (k > 0) { // Calculate floor(log2(k))
                        unsigned long temp_k = k;
                        #ifdef _MSC_VER
                        unsigned long index;
                        _BitScanReverse(&index, temp_k);
                        log2_k = index;
                        #else
                        log2_k = (sizeof(unsigned long)*8 - 1) - __builtin_clzl(temp_k);
                        #endif
                    }
                    if (log2_k < MAX_LOG_2) {
                        nums[log2_k]++;
                    }
                    continue;
                }
            }
        }
        total_hash_keys++;
    }

    // 3. Compute optimal array size
    size_t new_array_size = 0;
    int num_array_values = 0;
    int cumulative_int_keys = 0;
    for (int i = 0; i < MAX_LOG_2; ++i) {
        cumulative_int_keys += nums[i];
        if (cumulative_int_keys > (1 << (i - 1))) {
            new_array_size = (1 << i);
            num_array_values = cumulative_int_keys;
        }
    }

    // Keys that are integers but do not fit in the array part must go to the hash part.
    int total_integer_keys = cumulative_int_keys;
    total_hash_keys += (total_integer_keys - num_array_values);

    // 4. Resize and re-populate
    m_array.assign(new_array_size, LuaValue());
    size_t new_hash_size = 8;
    while (new_hash_size < total_hash_keys) {
        new_hash_size <<= 1;
    }
    m_nodes.assign(new_hash_size, Node());
    m_last_free_hint = new_hash_size;

    for (const auto& node : all_nodes) {
        if (node.key.getType() == LuaType::NUMBER) {
            if (auto* integer = dynamic_cast<LuaInteger*>(node.key.getObject())) {
                luaInt k = integer->getValue();
                if (k > 0 && k <= new_array_size) {
                    m_array[k - 1] = node.value;
                    continue;
                }
            }
        }
        raw_insert(node.key, node.value);
    }
}

// --- Public Methods ---

LuaTable::LuaTable() = default;
LuaTable::~LuaTable() = default;

LuaValue LuaTable::get(const LuaValue& key) const {
    if (key.getType() == LuaType::NUMBER) {
        if (auto* integer = dynamic_cast<const LuaInteger*>(key.getObject())) {
            luaInt idx = integer->getValue();
            if (idx >= 1 && idx <= m_array.size()) {
                return m_array[idx - 1];
            }
        }
    }
    if (m_nodes.empty()) return LuaValue();
    const Node* n = main_position(key);
    while (n) {
        if (keys_equal(n->key, key)) return n->value;
        if (n->next == -1) break;
        n = &m_nodes[n->next];
    }
    return LuaValue();
}

void LuaTable::set(const LuaValue& key, const LuaValue& value) {
    if (key.getType() == LuaType::NIL || (key.getType() == LuaType::NUMBER && std::isnan(get_number_from_value(key)))) {
        return; // Invalid key
    }

    // Handle array part
    if (key.getType() == LuaType::NUMBER) {
        if (auto* integer = dynamic_cast<const LuaInteger*>(key.getObject())) {
            luaInt idx = integer->getValue();
            if (idx >= 1) {
                if (idx <= m_array.size()) { m_array[idx - 1] = value; return; }
                if (idx == m_array.size() + 1) { m_array.push_back(value); return; }
            }
        }
    }

    if (m_nodes.empty() && value.getType() == LuaType::NIL) {
        return; // Deleting from empty hash part
    }
    if (m_nodes.empty()) {
        rehash();
    }

    // Find node and its predecessor
    Node* mp = main_position(key);
    Node* prev = nullptr;
    Node* n = mp;

    while (n != nullptr && n->key.getType() != LuaType::NIL) {
        if (keys_equal(n->key, key)) {
            // Key found, perform update or deletion
            if (value.getType() == LuaType::NIL) {
                // --- DELETION ---
                if (prev) { // Node is in a chain
                    prev->next = n->next;
                } else { // Node is the main position
                    if (n->next != -1) {
                        // Move next node's data into main position
                        Node* next_node = &m_nodes[n->next];
                        n->key = next_node->key;
                        n->value = next_node->value;
                        n->next = next_node->next;
                        // Clear the now-unused node
                        next_node->key = LuaValue();
                        next_node->value = LuaValue();
                        next_node->next = -1;
                    } else {
                        // Main position with no chain, just clear it
                        n->key = LuaValue();
                        n->value = LuaValue();
                    }
                }
            } else {
                // --- UPDATE ---
                n->value = value;
            }
            return;
        }
        prev = n;
        if (n->next == -1) {
            n = nullptr; // End of chain
        } else {
            n = &m_nodes[n->next];
        }
    }

    // Key not found, perform insertion (if value is not nil)
    if (value.getType() == LuaType::NIL) {
        return;
    }

    Node* free_node = get_free_node();
    if (free_node == nullptr) {
        rehash();
        set(key, value); // Retry insertion after rehashing
        return;
    }

    if (mp->key.getType() == LuaType::NIL) { // Main position is free
        mp->key = key;
        mp->value = value;
    } else { // Collision, add to end of chain
        Node* chain_end = mp;
        while (chain_end->next != -1) {
            chain_end = &m_nodes[chain_end->next];
        }
        free_node->key = key;
        free_node->value = value;
        chain_end->next = (free_node - m_nodes.data());
    }
}

void LuaTable::setMetatable(LuaTable* mt) {
    m_metatable = mt;
}
