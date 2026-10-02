extends Button
@onready var theme_name_input : LineEdit = $"../../ThemeNameInput/LineEdit"
var color_buttons = []
var set_keywords_list = ["reserved", "string", "number", "symbol", "variable", "comments", "error", "function", "member", "import"]


func _ready() -> void:
	for button in $"../..".find_children("*", "ColorPickerButton", true):
		color_buttons.append(button)

func _on_pressed() -> void:
	var Lua_theme_dir : DirAccess = DirAccess.open("user://Lua/themes")
	var theme_name = ""
	if Lua_theme_dir == null: return
	if theme_name_input.text.strip_edges() == "": theme_name = theme_name_input.placeholder_text
	else: theme_name = theme_name_input.text
	var file = FileAccess.open(Lua_theme_dir.get_current_dir() + "/%s.lua" % theme_name, FileAccess.WRITE)
	for button in color_buttons:
		if button.key_referring_to in set_keywords_list: file.store_string('set_keywords("%s", "%s")' % [button.key_referring_to, button.color.to_html(false)])
		else: file.store_string('set_gui("%s", "%s")' % [button.key_referring_to, button.color.to_html(false)])
