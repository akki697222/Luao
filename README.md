> [!IMPORTANT]
> This project is still incomplete and in progress, so it is not guaranteed to be complete and may contain unfixed or serious bugs.

# Luao
The new, modern Lua language specification

<img src="https://github.com/akki697222/Luao/blob/main/luao-logo.png?raw=true" width="200">

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

### Typed Function/Variable
```luao
function add(a: int, b: int) -> int
    return a + b
end

-- multiple type annotation
function returnMultiple() -> (int, long, str)
    return 1, 9999999, "Hello!"
end

-- the return type can be omitted.
function customPrint(...: obj)
    print("CustomPrint: " .. tostring(...))
end

-- attributed and typed local
local x, y: int, int = 10, 20
local helloWorld<const>: string = "Hello, World!"
```

### Type casting
```luao
local x: long = 10
local y: int = 5

function add(a: int, b: int) -> int
    return a + b
end

-- Error!!
print(add(x, y))

-- Ok
print(add(cast(x, int), y))
```

### Implicit Casting
```luao
function mul(a: long, b: long) -> long
    return a * b
end

local x: int = 20
local y: int = 10

-- Implicitly cast to long
-- Cannot cast from long to int
print(mul(x, y)) -- 200
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
for i, name each strings do
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
    self _max_len<const>: int

    -- adding ! to type annotation makes it nullable
    function $init(_type: type, max_len: !int)
        self._type = _type
        self._index = {}
        if max_len == nil then
            self._max_len = -1
        elseif max_len < 1 or max_len > int.MAX then
            throw InvalidArrayLengthError("invalid length " .. len .. " (expected 1~2147483647)")
        end
    end

    -- iterator
    function next()
        local index = 0
        return function()
            index = index + 1
            return index, self._index[index]
        end
    end

    function add(v: _type)
        self:set(#self._index, v)
    end

    function set(i: int, v: _type)
        if i >= self._max_len and self._max_len ~= -1 then
            throw ArrayIndexOutOfBoundsError("index " .. i .. " out of bounds " .. self._len)
        end
        _index[i] = v
        self._len = #self._index
    end

    function remove(i: int, v: _type)
        if i >= self._len and self._max_len ~= -1 then
            throw ArrayIndexOutOfBoundsError("index " .. i .. " out of bounds " .. self._len)
        end
        table.remove(_index, i)
        self._len = #self._index
    end

    function clear()
        for i = 1, #self._index do
            table.remove(_index, i)
        end
    end

    function len()
        return #self._index
    end

    -- __len metamethod
    function __len<meta>()
        return #self._index
    end
end
```

### Operator Overload
```luao
object CppStdCout;
    function __shl<meta>(a: CppStdCout, b: str)
        print(b)
    end
end

CppStdCout << "Hello, World!"
```

### Advanced Error Handling
```luao
-- dangerous function (lol)
function dangerousFunction()
    throw RuntimeError("Boom!")
end

-- basic try-catch-finally
try
    dangerousFunction()
catch (e: RuntimeError) do
    print("Error: " .. e) -- Error: RuntimeError: Boom!
finally
    print("Executed!")
end

-- without finally
try
    dangerousFunction()
catch (e: RuntimeError) do
    print("Error: " .. e) -- Error: RuntimeError: Boom!
end

-- with multiple cases
try 
    dangerousFunction()
catch (e: RuntimeError) do
    print("RuntimeError occurred")
catch (e: SomeError) do
    print("SomeError occurred")
end
```

### Metamethod Injection
```luao
object Vec3;
    self x: int
    self y: int
    self z: int
    
    function $init(x: int, y: int, z: int)
        self.x = x
        self.y = y
        self.z = z
    end

    function unpack() -> (int, int, int)
        return self.x, self.y, self.z
    end

    function __add<meta>(a: Vec3, b: Vec3) -> Vec3
        -- type inference
        local new = Vec3(0, 0, 0)
        new.x = a.x + b.x
        new.y = a.y + b.y
        new.z = a.z + b.z
        return new
    end

    function __sub<meta>(a: Vec3, b: Vec3) -> Vec3
        -- type inference
        local new = Vec3(0, 0, 0)
        new.x = a.x - b.x
        new.y = a.y - b.y
        new.z = a.z - b.z
        return new
    end

    function __mul<meta>(a: Vec3, b: Vec3) -> Vec3
        -- type inference
        local new = Vec3(0, 0, 0)
        new.x = a.x * b.x
        new.y = a.y * b.y
        new.z = a.z * b.z
        return new
    end

    function __div<meta>(a: Vec3, b: Vec3) -> Vec3
        -- type inference
        local new = Vec3(0, 0, 0)
        new.x = a.x / b.x
        new.y = a.y / b.y
        new.z = a.z / b.z
        return new
    end
end
```