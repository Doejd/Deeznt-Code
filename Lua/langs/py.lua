--- Highligth Keywords
highlight("import", "reserved")
highlight("from", "reserved")
highlight("while", "reserved")
highlight("if", "reserved")
highlight("else", "reserved")
highlight("elif", "reserved")
highlight("as", "reserved")
highlight("class", "reserved")
highlight("return", "reserved")
highlight("def", "reserved")
highlight("for", "reserved")
highlight("pass", "reserved")
highlight("try", "reserved")
highlight("except", "reserved")
highlight("yield", "reserved")
highlight("assert", "reserved")
highlight("continue", "reserved")
highlight("False", "reserved")
highlight("True", "reserved")
highlight("None", "reserved")
highlight("global", "reserved")
highlight("local", "reserved")

--- Logical Operators
highlight("and", "reserved")
highlight("or", "reserved")
highlight("not", "reserved")

--- Membership Operators
highlight("in", "reserved")

--- Special Characters
highlight("(", "symbol")
highlight("(", "symbol")
highlight("{", "symbol")
highlight("}", "symbol")
highlight("[", "symbol")
highlight("]", "symbol")


-- Decorators
highlight_region("@", "", "symbol", true)

--- Strings
highlight_region("'", "'", "string")
highlight_region('"', '"', "string")
highlight_region('"""', '"""', "string")

--- User Comments
highlight_region("#", "", "comments", true)


function detect_functions(content)
    local functionNames = {}

    for line in content:gmatch("[^\r\n]+") do
        local functionName = line:match("def%s+([%w_]+)%s*%(")
        if functionName then
            table.insert(functionNames, functionName)
        end
        local asyncFunctionName = line:match("async%s+def%s+([%w_]+)%s*%(")
        if asyncFunctionName then
            table.insert(functionNames, asyncFunctionName)
        end
    end

    return functionNames
end

function detect_variables(content)
    local variable_names = {"__name__", "__annotations__", "__build_class__", "__builtins__", 
    "__cached__", "__dict__", "__doc__", "__file__", "__import__", "__loader__", "__name__", "__package__", "__path__", "__spec__"}
    for line in content:gmatch("[^\r\n]+") do
        -- Handle tuple unpacking assignments like: foo, bar = 800, 600
        local unpack_vars = line:match("^%s*([%w_%s,]+)%s*=%s*.+")
        if unpack_vars and unpack_vars:find(",") then
            for var in unpack_vars:gmatch("([%w_]+)") do
                table.insert(variable_names, var)
            end
        else
            -- Handle single variable assignment
            local assignment = line:match("^%s*([%w_%.]+)%s*=%s*[^=].*")
            if assignment then
                table.insert(variable_names, assignment)
            end
        end
    end
    return variable_names
end

function detect_imports(content)
    local import_names = {}
    for line in content:gmatch("[^\r\n]+") do
        local import_as = line:match("^%s*import%s+([%w_%.]+)%s+as%s+([%w_]+)")
        if import_as then
            -- Only add the alias, not the original name
            local _, alias = line:match("^%s*import%s+([%w_%.]+)%s+as%s+([%w_]+)")
            table.insert(import_names, alias)
        else
            local import_simple = line:match("^%s*import%s+([%w_,%s]+)")
            if import_simple then
                for name in import_simple:gmatch("([%w_]+)") do
                    table.insert(import_names, name)
                end
            end
        end
        local from_import_as = line:match("^%s*from%s+[%w_%.]+%s+import%s+([%w_%.]+)%s+as%s+([%w_]+)")
        if from_import_as then
            -- Only add the alias, not the original name
            local _, alias = line:match("^%s*from%s+[%w_%.]+%s+import%s+([%w_%.]+)%s+as%s+([%w_]+)")
            table.insert(import_names, alias)
        else
            local from_import_multi = line:match("^%s*from%s+[%w_%.]+%s+import%s+([%w_,%s]+)")
            if from_import_multi then
                for name in from_import_multi:gmatch("([%w_]+)") do
                    table.insert(import_names, name)
                end
                -- Handle "from foo import bar, foobar as fizz, buzz"
                -- Split by comma, then check for "as"
                for item in from_import_multi:gmatch("([^,]+)") do
                    local name, alias = item:match("^%s*([%w_]+)%s+as%s+([%w_]+)%s*$")
                    if alias then
                        table.insert(import_names, alias)
                    else
                        local simple_name = item:match("^%s*([%w_]+)%s*$")
                        if simple_name then
                            table.insert(import_names, simple_name)
                        end
                    end
                end
            end
        end

        local from_import = line:match("^%s*from%s+[%w_%.]+%s+import%s+([%w_]+)")
        if from_import then
            table.insert(import_names, from_import)
        end
    end
    return import_names
end

function get_keywords(content)
    local keywords = {
        "False", "None", "True", "and", "as", "assert", "async", "await",
        "break", "class", "continue", "def", "del", "elif", "else", "except", 
        "finally", "for", "from", "global", "if", "import", "in", "is", "lambda", 
        "nonlocal", "not", "or", "pass", "raise", "return", "try", "while", "with", "yield"
    }
    return keywords
end
