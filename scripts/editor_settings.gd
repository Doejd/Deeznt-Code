extends VBoxContainer
@onready var control : Control = $"../../.."
@onready var editor : CodeEdit = $"../../../Editor_Container".find_child("Editor")
var theme_nodes = []

func _ready() -> void:
	var setting_window = $"../.."
	setting_window.connect("close_requested", setting_window.hide)

func _on_general_button_pressed() -> void:
	for pannel in range(get_child_count()): get_child(pannel).visible = false
	find_child("General Settings", false).visible = true

func _on_editor_button_pressed() -> void:
	for pannel in range(get_child_count()): get_child(pannel).visible = false
	find_child("Editor Settings", false).visible = true

func _on_themes_button_pressed() -> void:
	for pannel in range(get_child_count()): get_child(pannel).visible = false
	find_child("Theme Settings", false).visible = true

func _on_advanced_button_pressed() -> void:
	for pannel in range(get_child_count()): get_child(pannel).visible = false
	find_child("Advanced Settings", false).visible = true

func _on_reset_to_defaults_button_pressed() -> void:
	control.SettingManager.preference_setting_map = control.SettingManager.default_preference_setting_map
	control.SettingManager.editor_setting_map = control.SettingManager.default_editor_setting_map
	control.SettingManager.timer_map = control.SettingManager.default_timer_map
	control.on_load_emit_pref()
