#define UNICODE
#ifdef __linux__

#include "main.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_action.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <unistd.h>
#include <pty.h>
#include <utmp.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <string>

void AnsiHighlighter::_bind_methods() {}

godot::Dictionary AnsiHighlighter::_get_line_syntax_highlighting(const int line) const {
    godot::Dictionary res;

    const auto host = cast_to<LinuxHost>(get_text_edit());
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

bool LinuxHost::fileExists(const char *path) {
    struct stat st{};
    return path && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

void LinuxHost::loadHistory(const uint32_t &max_lines) {
    const char* hist_path = getenv("HISTFILE");
    godot::String path;
    if (!hist_path) {
        godot::UtilityFunctions::printerr("HISTFILE environment variable not configured, trying backup");
        const char* home_path = getenv("HOME");
        if (!home_path) {godot::UtilityFunctions::printerr("Could not locate home path (backup) file/history is only local"); return;}
        path = godot::String(home_path) + "/.bash_history";
    }
    else path = hist_path;
    if (!fileExists(path.utf8().get_data())) {godot::UtilityFunctions::printerr("Could not locate history file/history is only local"); return;}

    const godot::Ref<godot::FileAccess> file = godot::FileAccess::open(godot::String(path), godot::FileAccess::READ);
    if (file.is_null()) {godot::UtilityFunctions::printerr("Could not locate history file/history is only local"); return;}

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

bool LinuxHost::clampCaret() {
    if (const int64_t rel = getRelativeCaretIndex(); rel <= 0) {
        set_caret_line(input_start_line_col.x);
        set_caret_column(input_start_line_col.y);
        return true;
    }
    return false;
}

int64_t LinuxHost::getRelativeCaretIndex() const {
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

void LinuxHost::bulkRemove(const int32_t &to_line) {
    if (to_line <= 0) return;
    const int32_t count = std::min(to_line, static_cast<int32_t>(segments.size()));
    segments.erase(segments.begin(), segments.begin() + count);
}

int LinuxHost::ansiToColor(const int &code) {
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
        default: return 0xffffff;    // case 15 included
    }
}

int LinuxHost::ansi256ToColor(const int &code){
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

void LinuxHost::applyStyle(const int code, Segment &seg){
    switch(code){
        case 0:
            seg.color = 0xffffff;
            seg.bg_color = 0x000000;
            seg.bold = false;
            break;

        case 1: seg.bold = true; break;
        case 22: seg.bold = false; break;

        case 30 ... 37: seg.color = ansiToColor(code - 30); break;
        case 40 ... 47: seg.bg_color = ansiToColor(code - 40); break;
        case 90 ... 97: seg.color = ansiToColor(code - 90 + 8); break;
        case 100 ... 107: seg.bg_color = ansiToColor(code - 100 + 8); break;
        default: ;
    }
}

void LinuxHost::applyArgs(Segment &seg, const godot::String &args) {
    auto params = args.split(";");
    for (ssize_t i = 0; i < params.size();){
        const int code = static_cast<int>(params[i].to_int());

        if (code == 38 || code == 48){
            const bool is_fg = (code == 38);
            const int mode = static_cast<int>(params[i+1].to_int());
            if (mode == 5){
                const int idx = static_cast<int>(params[i+2].to_int());
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

void LinuxHost::pushToSegments(const int32_t &line, godot::String &frame_text) {
    if (!current.text.is_empty()) {
        if (segments.size() <= line) segments.emplace_back();
        segments[line].push_back(current);
        frame_text += current.text;
        current.starting_column += static_cast<int32_t>(current.text.length());
        current.text = "";
    }
}

void LinuxHost::getHighlighting(const godot::String &ansi_string, godot::String &frame_text) {
    godot::String cur_args;
    ParseState parse_state = ParseState::Normal;
    int32_t line{get_line_count() - 1};
    for (int i{0}; i < ansi_string.length(); i++) {
        const auto ch = ansi_string[i];
        if (parse_state == ParseState::Normal) {
            if (ch == '\e') {
                pushToSegments(line, frame_text);
                parse_state = ParseState::Escape;
                continue;
            }
            if (ch == '\n') {
                pushToSegments(line, frame_text);
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
            else if (ch == 'J') {parse_state = ParseState::Normal; segments.clear();}
            else if (ch == 'K' || ch == 'H' || ch == 'f' || ch == 'A' || ch == 'B' || ch == 'C' || ch == 'D' || ch == 'h' || ch == 'l' || ch == 'r') {
                parse_state = ParseState::Normal;
                cur_args = "";
            }
            else if (ch != '\n') cur_args += ch;
        }
    }
    pushToSegments(line, frame_text);
}

void LinuxHost::_bind_methods(){
    godot::ClassDB::bind_method(godot::D_METHOD("endTerminal"), &LinuxHost::endTerminal);
    godot::ClassDB::bind_method(godot::D_METHOD("startTerminal"), &LinuxHost::startTerminal);
    godot::ClassDB::bind_method(godot::D_METHOD("writeToTerminal"), &LinuxHost::writeToTerminal);
}

void LinuxHost::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_WM_CLOSE_REQUEST:
        case NOTIFICATION_PREDELETE:
        case NOTIFICATION_EXIT_TREE:
            endTerminal();
            break;

        default: break;
    }
}

void LinuxHost::startTerminal(){
    if (running) return;
    if (openpty(&master_fd, &slave_fd, nullptr, nullptr, nullptr) == -1) {godot::UtilityFunctions::printerr("Openpty Failed"); return;}
    const int flags = fcntl(master_fd, F_GETFL, 0);
    fcntl(master_fd, F_SETFL, flags | O_NONBLOCK);

    child_pid = fork();
    if (child_pid == -1) {godot::UtilityFunctions::printerr("Fork Failed"); return;}

    if (child_pid == 0){
        close(master_fd);
        if (setsid() == -1) {perror("setsid"); _exit(1);}

        login_tty(slave_fd);

        execlp("bash", "bash", nullptr);

        perror("execlp");
        _exit(1);
    }

    close(slave_fd);
    running = true;

    godot::UtilityFunctions::print("PTY searched, PID: ", child_pid);

    loadHistory(500);
}

void LinuxHost::endTerminal(){
    if (!running) return;

    running = false;

    if (child_pid > 0){
        kill(child_pid, SIGHUP);

        kill(child_pid, SIGKILL);

        waitpid(child_pid, nullptr, WNOHANG);
        child_pid = -1;
    }

    if (master_fd != -1){
        close(master_fd);
        master_fd = -1;
    }
}

void LinuxHost::writeToTerminal(const godot::String &text) {
    const std::string native = text.utf8().get_data();

    if (native == "clear\n") {
        clear();
        segments.clear();
    }

    if (native == "exit\n") {
        endTerminal();
        clear();
        segments.clear();
    }

    if (master_fd != -1) {
        const ssize_t result = write(master_fd, native.c_str(), native.size());
        if (result == -1) perror("write");
    }
}

void LinuxHost::readFromTerminal() {
    if (!running || master_fd == -1) return;

    pollfd pfd{};
    pfd.fd = master_fd;
    pfd.events = POLLIN;

    if (const int ret = poll(&pfd, 1, 0); ret <= 0) return;

    if (pfd.revents & (POLLIN | POLLHUP)) {
        const ssize_t n = read(master_fd, buffer, sizeof(buffer) - 1);

        if (n > 0) leftoverRead += godot::String::utf8(buffer, static_cast<int>(n));
        else if (n == 0 || (n < 0 && errno == EIO)) running = false;
    }
}

void LinuxHost::_ready() {
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

void LinuxHost::_exit_tree() {
    endTerminal();
}

void LinuxHost::_gui_input(const godot::Ref<godot::InputEvent> &event) {
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
        if (master_fd == -1) return;
        const pid_t fg_pgid = tcgetpgrp(master_fd);
        if (fg_pgid > 0) kill(-fg_pgid, SIGINT);

        input = "";
        accept_event();
        return;
    }
    if (keycode == godot::KEY_ENTER) {
        if (!input.strip_edges().is_empty()) history.push_back(input); history_index = static_cast<int32_t>(history.size());
        remove_text(input_start_line_col.x, input_start_line_col.y, get_line_count() - 1, static_cast<int32_t>(get_line(get_line_count() - 1).length()));
        writeToTerminal(input + "\n");
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

void LinuxHost::_process(double p_delta) {
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

void  LinuxHost::_draw() {
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
            if (seg.bg_color == 0x000000) continue;
            const int char_column = godot::Math::max(0 , seg.starting_column);
            const godot::Rect2i rect = get_rect_at_line_column(line, char_column + 1); // get_rect_at_line_column(line, char_column) returns the rect of the previous char because that makes sense
            const godot::Rect2i drawRect{rect.position, godot::Size2i{static_cast<int>(cell_width * (seg.text.length() + 1)), rect.size.height}};
            draw_rect(drawRect, godot::Color::hex(seg.bg_color << 8 | 0xFF));
        }
    }
}

std::deque<godot::Vector<Segment>> LinuxHost::getSegments() const {return segments;}

#endif