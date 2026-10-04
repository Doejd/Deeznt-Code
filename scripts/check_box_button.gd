extends CheckBox
@export var modifies_property : String = ""
@onready var editor : CodeEdit = $"../../../../../Editor_Container".find_child("Editor")
@onready var control : Control = $"../../../../.."

func _ready() -> void:
	control.connect("emit_setting", on_emit_setting)
	connect("pressed", _on_pressed)
	
func _on_pressed() -> void:
	if modifies_property == "show_intro_wind":
		control.intro_wind_popup = button_pressed
		control.SettingManager.editor_setting_map[modifies_property] = button_pressed
		return
	elif modifies_property == "open_last_project_on_startup":
		control.open_last_project_on_startup = button_pressed
		control.SettingManager.editor_setting_map[modifies_property] = button_pressed
		return
	elif modifies_property == "include_hidden":
		control.dir.include_hidden = button_pressed
		control.SettingManager.preference_setting_map[modifies_property] = button_pressed
		return
	var prop = editor.get(modifies_property)
	if prop == null: return
	editor.set(modifies_property, button_pressed)
	control.SettingManager.editor_setting_map[modifies_property] = button_pressed

func on_emit_setting(_should_load_last_project : Variant):
	var property = editor.get(modifies_property)
	if property == null: property = false
	if not control.SettingManager.editor_setting_map.has(modifies_property): return
	button_pressed = control.SettingManager.editor_setting_map[modifies_property]
	editor.set(modifies_property, button_pressed)
