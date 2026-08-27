#define _CRT_SECURE_NO__WARNINGS
#define WIN32_LEAN_AND_MEAN

#include "main.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_action.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <winbase.h>
#include <thread>
#include <string>
#include <regex>
#include <cctype>

#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/file_access.hpp"

godot::Dictionary AnsiHighlighter::_get_line_syntax_highlighting(const int line) const{
    godot::Dictionary res;

    const auto host = cast_to<WindowsHost>(get_text_edit());
    if (!host) return res;

    const auto segments_per_line = host->getSegments();
    if (segments_per_line.empty()) return res;

    if (line < 0 || line >= segments_per_line.size()) return res;

    for (const auto &seg : segments_per_line[line]) {
        const int start_col = seg.starting_column;
        const int end_col = start_col + static_cast<int32_t>(seg.text.length());

        godot::Dictionary style;
        style["color"] = godot::Color::hex(seg.color << 8 | 0xFF);

        for (int col = start_col; col < end_col; ++col) res[static_cast<godot::Variant>(col)] = style;
    }

    return res;
}

void AnsiHighlighter::_bind_methods() {}

bool WindowsHost::fileExists(const char *path) {
    if(INVALID_FILE_ATTRIBUTES == GetFileAttributes(path)) return false;
    return true;
}

void WindowsHost::loadHistory(const uint32_t &max_lines) {
    const char* hist_path = getenv("HISTFILE");
    godot::String path;
    if (!hist_path) {
        const char* home_path = getenv("HOME");
        if (!home_path) return;
        path = godot::String(home_path) + "/.bash_history";
    }
    else path = hist_path;
    if (!fileExists(path.utf8().get_data())) return;

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

void WindowsHost::bulkRemove(const int32_t &to_line) {
    if (to_line <= 0) return;
    const int32_t count = std::min(to_line, static_cast<int32_t>(segments.size()));
    segments.erase(segments.begin(), segments.begin() + count);
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
    if (code >= 16 && code <= 231){
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
            seg = Segment();
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
        case 47: seg.bg_color = ansiToColor(code - 40); break;

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
        case 107: seg.bg_color = ansiToColor(code - 100 + 8); break;

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
                else seg.bg_color = rgb;

                i += 3;
                continue;
            }
            if (mode == 2){
                const int r = static_cast<int>(params[i+2].to_int());
                const int g = static_cast<int>(params[i+3].to_int());
                const int b = static_cast<int>(params[i+4].to_int());

                if (is_fg) seg.color = r << 16 | g << 8 | b;
                else seg.bg_color = r << 16 | g << 8 | b;

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

void WindowsHost::getHighlighting(const godot::String &ansi_string, godot::String &frame_text){
    godot::String cur_args;
    ParseState parse_state = ParseState::Normal;
    int32_t line{get_line_count() - 1};
    for (int i{0}; i < ansi_string.length(); i++) {
        const auto ch = ansi_string[i];
        if (parse_state == ParseState::Normal) {
            if (ch == '\e') {
                if (!current.text.is_empty()) {
                    if (segments.size() <= line) segments.emplace_back();
                    segments[line].push_back(current);
                    frame_text += current.text;
                    current.starting_column += static_cast<int32_t>(current.text.length());
                    current.text = "";
                }
                parse_state = ParseState::Escape;
                continue;
            }
            if (ch == '\n') {
                if (!current.text.is_empty()) {
                    if (segments.size() <= line) segments.emplace_back();
                    segments[line].push_back(current);
                    frame_text += current.text;
                    current.text = "";
                }
                frame_text += '\n';
                line++;
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
            else if (ch != '\n') cur_args += ch;
        }
    }
    if (!current.text.is_empty()) {
        if (segments.size() <= line) segments.emplace_back();
        segments[line].push_back(current);
        frame_text += current.text;
        current.text = "";
    }
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
    end_pseudoconsole_session();
}

void WindowsHost::_process(double p_delta) {
    if (godot::Engine::get_singleton()->is_editor_hint()) return;

    readFromTerminal();

    godot::String frame_text{""};

    getHighlighting(leftoverRead, frame_text);

    leftoverRead = "";

    if (frame_text.is_empty()) return;

    if (const int excess = get_line_count() - TOTAL_MAX_LINES; excess > 0) {
        remove_text(0, 0, excess, static_cast<int32_t>(get_line(excess).length()));
        bulkRemove(excess);
        highlighter->clear_highlighting_cache();
        center_viewport_to_caret();
    }

    set_caret_line(get_line_count() - 1);
    set_caret_column(static_cast<int32_t>(get_line(get_line_count() - 1).length()));
    insert_text_at_caret(frame_text);
    input_start_line_col = {get_line_count() - 1, static_cast<int32_t>(get_line(get_line_count() - 1).length())};
    queue_redraw();
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
        godot::UtilityFunctions::print("Failed to allocate memory, HeapAlloc() -> Failed");
        return;
    }
    if (!InitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &attrSize)){
        UtilityFunctions::print("InitializeProcThreadAttributeList() -> Failed");
        return;
    }
    if(!UpdateProcThreadAttribute(si.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, hPC, sizeof(hPC), NULL, NULL)){
        godot::UtilityFunctions::print("UpdateProcThreadAttribute() -> Failed");
    }
    if(!CreateProcessW(
        L"c:\\Windows\\System32\\cmd.exe",
        NULL,
        NULL, NULL,
        TRUE,
        EXTENDED_STARTUPINFO_PRESENT,
        NULL,
        NULL,
        &si.StartupInfo,
        &pi)){
            godot::UtilityFunctions::print("CreateProcessW() -> Failed");
            return;
    }
    CloseHandle(child_stdin_read);
    CloseHandle(child_stdout_write);
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
// TODO: Implement readFromTerminal reading up to 32KB
void WindowsHost::readFromTerminal(){
    if (parent_stdout_read == NULL) return;
    DWORD bytes_available = 0;
    if (!PeekNamedPipe(parent_stdout_read, NULL, 0, NULL, &bytes_available, NULL)) return;
    if (bytes_available == 0) return;

    CHAR buf[4097];
    DWORD read = 0;

    BOOL success = ReadFile(parent_stdout_read, buf, sizeof(buf) - 1, &read, NULL);
    if (!success || read == 0) return;

    buf[read] = '\0';

    leftoverRead += godot::String::utf8(buf);
}

// TODO: _gui_input needs to be reworked with CTRL + C
void WindowsHost::_gui_input(const godot::Ref<godot::InputEvent> &event) {
    const godot::Ref<godot::InputEventKey> key_event = event;
    if (event->is_class("InputEventMouseButton") || event->is_class("InputEventMouseMotion")) clampCaret();
    if (!key_event.is_valid() || !key_event->is_pressed()) return;
    const int keycode = key_event->get_keycode();
    if (keycode == godot::KEY_LEFT || keycode == godot::KEY_PAGEUP || keycode == godot::KEY_HOME) {
        clampCaret();
        return;
    }
    if (keycode == godot::KEY_ENTER) {
        if (!input.strip_edges().is_empty()) history.push_back(input); history_index = static_cast<int32_t>(history.size());
        const Vector2i line_col = highlighter->from_index_get_line_column(input_start_index);
        remove_text(line_col.x, line_col.y, get_line_count() - 1, static_cast<int32_t>(get_line(get_line_count() - 1).length()));
        write_to_pwsh(input);
        input = "";
        accept_event();
        return;
    }
    if (keycode == KEY_BACKSPACE) {
        if (const int caret_index = get_caret_index(); caret_index > input_start_index) {
            const int rel = caret_index - input_start_index;
            input = input.substr(0, rel-1) + input.substr(rel + 1);
            backspace();
        }
        else clamp_caret();
        accept_event();
        return;
    }
    if (keycode == KEY_UP) {
        if (history.is_empty()) { accept_event(); return; }
        if (history_index == history.size()) history_temp = input;
        if (history_index > 0) history_index--;
        input = history[history_index];
        const Vector2i line_col = highlighter->from_index_get_line_column(input_start_index);
        remove_text(line_col.x, line_col.y, get_line_count() - 1, get_line(get_line_count() - 1).length());
        insert_text(input, line_col.x, line_col.y);
        accept_event();
        return;
    }
    if (keycode == KEY_DOWN) {
        if (history_index < history.size()) history_index++;
        if (history_index == history.size()) input = history_temp;
        else input = history[history_index];
        const Vector2i line_col = highlighter->from_index_get_line_column(input_start_index);
        remove_text(line_col.x, line_col.y, get_line_count() - 1, get_line(get_line_count() - 1).length());
        insert_text(input, line_col.x, line_col.y);
        accept_event();
        return;
    }
    if (!key_event->is_ctrl_pressed() && !key_event->is_alt_pressed()) {
        if (const char32_t unicode = key_event->get_unicode(); unicode != 0) {
            const int rel = get_caret_index() - input_start_index;
            input = input.substr(0, rel) + String::chr(unicode) + input.substr(rel + 1);
        }
    }
}

void WindowsHost::writeToTerminal(const godot::String &text){
    if (parent_stdin_write == nullptr) return;
    const godot::String full_input = input + godot::String("\r\n");
    const std::string utf8_input = full_input.utf8().get_data();

    if (utf8_input == "cls\r\n") {
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

    if (!success) godot::UtilityFunctions::print("WriteFile() -> Failed");
}

std::deque<godot::Vector<Segment>> WindowsHost::getSegments() const {return segments;}