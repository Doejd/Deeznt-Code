--- Highlight Keywords
highlight("alignas", "reserved")
highlight("alignof", "reserved")
highlight("and", "reserved")
highlight("and_eq", "reserved")
highlight("asm", "reserved")
highlight("atomic_cancel", "reserved")
highlight("atomic_commit", "reserved")
highlight("atomic_noexcept", "reserved")
highlight("auto", "reserved")
highlight("bitand", "reserved")
highlight("bitor", "reserved")
highlight("bool", "reserved")
highlight("break", "reserved")
highlight("case", "reserved")
highlight("catch", "reserved")
highlight("char", "reserved")
highlight("char8_t", "reserved")
highlight("char16_t", "reserved")
highlight("char32_t", "reserved")
highlight("class", "reserved")
highlight("compl", "reserved")
highlight("concept", "reserved")
highlight("const", "reserved")
highlight("consteval", "reserved")
highlight("constexpr", "reserved")
highlight("constinit", "reserved")
highlight("const_cast", "reserved")
highlight("continue", "reserved")
highlight("co_await", "reserved")
highlight("co_return", "reserved")
highlight("co_yield", "reserved")
highlight("decltype", "reserved")
highlight("default", "reserved")
highlight("delete", "reserved")
highlight("do", "reserved")
highlight("double", "reserved")
highlight("dynamic_cast", "reserved")
highlight("else", "reserved")
highlight("enum", "reserved")
highlight("explicit", "reserved")
highlight("export", "reserved")
highlight("extern", "reserved")
highlight("false", "reserved")
highlight("float", "reserved")
highlight("for", "reserved")
highlight("friend", "reserved")
highlight("goto", "reserved")
highlight("if", "reserved")
highlight("include", "reserved")
highlight("inline", "reserved")
highlight("int", "reserved")
highlight("long", "reserved")
highlight("module", "reserved")
highlight("mutable", "reserved")
highlight("namespace", "reserved")
highlight("new", "reserved")
highlight("noexcept", "reserved")
highlight("not", "reserved")
highlight("not_eq", "reserved")
highlight("nullptr", "reserved")
highlight("operator", "reserved")
highlight("or", "reserved")
highlight("or_eq", "reserved")
highlight("private", "reserved")
highlight("protected", "reserved")
highlight("public", "reserved")
highlight("reflexpr", "reserved")
highlight("register", "reserved")
highlight("reinterpret_cast", "reserved")
highlight("requires", "reserved")
highlight("return", "reserved")
highlight("short", "reserved")
highlight("signed", "reserved")
highlight("sizeof", "reserved")
highlight("static", "reserved")
highlight("static_assert", "reserved")
highlight("static_cast", "reserved")
highlight("string", "reserved")
highlight("struct", "reserved")
highlight("switch", "reserved")
highlight("synchronized", "reserved")
highlight("template", "reserved")
highlight("this", "reserved")
highlight("thread_local", "reserved")
highlight("throw", "reserved")
highlight("true", "reserved")
highlight("try", "reserved")
highlight("typedef", "reserved")
highlight("typeid", "reserved")
highlight("typename", "reserved")
highlight("union", "reserved")
highlight("unsigned", "reserved")
highlight("using", "reserved")
highlight("virtual", "reserved")
highlight("void", "reserved")
highlight("volatile", "reserved")
highlight("wchar_t", "reserved")
highlight("while", "reserved")
highlight("xor", "reserved")
highlight("xor_eq", "reserved")

--- Arithmetic Operators
highlight("+", "operator")
highlight("-", "operator")
highlight("*", "operator")
highlight("/", "operator")
highlight("%", "operator")
highlight("**", "operator")
highlight("++", "operator")
highlight("--", "operator")

--- Assignment Operators
highlight("=", "operator")
highlight("+=", "operator")
highlight("-=", "operator")
highlight("*=", "operator")
highlight("/=", "operator")
highlight("%=", "operator")

--- Comparison Operators
highlight("==", "operator")
highlight("!=", "operator")
highlight(">", "operator")
highlight("<", "operator")
highlight(">=", "operator")
highlight("<=", "operator")

--- Logical Operators
highlight("&&", "operator")
highlight("||", "operator")
highlight("!", "operator")

--- Bitwise Operators
highlight("&", "operator")
highlight("|", "operator")
highlight("^", "operator")
highlight("~", "operator")
highlight("<<", "operator")
highlight(">>", "operator")

--- Special Characters
highlight("{", "binary")
highlight("}", "binary")
highlight("[", "binary")
highlight("]", "binary")
highlight("(", "binary")
highlight(")", "binary")
highlight(";", "binary")
highlight(",", "binary")

--- Strings
highlight_region("\"", "\"", "string")
highlight_region("'", "'", "string")

--- Comments
highlight_region("//", "", "comments", true)
highlight_region("/*", "*/", "comments", false)

function detect_functions(content)
    local function_names = {}

    for line in content:gmatch("[^\r\n]+") do
        local name = line:match("^%s*[%w_:<>*&]+%s+([%w_:~]+)%s*%b()")
        if name then
            table.insert(function_names, name)
        end
    end

    return function_names
end


function detect_variables(content)
    local variable_names = {}

    for line in content:gmatch("[^\r\n]+") do
        if not line:find("%b()") then
            local name = line:match("^[ \t]*[%w_:<>,*&%[%] \t]-[ \t]+([%a_][%w_]*)[ \t]*[=;{]")
            if name then
                table.insert(variable_names, name)
            end
        end
    end
    return variable_names
end

function detect_imports(content)
    local import_names = {}
    for line in content:gmatch("[^\r\n]+") do
        local import_lib = line:match('#include%s*<%s*([^%s>]+)%s*>')
        if not import_lib then
            import_lib = line:match('#include%s*"([^"]+)"')
        end
        if import_lib then
            table.insert(import_names, import_lib)
        end
    end
    return import_names
end

function get_keywords(content)
    local keywords = {
        "alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel", "atomic_commit", "atomic_noexcept",
        "auto", "bitand", "bitor", "bool", "break", "case", "catch", "char", "char8_t", "char16_t", "char32_t",
        "class", "compl", "concept", "const", "consteval", "constexpr", "constinit", "const_cast", "continue",
        "co_await", "co_return", "co_yield", "decltype", "default", "delete", "do", "double", "dynamic_cast",
        "else", "enum", "explicit", "export", "extern", "false", "float", "for", "friend", "goto", "if",
        "include", "inline", "int", "long", "module", "mutable", "namespace", "new", "noexcept", "not", "not_eq",
        "nullptr", "operator", "or", "or_eq", "private", "protected", "public", "reflexpr", "register",
        "reinterpret_cast", "requires", "return", "short", "signed", "sizeof", "static", "static_assert",
        "static_cast", "string", "struct", "switch", "synchronized", "template", "this", "thread_local", "throw",
        "true", "try", "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual", "void",
        "volatile", "wchar_t", "while", "xor", "xor_eq"
    }
    return keywords
end
