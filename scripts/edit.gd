extends CodeEdit
@onready var main = get_node("../../../..")
var function = preload("res://Images/function.png")
var variable = preload("res://Images/variable.png")
var import = preload("res://Images/import.png")
var keyword_img = preload("res://Images/keyword.png")
var lua : LuaAPI = LuaAPI.new()
var lua_theme : LuaAPI = LuaAPI.new()
var keywords_to_highlight: Dictionary = {}
var color_regions_to_highlight: Array = []
var cur_theme_name = ""
var keywords: Dictionary = {
	"reserved":   str_to_clr("ff7ab2"),
	"string":     str_to_clr("ecc48d"),
	"number":     str_to_clr("f78c6c"),
	"symbol":     str_to_clr("89ddff"),
	"variable":   str_to_clr("fcbf6c"),
	"comments":   str_to_clr("5c6370"),
	"error":      str_to_clr("ff5370"),
	"function":   str_to_clr("82aaff"),
	"member":     str_to_clr("c792ea"),
	"import":     str_to_clr("addb67")
}
var GUI : Dictionary = {
	"background_color": str_to_clr("23272e"),
	"current_line_color": str_to_clr("2c313c"),
	"selection_color": str_to_clr("3d4556"),
	"font_color": str_to_clr("e06c75"),
	"word_highlighted_color": str_to_clr("3d4556"),
	"completion_background_color": str_to_clr("1e2227"),
	"completion_selected_color": str_to_clr("2c313a"),
	"caret_color": str_to_clr("528bff")
}

func str_to_clr(string: String) -> Color:
	return Color.from_string(string, "#ff0000")
	
func highlight(kword : String, color : String):
	if not (color in keywords.keys()):
		print("Dumbass")
		return

	keywords_to_highlight[kword] = color
	
func highlight_region(start : String, end : String, color : String, single_line : bool = false):
	if not (color in keywords.keys()):
		print("Lol, that ain't there")
		return
		
	color_regions_to_highlight.append([start, end, color, single_line])

func _ready() -> void:
	lua_theme.bind_libraries(["base", "table", "string"])
	lua_theme.push_variant("set_keywords", set_keywords)
	lua_theme.push_variant("set_gui", set_gui)
	lua.bind_libraries(["base", "table", "string"])
	lua.push_variant("highlight", highlight)
	lua.push_variant("highlight_region", highlight_region)

func set_up_extensions(extension : String):
	keywords_to_highlight.clear()
	color_regions_to_highlight.clear()
	var pairs : Dictionary = auto_brace_completion_pairs
	if extension == "html": pairs["<"] = ">"
	else: pairs.erase("<") 
	auto_brace_completion_pairs = pairs
	var error = lua.do_file("user://Lua/langs/" + extension + ".lua")
	if error is LuaError:
		print("ERROR %d: %s" % [error.type, error.message])
		return
	
func setup_highlighter() -> void:
	var CH: CodeHighlighter = CodeHighlighter.new()
	syntax_highlighter = CH
	CH.number_color = keywords.number
	CH.symbol_color = keywords.symbol
	CH.function_color = keywords.function
	CH.member_variable_color = keywords.member
	var kth = keywords_to_highlight
	var crth = color_regions_to_highlight
	for key in kth:
		CH.add_keyword_color(key, keywords[kth[key]])
	for entry in crth:
		if CH.has_color_region(entry[0]): continue
		CH.add_color_region(entry[0], entry[1], keywords[entry[2]], entry[3])
	
func set_keywords(keyword : String, color : String):
	keywords[keyword] = str_to_clr(color)
	
func set_gui(keyword : String, color : String):
	if not (keyword in GUI.keys()):
		print("Nope, nobody's here, and definitely not this keyword: %s" % keyword)
		return
	
	GUI[keyword] = str_to_clr(color)
	
func setup_cur_theme(cur_theme : String):
	var error = lua_theme.do_file("user://Lua/themes/" + cur_theme + ".lua")
	if error is LuaError:
		print("ERROR %d: %s" % [error.type, error.message])
		return
	
func setup_theme(gui : Dictionary = GUI):
	add_theme_color_override("background_color", gui.background_color)
	add_theme_color_override("current_line_color", gui.current_line_color)
	add_theme_color_override("selection_color", gui.selection_color)
	add_theme_color_override("font_color", gui.font_color)
	add_theme_color_override("word_highlighted_color", gui.word_highlighted_color)
	add_theme_color_override("completion_background_color", gui.completion_background_color)
	add_theme_color_override("completion_selected_color", gui.completion_selected_color)
	add_theme_color_override("caret_color", gui.caret_color)
	$"../../../../ColorRect".color = gui.background_color

func _on_option_button_on_theme_change(cur_theme: Variant) -> void:
	cur_theme_name = cur_theme
	setup_cur_theme(cur_theme)
	
func unique_array(arr: Array) -> Array:
	var out := {}
	for element in arr:
		out[element] = element
	return out.values()
	
func _on_code_completion_requested() -> void:
	var function_names = lua.call_function("detect_functions", [text, get_caret_line(), get_caret_column()])
	var variable_names = lua.call_function("detect_variables", [text, get_caret_line(), get_caret_column()])
	var import_names = lua.call_function("detect_imports", [text, get_caret_line(), get_caret_column()])
	var lang_keywords = lua.call_function("get_keywords", [text, get_caret_line(), get_caret_column()])
	if typeof(function_names) == Variant.Type.TYPE_ARRAY:
		for each in unique_array(function_names):
			add_code_completion_option(CodeEdit.KIND_FUNCTION, each, each+"()", keywords.function, function)
	if typeof(variable_names) == Variant.Type.TYPE_ARRAY:
		for each in unique_array(variable_names):
			add_code_completion_option(CodeEdit.KIND_VARIABLE, each, each, keywords.variable, variable)
	if typeof(import_names) == Variant.Type.TYPE_ARRAY:
		for each in unique_array(import_names):
			add_code_completion_option(CodeEdit.KIND_PLAIN_TEXT, each, each, keywords.import, import)
	if typeof(lang_keywords) == Variant.Type.TYPE_ARRAY:
		for each in unique_array(lang_keywords):
			add_code_completion_option(CodeEdit.KIND_PLAIN_TEXT, each, each, keywords.reserved, keyword_img)
	update_code_completion_options(true)

func _on_text_changed() -> void:
	if Input.is_action_just_pressed("enter"): return
	call_deferred("_on_code_completion_requested")

func _on_control_on_load_theme(theme_: Variant) -> void:
	setup_cur_theme(theme_)
	setup_highlighter()
