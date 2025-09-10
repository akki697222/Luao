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
