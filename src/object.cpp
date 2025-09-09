#include <object.hpp>
#include <table.hpp>

namespace luao {

LuaValue LuaGCObject::getMetamethod(const LuaValue& key) const {
    if (!metatable) return LuaValue();
    return metatable->get(key);
}

} // namespace luao
