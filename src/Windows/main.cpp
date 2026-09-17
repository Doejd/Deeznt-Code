#define WIN32_LEAN_AND_MEAN

#include "main.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_action.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <winbase.h>
#include <cstdlib>
#include <string>

void AnsiHighlighter::_bind_methods() {}

godot::Dictionary AnsiHighlighter::_get_line_syntax_highlighting(const int line) const{
    godot::Dictionary res;

    const auto host = cast_to<WindowsHost>(get_text_edit());
    if (!host) return res;

    const auto segments_per_line = host->getSegments();

    if (line < 0) return res;
    for (const auto &seg : segments_per_line[line]) {
        godot::Dictionary style;
        style["color"] = godot::Color::hex(seg.color << 8 | 0xFF);

        res[static_cast<godot::Variant>(seg.starting_column)] = style;
    }

    return res;
}

bool WindowsHost::fileExists(const char *path) {
    if (!path) return false;
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

void WindowsHost::loadHistory(const uint32_t &max_lines) {
    const char* home_path = std::getenv("APPDATA");
    if (!home_path) {godot::UtilityFunctions::printerr("Appdata env variable not configured/history will be local only"); return;}
    godot::String path = godot::String(home_path) + "/Microsoft/Windows/PowerShell/PSReadLine/ConsoleHost_history.txt";
    if (!fileExists(path.utf8().get_data())) {godot::UtilityFunctions::printerr("Could not locate history file/history will be local only"); return;}

    const godot::Ref<godot::FileAccess> file = godot::FileAccess::open(godot::String(path), godot::FileAccess::READ);

    const auto file_size = file->get_length();

    uint64_t pos{file_size};
    int newline_count{0};
    godot::String buffer;

    while (pos > 0 && newline_count <= max_lines) {
        constexpr uint64_t chunk_size{4096};
        const uint64_t read_size = godot::Math::min(chunk_size, pos);
        pos -= read_size;

        file->seek(pos);
        godot::PackedByteArray bytes = file->get_buffer(static_cast<int64_t>(read_size));
        godot::String chunk = bytes.get_string_from_utf8();

        buffer = chunk + buffer;
        newline_count += static_cast<int>(chunk.count("\n"));
    }

    file->close();

    buffer = buffer.replace("\r\n", "\n");

    godot::PackedStringArray lines = buffer.split("\n", false);
    if (lines.size() > max_lines) lines = lines.slice(lines.size() - max_lines, lines.size());

    history = lines;
    history_index = static_cast<int>(history.size());
    history_temp = "";
}

bool WindowsHost::clampCaret() {
    if (const int64_t rel = getRelativeCaretIndex(); rel <= 0) {
        set_caret_line(input_start_line_col.x);
        set_caret_column(input_start_line_col.y);
        return true;
    }
    return false;
}

int64_t WindowsHost::getRelativeCaretIndex() const {
    const int start_line = input_start_line_col.x;
    const int start_col = input_start_line_col.y;

    const int caret_line = get_caret_line();
    const int caret_column = get_caret_column();
    if (caret_line < start_line || (caret_line == start_line && caret_column < start_col)) return -1;

    if (caret_line == start_line) return caret_column - start_col;

    int64_t idx = 0;
    idx += get_line(start_line).length();
    for (int i{start_line+1}; i < caret_line; i++) idx += get_line(i).length();
    idx += caret_column;

    return idx;
}

int WindowsHost::ansiToColor(const int &code) {
    switch (code) {
        case 0: return 0x000000;     // black
        case 1: return 0xff0000;     // red
        case 2: return 0x00ff00;     // green
        case 3: return 0xffff00;     // yellow
        case 4: return 0x0000ff;     // blue
        case 5: return 0xff00ff;     // magenta
        case 6: return 0x00ffff;     // cyan
        case 7: return 0xffffff;     // white
        case 8: return 0x888888;     // bright black / dark gray
        case 9: return 0xff8888;     // bright red
        case 10: return 0x88ff88;    // bright green
        case 11: return 0xffff88;    // bright yellow
        case 12: return 0x8888ff;    // bright blue
        case 13: return 0xff88ff;    // bright magenta
        case 14: return 0x88ffff;    // bright cyan
        default: return 0xffffff;    // default white
    }
}

int WindowsHost::ansi256ToColor(const int &code){
    if (code < 16) return ansiToColor(code);
    if (code <= 231){
        const int idx = code - 16;

        const int r = idx / 36;
        const int g = (idx % 36) / 6;
        const int b = idx % 6;

        const int steps[6] = {0, 95, 135, 175, 215, 255};

        return steps[r] << 16 | steps[g] << 8 | steps[b];

    }
    const int gray = 8 + (code - 232) * 10;
    return gray << 16 | gray << 8 | gray;
}


void WindowsHost::applyStyle(const int code, Segment &seg){
    switch(code){
        case 0:
            seg.bold = false;
            seg.hasBg = false;
            seg.bg_color = 0x000000;
            seg.color = 0xffffff;
            break;

        case 1: seg.bold = true; break;

        case 22: seg.bold = false; break;

        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37: seg.color = ansiToColor(code - 30); break;

        case 40:
        case 41:
        case 42:
        case 43:
        case 44:
        case 45:
        case 46:
        case 47: seg.bg_color = ansiToColor(code - 40); seg.hasBg = true; break;

        case 90:
        case 91:
        case 92:
        case 93:
        case 94:
        case 95:
        case 96:
        case 97: seg.color = ansiToColor(code - 90 + 8); break;

        case 100:
        case 101:
        case 102:
        case 103:
        case 104:
        case 105:
        case 106:
        case 107: seg.bg_color = ansiToColor(code - 100 + 8); seg.hasBg = true; break;

        default: ;
    }
}

void WindowsHost::applyArgs(Segment &seg, const godot::String &args){
    auto params = args.split(";");
    for (int32_t i = 0; i < params.size();){
        const int code = static_cast<int>(params[i].to_int());

        if (code == 38 || code == 48){
            const bool is_fg = (code == 38);
            const int mode = static_cast<int>(params[i+1].to_int());
            if (mode == 5){
                int idx = static_cast<int>(params[i+2].to_int());
                const int rgb = ansi256ToColor(idx);

                if (is_fg) seg.color = rgb;
                else {seg.bg_color = rgb; seg.hasBg = true;}

                i += 3;
                continue;
            }
            if (mode == 2) {
                const int r = static_cast<int>(params[i+2].to_int());
                const int g = static_cast<int>(params[i+3].to_int());
                const int b = static_cast<int>(params[i+4].to_int());

                if (is_fg) seg.color = r << 16 | g << 8 | b;
                else {seg.bg_color = r << 16 | g << 8 | b; seg.hasBg = true;}
                i += 5;
                continue;
            }
            i++;
            continue;
        }
        applyStyle(code, seg);
        i++;
    }
}

void WindowsHost::pushToSegments(godot::String &frame_text) {
    if (current.text.is_empty()) return;
    frame_text += current.text;
    segments.pushSegment(current);
    current.starting_column += static_cast<int32_t>(current.text.length());
    current.text = "";
}

void WindowsHost::getHighlighting(godot::String &ansi_string, godot::String &frame_text){
    ansi_string = ansi_string.replace("\r\n", "\n");
    godot::String cur_args;
    ParseState parse_state = ParseState::Normal;
    for (int i{0}; i < ansi_string.length(); i++) {
        const auto ch = ansi_string[i];
        if (parse_state == ParseState::Normal) {
            if (ch == '\x1b') {
                pushToSegments(frame_text);
                parse_state = ParseState::Escape;
                continue;
            }
            if (ch == '\n' || current.starting_column + current.text.length() >= MAX_COLS) {
                pushToSegments(frame_text);
                frame_text += '\n';
                segments.initNewLine();
                current.starting_column = 0;
                continue;
            }
            current.text += ch;
        }
        else if (parse_state == ParseState::Escape) {
            if (ch == '[') {parse_state = ParseState::CSI; cur_args = "";}
            else parse_state = ParseState::Normal;
        }
        else {
            if (ch == 'm') {applyArgs(current, cur_args); parse_state = ParseState::Normal;}
            else if (ch == 'J') {parse_state = ParseState::Normal; segments.clear(); clear();}
            else if (ch >= '@' && ch <= '~') parse_state = ParseState::Normal;
            else if (ch != '\n') cur_args += ch;
        }
    }
    pushToSegments(frame_text);
}

void WindowsHost::_bind_methods(){
    godot::ClassDB::bind_method(godot::D_METHOD("endTerminal"), &WindowsHost::endTerminal);
    godot::ClassDB::bind_method(godot::D_METHOD("startTerminal"), &WindowsHost::startTerminal);
    godot::ClassDB::bind_method(godot::D_METHOD("writeToTerminal"), &WindowsHost::writeToTerminal);
}

void WindowsHost::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_WM_CLOSE_REQUEST:
        case NOTIFICATION_PREDELETE:
        case NOTIFICATION_EXIT_TREE:
            endTerminal();
            break;

        default: break;
    }
}

void WindowsHost::startTerminal(){
    if(!CreatePipe(&child_stdin_read, &parent_stdin_write, &sa, 0) ||
       !CreatePipe(&parent_stdout_read, &child_stdout_write, &sa, 0)){
        return;
    }
    HRESULT hr = CreatePseudoConsole(size, child_stdin_read, child_stdout_write, 0, &hPC);
    if (FAILED(hr)) {
        return;
    }
    ZeroMemory(&si, sizeof(si));
    si.StartupInfo.cb = sizeof(si);
    InitializeProcThreadAttributeList(NULL, 1, 0, &attrSize);
    si.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, attrSize);
    if (!si.lpAttributeList){
        godot::UtilityFunctions::printerr("Failed to allocate memory, HeapAlloc() -> Failed");
        return;
    }
    if (!InitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &attrSize)){
        godot::UtilityFunctions::printerr("InitializeProcThreadAttributeList() -> Failed");
        return;
    }
    if(!UpdateProcThreadAttribute(si.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, hPC, sizeof(hPC), NULL, NULL)){
        godot::UtilityFunctions::printerr("UpdateProcThreadAttribute() -> Failed");
    }
    if(!CreateProcessW(
        L"C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe",
        NULL,
        NULL, NULL,
        TRUE,
        EXTENDED_STARTUPINFO_PRESENT,
        NULL,
        NULL,
        &si.StartupInfo,
        &pi)){
        godot::UtilityFunctions::printerr("CreateProcessW() -> Failed");
        return;
    }
    CloseHandle(child_stdin_read);
    CloseHandle(child_stdout_write);

    loadHistory(500);
}

void WindowsHost::endTerminal(){
    running = false;
    ClosePseudoConsole(hPC);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    DeleteProcThreadAttributeList(si.lpAttributeList);
    HeapFree(GetProcessHeap(), 0, (LPVOID)si.lpAttributeList);
    CloseHandle(parent_stdin_write);
    CloseHandle(parent_stdout_read);
}

void WindowsHost::writeToTerminal(const godot::String &text){
    if (parent_stdin_write == nullptr) return;
    const godot::String full_input = text + godot::String::chr('\r');
    const std::string utf8_input = full_input.utf8().get_data();

    if (text.to_lower() == "cls" || text.to_lower() == "clear") {
        clear();
        segments.clear();
    }

    if (text.to_lower() == "exit") {
        endTerminal();
        clear();
        segments.clear();
    }

    DWORD written = 0;
    BOOL success = WriteFile(
        parent_stdin_write,
        utf8_input.c_str(),
        (DWORD)utf8_input.size(),
        &written,
        NULL
    );

    if (!success) godot::UtilityFunctions::printerr("WriteFile() -> Failed");
}

godot::String WindowsHost::readFromTerminal(){
    if (parent_stdout_read == NULL) return "";
    DWORD bytes_available = 0;
    if (!PeekNamedPipe(parent_stdout_read, NULL, 0, NULL, &bytes_available, NULL)) return "";
    if (bytes_available == 0) return "";

    DWORD read = 0;

    BOOL success = ReadFile(parent_stdout_read, buf, sizeof(buf) - 1, &read, NULL);
    if (!success || read == 0) return "";

    buf[read] = '\0';

    return godot::String::utf8(buf);
}


void WindowsHost::_ready() {
    if (godot::Engine::get_singleton()->is_editor_hint()) {
        set_process(false);
        return;
    }

    clear();
    set_focus_mode(FOCUS_ALL);
    set_selecting_enabled(false);
    set_emoji_menu_enabled(false);
    set_context_menu_enabled(false);
    set_drag_and_drop_selection_enabled(false);
    set_middle_mouse_paste_enabled(false);
    set_empty_selection_clipboard_enabled(false);
    set_process(true);

    highlighter.instantiate();
    this->set_syntax_highlighter(highlighter);
    font = get_theme_font("font", "TextEdit");

    startTerminal();
}

void WindowsHost::_exit_tree(){
    endTerminal();
}

void WindowsHost::_gui_input(const godot::Ref<godot::InputEvent> &event) {
    const godot::Ref<godot::InputEventKey> key_event = event;
    if (event->is_class("InputEventMouseButton")) clampCaret();
    if (!key_event.is_valid() || !key_event->is_pressed()) return;
    if (!has_focus()) return;
    const int keycode = key_event->get_keycode();
    if (keycode == godot::KEY_LEFT || keycode == godot::KEY_PAGEUP || keycode == godot::KEY_HOME) {
        if (clampCaret()) accept_event();
        return;
    }
    if (keycode == godot::KEY_C && key_event->is_ctrl_pressed()) {
        DWORD written = 0;
        char ctrlC = 0x03;
        WriteFile(parent_stdin_write, &ctrlC, 1, &written, nullptr);

        input = "";
        accept_event();
        return;
    }
    if (keycode == godot::KEY_ENTER) {
        if (!input.strip_edges().is_empty()) history.push_back(input); history_index = static_cast<int32_t>(history.size());
        remove_text(input_start_line_col.x, input_start_line_col.y, get_line_count() - 1, static_cast<int32_t>(get_line(get_line_count() - 1).length()));
        writeToTerminal(input);
        input = "";
        accept_event();
        return;
    }
    if (keycode == godot::KEY_BACKSPACE) {
        if (const int64_t rel = getRelativeCaretIndex(); rel > 0 && rel <= input.length()) {
            input = input.substr(0, rel - 1) + input.substr(rel);
            backspace();
        }
        accept_event();
        return;
    }
    if (keycode == godot::KEY_UP) {
        if (history.is_empty()) {accept_event(); return;}
        if (history_index == history.size()) history_temp = input;
        history_index = std::max(0, history_index - 1);
        input = history[history_index];
        remove_text(input_start_line_col.x, input_start_line_col.y, get_line_count() - 1, static_cast<int32_t>(get_line(get_line_count() - 1).length()));
        insert_text(input,input_start_line_col.x, input_start_line_col.y);
        accept_event();
        return;
    }
    if (keycode == godot::KEY_DOWN) {
        history_index = std::min(static_cast<int32_t>(history.size()) , history_index + 1);
        if (history_index == history.size()) input = history_temp;
        else if (history_index < history.size()) input = history[history_index];
        remove_text(input_start_line_col.x, input_start_line_col.y, get_line_count() - 1, static_cast<int32_t>(get_line(get_line_count() - 1).length()));
        insert_text(input,input_start_line_col.x, input_start_line_col.y);
        accept_event();
        return;
    }
    if (!key_event->is_ctrl_pressed() && !key_event->is_alt_pressed()) {
        const char32_t unicode = key_event->get_unicode();
        if (unicode == 0) return;
        if (const int64_t rel = getRelativeCaretIndex(); rel >= 0 && rel <= input.length()) {
            input = input.substr(0, rel) + godot::String::chr(unicode) + input.substr(rel);
        }
        else accept_event();
    }
}

void WindowsHost::_process(double p_delta) {
    if (godot::Engine::get_singleton()->is_editor_hint()) return;

    auto res = readFromTerminal();

    godot::String frame_text{""};

    getHighlighting(res, frame_text);

    if (frame_text.is_empty()) return;

    set_caret_line(get_line_count() - 1);
    set_caret_column(static_cast<int32_t>(get_line(get_line_count() - 1).length()));
    insert_text_at_caret(frame_text);

    if (const int excess = get_line_count() - static_cast<int>(segments.capacity()); excess > 0) {
        remove_text(0, 0, excess, static_cast<int32_t>(get_line(excess).length()));
        highlighter->clear_highlighting_cache();
        center_viewport_to_caret();
    }

    set_caret_line(get_line_count() - 1);
    set_caret_column(static_cast<int32_t>(get_line(get_line_count() - 1).length()));
    input_start_line_col = {get_line_count() - 1, static_cast<int32_t>(get_line(get_line_count() - 1).length())};
    queue_redraw();
}

void  WindowsHost::_draw() {
    if (godot::Engine::get_singleton()->is_editor_hint()) return;

    if (segments.empty()) return;

    if (font.is_null()) {
        font = get_theme_font("font", "TextEdit");
        return;
    }

    const int first_visible = get_first_visible_line();
    const int last_visible = first_visible + get_visible_line_count();

    const int font_size = get_theme_font_size("font_size", "TextEdit");
    const int cell_width = static_cast<int>(font->get_char_size('W', font_size).x);

    for (int line = first_visible; line < last_visible; line++) {
        if (line >= segments.size()) continue;
        for (const auto &seg : segments[line]) {
            if (!seg.hasBg) continue;
            const int char_column = godot::Math::max(0 , seg.starting_column);
            const godot::Rect2i rect = get_rect_at_line_column(line, char_column + 1); // get_rect_at_line_column(line, char_column) returns the rect of the previous char because that makes sense
            const godot::Rect2i drawRect{rect.position, godot::Size2i{static_cast<int>(cell_width * (seg.text.length() + 1)), rect.size.height}};
            draw_rect(drawRect, godot::Color::hex(seg.bg_color << 8 | 0xFF));
        }
    }
}

LineRingBuffer WindowsHost::getSegments() const {return segments;}
