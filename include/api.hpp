#pragma once

#include <luao.hpp>
#include <object.hpp>
#include <vm.hpp>

namespace luao {
namespace api {

static LuaValue& getglobal(VM& vm, CallInfo* ci);

} // namespace api
} // namespace luao