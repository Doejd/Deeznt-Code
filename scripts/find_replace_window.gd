extends Window
@onready var labels = [$VBoxContainer/Find/Label, $"VBoxContainer/Replace All/Label2", $"VBoxContainer/Replace selected/Label"]
@onready var find_input = $VBoxContainer/Find/HBoxContainer/LineEdit
@onready var replace_selected_input = $"VBoxContainer/Replace selected/LineEdit"
@onready var replace_input = $"VBoxContainer/Replace All/LineEdit2"
@onready var editor = get_node("../Editor_Container").find_child("Editor")
@onready var main = get_node("..")
@onready var buttons = [$"VBoxContainer/Find/HBoxContainer/Up Arrow", $"VBoxContainer/Find/HBoxContainer/Down Arrow"]
var all_matches = []
var cur_selected_match = 0

func _ready() -> void:
	hide()
	connect("close_requested", hide)
	
func find_all_occurances(line_index: int, text_to_find: String, text_to_search: String):
	var start_idx: int = 0
	while (start_idx + text_to_find.length() - 1 < text_to_search.length()):
		if (text_to_search.substr(start_idx, text_to_find.length()) == text_to_find):
			all_matches.append(Vector3i(line_index, start_idx, text_to_find.length()))
		start_idx += 1
			

func _on_line_edit_text_submitted(new_text: String) -> void:
	all_matches.clear()
	cur_selected_match = 0
	var lines = editor.get_text().split("\n", true)
	for line_ind in lines.size():
		find_all_occurances(line_ind, new_text, lines[line_ind])
	if all_matches.is_empty(): return
	editor.grab_focus()
	editor.set_caret_line(all_matches[cur_selected_match].x)
	editor.set_caret_column(all_matches[cur_selected_match].y)
	editor.select(all_matches[0].x, all_matches[0].y, all_matches[0].x, all_matches[0].y + all_matches[0].z, 0)

func _on_line_edit_2_text_submitted(new_text: String) -> void:
	if find_input.get_text() == "": return 
	var full_text = editor.get_text()
	full_text = full_text.replace(find_input.get_text(), new_text)
	editor.set_text(full_text)
	
func _on_line_edit3_text_submitted(new_text: String) -> void:
	if editor.has_selection():
		editor.delete_selection()
		editor.insert_text_at_caret(new_text)
	
func _on_up_arrow_pressed() -> void:
	if all_matches.is_empty(): return
	cur_selected_match -= 1
	cur_selected_match = clamp(cur_selected_match, 0, all_matches.size() - 1)
	editor.grab_focus()
	editor.set_caret_line(all_matches[cur_selected_match].x)
	editor.set_caret_column(all_matches[cur_selected_match].y)
	editor.select(all_matches[cur_selected_match].x, all_matches[cur_selected_match].y, all_matches[cur_selected_match].x, all_matches[cur_selected_match].y + all_matches[cur_selected_match].z, 0)

func _on_down_arrow_pressed() -> void:
	if all_matches.is_empty(): return
	cur_selected_match += 1
	cur_selected_match = clamp(cur_selected_match, 0, all_matches.size() - 1)
	editor.grab_focus()
	editor.set_caret_line(all_matches[cur_selected_match].x)
	editor.set_caret_column(all_matches[cur_selected_match].y)
	editor.select(all_matches[cur_selected_match].x, all_matches[cur_selected_match].y, all_matches[cur_selected_match].x, all_matches[cur_selected_match].y + all_matches[cur_selected_match].z, 0)
