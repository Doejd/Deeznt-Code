highlight("false", "reserved")
highlight("true", "reserved")
highlight("nil", "reserved")

highlight("+", "operator")
highlight("-", "operator")
highlight("*", "operator")
highlight("/", "operator")
highlight("^", "operator")
highlight("<", "operator")
highlight(">", "operator")
highlight("<=", "operator")
highlight(">=", "operator")
highlight("==", "operator")
highlight("~=", "operator")

highlight("if", "reserved")
highlight("else", "reserved")
highlight("elseif", "reserved")
highlight("and", "reserved")
highlight("repeat", "reserved")
highlight("for", "reserved")
highlight("in", "reserved")
highlight("until", "reserved")
highlight("or", "reserved")
highlight("not", "reserved")
highlight("then", "reserved")
highlight("while", "reserved")
highlight("do", "reserved")
highlight("end", "reserved")
highlight("local", "reserved")
highlight("function", "reserved")
highlight("goto", "reserved")

highlight("break", "reserved")
highlight("return",  "reserved")
highlight("pairs", "variable")
highlight("ipairs", "variable")

highlight("{", "reserved")
highlight("}", "reserved")
highlight("[", "reserved")
highlight("]", "reserved")

highlight_region("'","'", "string", false)
highlight_region("\"","\"", "string", false)
highlight_region("[[","]]", "string", false)

highlight_region("--", "", "comments" ,true)
highlight_region("--[[", "]]", "comments" ,false)

-- Utility functions
function trim(s)
    return s:match("^%s*(.-)%s*$")
end

function splitstr(str, sep)
    local t = {}
    for part in string.gmatch(str, "([^" .. sep .. "]+)") do
        table.insert(t, trim(part))
    end
    return t
end

-- Known libraries and their functions
local libraries = {
    math = {
        "abs", "acos", "asin", "atan",
        "atan2", "ceil", "cos", "cosh",
        "deg", "exp", "floor", "fmod",
        "frexp", "huge", "ldexp", "log",
        "log10", "max", "min", "modf",
        "pi", "pow", "rad", "random",
        "randomseed", "sin", "sinh", "sqrt",
        "tan", "tanh"
    },
    table = {
        "concat", "insert", "maxn", "remove",
        "sort"
    },
    string = {
        "upper", "lower", "gsub", "find",
        "reverse", "format", "char", "len",
        "rep", "byte"
    },
    debug = {
        "getfenv", "gethook", "getinfo", "getlocal",
        "getmetatable", "getregistry", "getupvalue", "setfenv",
        "sethook", "setlocal", "setmetatable", "setupvalue",
        "traceback"
    },
    coroutine = {
        "create", "resume", "yield", "status",
        "wrap", "running"
    },
    io = {
        "open", "input", "close", "read",
        "write", "output", "tmpfile", "lines",
        "flush", "type"
    },
    os = {
        "clock", "date", "difftime", "execute",
        "exit", "getenv", "remove", "rename",
        "setlocale", "time", "tmpname"
    }
}

local librariesFunctions = {}
for library, methods in pairs(libraries) do
    for _, method in ipairs(methods) do
        table.insert(librariesFunctions, library .. "." .. method)
    end
end

function detect_functions(content)
    local functionNames = {
        "print", "getfenv", "loadfile", "assert",
        "error", "ipairs", "require", "loadlib",
        "xpcall", "tostring", "rawset", "pairs",
        "loadstring", "pcall", "type", "dofile",
        "wait", "getmetatable", "load", "next",
        "rawget", "select", "tonumber", "unpack",
        "rawequal", "collectgarbage"
    }
    for _, libFunc in ipairs(librariesFunctions) do
        table.insert(functionNames, libFunc)
    end
    for line in content:gmatch("[^\r\n]+") do
        local trimmed = trim(line)
        if (trimmed:find("^function ") or trimmed:find("^local function ")) and trimmed:find("%(") then
            local funcName = trimmed:gsub("then", ""):match("function%s+(.-)%s*%(")
                         or trimmed:gsub("then", ""):match("local%s+function%s+(.-)%s*%(")
                         or trimmed:match("local%s+(.-)%s*%(")
            if funcName then
                table.insert(functionNames, trim(funcName))
            end
        end
    end

    return functionNames
end

local Variables = {
    "_", "_G", "_VERSION"
}
for lib, _ in pairs(libraries) do
    table.insert(Variables, lib)
end

function detect_variables(content)
    local variable_names = {
        "return", "in", "boolean", "number",
        "if", "then", "do", "while",
        "for", "repeat", "until", "break",
        "or", "and", "not", "else", "elseif",
        "false", "true", "nil",
        "userdata", "thread", "end",
        "local", "function"
    }

    for _, var in ipairs(Variables) do
        table.insert(variable_names, var)
    end

    for line in content:gmatch("[^\r\n]+") do
        local trimmed = trim(line)
        if trimmed:find("=") and not trimmed:find("==") then
            trimmed = trimmed:gsub("local", "")
            local parts = splitstr(trimmed, "=")
            if #parts > 0 then
                local lhs = parts[1]
                if lhs:find(",") then
                    for _, var in ipairs(splitstr(lhs, ",")) do
                        table.insert(variable_names, trim(var))
                    end
                else
                    table.insert(variable_names, trim(lhs))
                end
            end
        end
    end

    return variable_names
end
function detect_imports(content)
    local imports = {}

    for mod in content:gmatch("require%s*%(?%s*['\"]([%w%._/-]+)['\"]%s*%)?") do
        table.insert(imports, mod)
    end

    for path in content:gmatch("dofile%s*%(?%s*['\"]([%w%._/-]+%.lua)['\"]%s*%)?") do
        table.insert(imports, path)
    end

    for path in content:gmatch("loadfile%s*%(?%s*['\"]([%w%._/-]+%.lua)['\"]%s*%)?") do
        table.insert(imports, path)
    end

    return imports
end

function get_keywords(content)
    local keywords = {"and", "break", "do", "else", "elseif", "end", "false", "for", "function", "goto", "if", "in", "local", "nil", "not", "or", "repeat", "return", "then", "true", "until", "while"}
    return keywords
end
