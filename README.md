> [!IMPORTANT]
> This project is still incomplete and in progress, so it is not guaranteed to be complete and may contain unfixed or serious bugs.

# Luao
## The new, modern Lua language specification

## Overview
- Advanced class-based object-oriented programming including inheritance and abstraction
- Better error handling using try-catch-finally statements and error types

## Compatibility with Lua 5.4
Luao is fully backward compatible with Lua 5.4. Existing Lua scripts will run without modification.  
Metatable functionality is **deprecated** in Luao, but still supported for compatibility.  
If you use metatables (such as `setmetatable`, `getmetatable`, `__index`, operator overloading via metatables, etc.), you will receive a warning at compile or runtime, but your code will continue to work.

- Standard Lua syntax and APIs are available.
- Metatable-related functions are supported, but deprecated (warnings will be issued).
- New features (class, type annotations, error handling, operator overloading, etc.) are available via Luao syntax.
- You can mix traditional Lua code and Luao extensions in the same file.

### Typed Function
```luao
function add(a: int, b: int): int
    return a + b
end

-- the return type can be omitted.
function customPrint(...: obj)
    print("CustomPrint: " .. tostring(...))
end
```

### Type casting
```luao
local x: long = 10
local y: int = 5

function add(a: int, b: int): int
    return a + b
end

-- Error!!
print(add(x, y))

-- Ok
print(add(cast(x, int), y))
```

### Class
```luao
class Animal<virtual>;
    -- class field
    self age: number
    self name<const>: str

    function $init(age: number, name: str)
        self.age = age
        self.name = name
    end

    -- abstract(virtual) method
    function speak<virtual>()
end

class Human : Animal;
    function $init(age: number, name: str)
        parent(age, name)
    end

    local function privateMethod()
        -- private method are declare with local
    end

    function speak<impl>()
        print("Hello!")
    end
end
```

### Interfaces
```luao
class IEatable<interface>;
    function eat<virtual>()
end

class akki : Human, IEatable;
    function $init()
        parent(15, "akki")
    end

    -- override
    function speak<impl>()
        print("Helllllllllllllo!!!!!!!!!!!!")
    end

    function eat<impl>()
        print(":shi:")
    end
end
```

### Arrays
```luao
local Array = require("Array")

local strings = Array(str)
strings.add("Apple")
strings.add("Banana")
strings.add("Orange")

-- for iteration without pairs and ipairs
-- for each requires 'next()' method in object
for name each strings do
    print(name)
end
```

### ...and array impl
```luao
-- Array module

type InvalidArrayLengthError: RuntimeError
type ArrayIndexOutOfBoundsError: RuntimeError

object Array;
    self _type<const>: type
    self _index<const>: table
    self _len<const>: int

    -- length are nullable
    function $init(_type: type, len: !int)
        self._type = _type
        self._index = {}
        if len == nil then
            self.len = 1
        elseif len < 1 or len > int.MAX then
            throw InvalidArrayLengthError("invalid length " .. len .. " (expected 1~2147483647)")
        end
    end

    function add(v: _type)
        self:set(#self._index, v)
    end

    function set(i: int, v: _type)
        if i >= self._len then
            throw ArrayIndexOutOfBoundsError("index " .. i .. " out of bounds " .. self._len)
        end
        _index[i] = v
    end

    function remove(i: int, v: _type)
        if i >= self._len then
            throw ArrayIndexOutOfBoundsError("index " .. i .. " out of bounds " .. self._len)
        end
        table.remove(_index, i)
    end

    function clear()
        for i = 1, #self._index do
            table.remove(_index, i)
        end
    end
end
```

### Operator Overload
```luao
object CppStdCout;
    function __shl(a: CppStdCout, b: str)
        print(b)
    end
end

CppStdCout << "Hello, World!"
```