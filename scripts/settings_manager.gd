class_name Settings_Manager
extends Node
var default_preference_setting_map : Dictionary = {
	"font_size" : 16,
	"indent_size" : 4,
	"theme" : "Github Dark",
	"open_tabs" : [],
	"LastOpenPath" : "",
}
var default_editor_setting_map : Dictionary = {
	"open_last_project_on_startup" : true,
	"indent_automatic" : true,
	"indent_use_spaces" : false,
	"auto_brace_completion_enabled" : true,
	"auto_brace_completion_highlight_matching" : true,
	"gutters_draw_line_numbers" : true,
	"highlight_current_line" : true,
	"minimap_draw" : false,
	"show_intro_wind" : true,
	"gutters_draw_breakpoints_gutter" : false
}
var default_timer_map : Dictionary = {
	"save_timer_delay" : 4,
	"reload_timer_delay" : 4
}
var preference_setting_map : Dictionary = {}
var editor_setting_map : Dictionary = {}
var timer_map : Dictionary = {}
var save_file_path = "user://Preferance Data/save_data.cfg"

func _on_startup_load_settings() -> void:
	var dir_ : DirAccess = DirAccess.open("user://")
	if not dir_.dir_exists("Preferance Data"): dir_.make_dir("Preferance Data")
	_on_load_get_preferences()

func _on_load_get_preferences():
	var cfg : ConfigFile = ConfigFile.new()
	var err = cfg.load(save_file_path)
	if err != OK and err != ERR_FILE_NOT_FOUND:
		print("Failed to initialize %s" % err)
		return
	preference_setting_map = get_section_keys_values(cfg, "preferences", default_preference_setting_map)
	editor_setting_map = get_section_keys_values(cfg, "editor_settings", default_editor_setting_map)
	timer_map = get_section_keys_values(cfg, "timers", default_timer_map)

func get_section_keys_values(cfg: ConfigFile, section: String, default_map: Dictionary) -> Dictionary:
	var res := default_map.duplicate(true)
	if not cfg.has_section(section): return res
	for key in cfg.get_section_keys(section): res[key] = cfg.get_value(section, key)
	return res
	
func save_settings():
	var cfg = ConfigFile.new()
	var err = cfg.load(save_file_path)
	if err != OK and err != ERR_FILE_NOT_FOUND:
		print("Failed to initialize %s" % err)
		return
	save_section_keys_values(cfg, "preferences", preference_setting_map)
	save_section_keys_values(cfg, "editor_settings", editor_setting_map)
	save_section_keys_values(cfg, "timers", timer_map)
	cfg.save(save_file_path)
	
func save_section_keys_values(cfg : ConfigFile, section : String, setting_map : Dictionary) -> void:
	for key in setting_map.keys(): 
		cfg.set_value(section, key, setting_map[key])
