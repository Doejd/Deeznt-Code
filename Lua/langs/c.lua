-- Highlight Keywords
highlight("auto", "reserved")
highlight("break", "reserved")
highlight("case", "reserved")
highlight("char", "reserved")
highlight("const", "reserved")
highlight("continue", "reserved")
highlight("default", "reserved")
highlight("do", "reserved")
highlight("double", "reserved")
highlight("else", "reserved")
highlight("enum", "reserved")
highlight("extern", "reserved")
highlight("float", "reserved")
highlight("for", "reserved")
highlight("goto", "reserved")
highlight("if", "reserved")
highlight("inline", "reserved")
highlight("int", "reserved")
highlight("long", "reserved")
highlight("register", "reserved")
highlight("restrict", "reserved")
highlight("return", "reserved")
highlight("short", "reserved")
highlight("signed", "reserved")
highlight("sizeof", "reserved")
highlight("static", "reserved")
highlight("struct", "reserved")
highlight("switch", "reserved")
highlight("typedef", "reserved")
highlight("union", "reserved")
highlight("unsigned", "reserved")
highlight("void", "reserved")
highlight("volatile", "reserved")
highlight("while", "reserved")
highlight("_Alignas", "reserved")
highlight("_Alignof", "reserved")
highlight("_Atomic", "reserved")
highlight("_Bool", "reserved")
highlight("_Complex", "reserved")
highlight("_Generic", "reserved")
highlight("_Imaginary", "reserved")
highlight("_Noreturn", "reserved")
highlight("_Static_assert", "reserved")
highlight("_Thread_local", "reserved")

-- Special Characters
highlight("{", "symbol")
highlight("}", "symbol")
highlight("[", "symbol")
highlight("]", "symbol")
highlight("(", "symbol")
highlight(")", "symbol") 
highlight(";", "symbol")
highlight(",", "symbol")

-- Strings 
highlight_region("\"", "\"", "string")
highlight_region("'", "'", "string")

-- Comments
highlight_region("//", "", "comments", true)
highlight_region("/*", "*/", "comments", false)

-- Autocomplete

function detect_functions(content)
    local functionNames = {}
    
    for line in content:gmatch("[^\r\n]+") do
        -- Match function declarations
        local functionName = line:match("%s*([%w_]+)%s*%([^%)]*%)%s*%{?")
        if functionName then 
            table.insert(functionNames, functionName)
        end
    end
    
    return functionNames
end

function detect_variables(content) 
    local variable_names = {}
    local lines = content:gmatch("[^\r\n]+")
    
    for line in lines do
        -- Match variable declarations
        local variable = line:match("%s*([%w_]+)%s+[%w_]+%s*[=;%(%),]")
      
        if variable then
            table.insert(variable_names, variable)
        end
    end
  
    return variable_names
end

function detect_imports(content)
    local import_names = {}
    for line in content:gmatch("[^\r\n]+") do
        local import_lib = line:match('#include%s*"%s*([^%s>]+)%s*"')
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
    local keywords = {"auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else", "enum", "extern", "float", "for", "goto", "if", "inline", 
    "int", "long", "register", "restrict", "return", "short", "signed", "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while", 
    "_Alignas", "_Alignof", "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary", "_Noreturn", "_Static_assert", "_Thread_local"
}
    return keywords
end
