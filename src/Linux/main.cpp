#define UNICODE
#ifdef __linux__

#include "main.h"
#include "../shared/Color.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_action.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/display_server.hpp>
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

    auto segments_per_line = host->getSegments();

    if (line < 0) return res;
    for (const auto &seg : segments_per_line[line]) {
        godot::Dictionary style;
        style["color"] = godot::Color::hex(seg.color << 8 | 0xFF);

        res[static_cast<godot::Variant>(seg.starting_column)] = style;
    }

    return res;
}

bool LinuxHost::fileExists(const char *path) {
    struct stat st{};
    return path && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

godot::PackedStringArray LinuxHost::readEtcShells() {
    const godot::Ref<godot::FileAccess> etcShells = godot::FileAccess::open("/etc/shells", godot::FileAccess::READ);

    if (etcShells.is_null()) {
        godot::UtilityFunctions::printerr("Could not find /etc/shells file");
        return {};
    }

    const godot::String trustedShells = etcShells->get_as_text();
    godot::PackedStringArray validShells;
    auto lineSplit = trustedShells.split("\n", false);

    for (const auto &line : lineSplit) {
        if (const godot::String ln = line.strip_edges(); !ln.is_empty() && !ln.begins_with("#")) {
            auto fileName = line.get_file().strip_edges();
            const bool isValidShell = fileName.contains("bash") || fileName.contains("zsh") || fileName.contains("pwsh") ||
                                fileName.contains("powershell") || fileName.contains("fish");
            const bool blacklisted = fileName == "sh" || fileName == "dash" || fileName == "rbash";

            if (isValidShell && !blacklisted) validShells.push_back(ln);
        }
    }

    return validShells;
}


godot::String LinuxHost::loadHistoryPath(const std::string &shell_name) {
    godot::String resultPath{""};
    const char* homePath = getenv("HOME");
    if (strstr(shell_name.c_str(), "bash") != nullptr) {
        const char *histPath = getenv("HISTFILE");
        if (histPath != nullptr) {
            godot::UtilityFunctions::print("Found the bash shell history file, path: ", histPath);
            resultPath = godot::String(histPath);
        }
        if (homePath != nullptr) {
            const auto historyPath = godot::String(homePath) + godot::String("/.bash_history");
            godot::UtilityFunctions::print("Backup path for bash shell: ", historyPath);
            resultPath = historyPath;
        }
        else godot::UtilityFunctions::printerr("Could not find bash history file");
    }
    else if (strstr(shell_name.c_str(), "zsh") != nullptr) {
        const char *histPath = getenv("HISTFILE");
        if (histPath != nullptr) {
            godot::UtilityFunctions::print("Found the zsh shell history file, path: ", histPath);
            resultPath = godot::String(histPath);
        }
        if (homePath != nullptr) {
            const auto historyPath = godot::String(homePath) + godot::String("/.zsh_history");
            godot::UtilityFunctions::print("Backup path for zsh shell: ", historyPath);
            resultPath = historyPath;
        }
        else godot::UtilityFunctions::printerr("Could not find zsh history file");
    }
    else if (strstr(shell_name.c_str(), "fish") != nullptr) {
        if (homePath != nullptr) {
            resultPath = "/.local/share/fish/fish_history";
        }
        else godot::UtilityFunctions::printerr("Could not find fish history file");
    }
    else if (strstr(shell_name.c_str(), "pwsh") != nullptr || strstr(shell_name.c_str(), "powershell") != nullptr) {
        if (homePath != nullptr) {
            resultPath = "/.local/share/fish/pwsh_history";
        }
        else godot::UtilityFunctions::printerr("Could not find powershell history file");
    }
    if (!fileExists(resultPath.utf8().get_data())) {
        godot::UtilityFunctions::printerr("Could not find the history file, history is only local");
        return "";
    }
    return resultPath;
}

void LinuxHost::loadHistory(uint32_t max_lines, const std::string &shell_name) {
    auto path = loadHistoryPath(shell_name);
    const godot::Ref<godot::FileAccess> file = godot::FileAccess::open(path, godot::FileAccess::READ);
    if (file.is_null()) {godot::UtilityFunctions::printerr("Could not locate history file/history is only local"); return;}

    const auto file_size = file->get_length();

    uint64_t pos{file_size};
    int newline_count{0};
    godot::String fileTextBuffer;

    while (pos > 0 && newline_count <= max_lines) {
        constexpr uint64_t chunk_size{4096};
        const uint64_t read_size = godot::Math::min(chunk_size, pos);
        pos -= read_size;

        file->seek(pos);
        godot::PackedByteArray bytes = file->get_buffer(static_cast<int64_t>(read_size));
        godot::String chunk = bytes.get_string_from_utf8();

        fileTextBuffer = chunk + fileTextBuffer;
        newline_count += static_cast<int>(chunk.count("\n"));
    }

    file->close();

    godot::PackedStringArray lines = fileTextBuffer.split("\n", false);
    if (lines.size() > max_lines) lines = lines.slice(lines.size() - max_lines, lines.size());

    godot::UtilityFunctions::print("loaded lines: ", lines.size());

    for (const auto &line : lines) history.push_back(line);
    history_index = static_cast<int>(history.size());
    history_temp = "";
}

void LinuxHost::clampCaret() {
    if (const int64_t rel = getRelativeCaretIndex(); rel <= 0) {
        set_caret_line(input_start_line_col.x);
        set_caret_column(input_start_line_col.y);
        accept_event();
    }
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

void LinuxHost::pushToSegments(godot::String &frame_text, godot::String &cur_text) {
    if (cur_text.is_empty()) return;
    frame_text += cur_text;
    current.length = cur_text.length();
    segments.pushSegment(current);
    current.starting_column += static_cast<int32_t>(current.length);
    cur_text = "";
}

void LinuxHost::getHighlighting(const godot::String &ansi_string, godot::String &frame_text) {
    godot::String cur_args;
    godot::String cur_text;
    auto parse_state = ParseState::Normal;
    for (int i{0}; i < ansi_string.length(); i++) {
        const auto ch = ansi_string[i];

        if (ch == '\r') continue;

        if (parse_state == ParseState::Normal) {
            if (ch == '\e') {
                pushToSegments(frame_text, cur_text);
                parse_state = ParseState::Escape;
                continue;
            }
            if (ch == '\n' || current.starting_column + cur_text.length() >= TOTAL_MAX_COLS) {
                pushToSegments(frame_text, cur_text);
                frame_text += '\n';
                segments.initNewLine();
                current.starting_column = 0;
                if (ch == '\n') continue;
            }
            cur_text += ch;
        }

        else if (parse_state == ParseState::Escape) {
            if (ch == '[') {parse_state = ParseState::CSI; cur_args = "";}
            else parse_state = ParseState::Normal;
        }

        else {
            if (ch == 'm') {
                AnsiParseHelpers::parseAndApplyArgs(current, cur_args);
                parse_state = ParseState::Normal;
            }
            else if (ch == 'J') {
                const int code = cur_args.is_empty() ? 0 : static_cast<int>(cur_args.to_int());
                if (code == 2 || code == 3) {
                    frame_text = "";
                    cur_text = "";
                    segments.clear();
                    clear();
                    current.starting_column = 0;
                }

                parse_state = ParseState::Normal;
            }
            else if (ch >= '@' && ch <= '~') parse_state = ParseState::Normal;
            else if (ch != '\n') cur_args += ch;
        }
    }
    pushToSegments(frame_text, cur_text);
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

void LinuxHost::startTerminal() {
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

    loadHistory(500, "bash");
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

    if (native == "exit\n") {
        endTerminal();
        segments.clear();
        clear();
        return;
    }

    if (master_fd != -1) {
        const ssize_t result = write(master_fd, native.c_str(), native.size());
        if (result == -1) perror("write");
    }
}

godot::String LinuxHost::readFromTerminal() {
    if (!running || master_fd == -1) return "";

    godot::String res;

    pollfd pfd{};
    pfd.fd = master_fd;
    pfd.events = POLLIN;

    if (const int ret = poll(&pfd, 1, 0); ret <= 0) return "";

    if (pfd.revents & (POLLIN | POLLHUP)) {
        const ssize_t n = read(master_fd, buffer, sizeof(buffer) - 1);

        if (n > 0) res = godot::String::utf8(buffer, static_cast<int>(n));
        else if (n == 0 || (n < 0 && errno == EIO)) running = false;
    }
    return res;
}

void LinuxHost::_ready() {
    if (godot::Engine::get_singleton()->is_editor_hint()) {
        set_process(false);
        return;
    }

    clear();
    set_focus_mode(FOCUS_ALL);
    set_selecting_enabled(true);
    set_context_menu_enabled(true);
    set_emoji_menu_enabled(false);
    set_shortcut_keys_enabled(false);
    set_drag_and_drop_selection_enabled(false);
    set_middle_mouse_paste_enabled(false);
    set_empty_selection_clipboard_enabled(false);
    set_process(true);

    auto menu = get_menu();

    menu->set_item_count(menu->get_item_index(MENU_SELECT_ALL) + 1);
    menu->remove_item(MENU_CUT);

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
    if (!key_event.is_valid() || !key_event->is_pressed()) return;
    if (!has_focus()) return;
    const int keycode = key_event->get_keycode();
    if (key_event->is_ctrl_pressed() || key_event->is_alt_pressed()) {
        clampCaret();
        if (has_selection()) deselect();
    }
    if (keycode == godot::KEY_LEFT || keycode == godot::KEY_PAGEUP || keycode == godot::KEY_HOME) {
        clampCaret();
        return;
    }
    if (keycode == godot::KEY_C && key_event->is_ctrl_pressed()) {
        if (has_selection()) deselect();
        if (master_fd == -1) return;
        const pid_t fg_pgid = tcgetpgrp(master_fd);
        if (fg_pgid > 0) kill(-fg_pgid, SIGINT);

        input = "";
        clampCaret();
        return;
    }
    if (keycode == godot::KEY_ENTER) {
        if (has_selection()) deselect();
        if (!input.strip_edges().is_empty()) {
            history.push_back(input);
            history_index = static_cast<int32_t>(history.size());
        }
        remove_text(input_start_line_col.x, input_start_line_col.y, get_line_count() - 1, static_cast<int32_t>(get_line(get_line_count() - 1).length()));
        writeToTerminal(input + "\n");
        input = "";
        clampCaret();
        return;
    }
    if (keycode == godot::KEY_UP) {
        if (has_selection()) deselect();
        if (history.is_empty()) {accept_event(); return;}
        if (history_index == history.size()) history_temp = input;
        history_index = std::max(0, history_index - 1);
        input = history[history_index];
        remove_text(input_start_line_col.x, input_start_line_col.y, get_line_count() - 1, static_cast<int32_t>(get_line(get_line_count() - 1).length()));
        insert_text(input,input_start_line_col.x, input_start_line_col.y);
        clampCaret();
        accept_event();
        return;
    }
    if (keycode == godot::KEY_DOWN) {
        if (has_selection()) deselect();
        history_index = std::min(static_cast<int32_t>(history.size()) , history_index + 1);
        if (history_index == history.size()) input = history_temp;
        else if (history_index < history.size()) input = history[history_index];
        remove_text(input_start_line_col.x, input_start_line_col.y, get_line_count() - 1, static_cast<int32_t>(get_line(get_line_count() - 1).length()));
        insert_text(input,input_start_line_col.x, input_start_line_col.y);
        clampCaret();
        accept_event();
        return;
    }
    if (keycode == godot::KEY_BACKSPACE) {
        if (const int64_t rel = getRelativeCaretIndex(); rel > 0 && rel <= input.length()) {
            input = input.substr(0, rel - 1) + input.substr(rel);
        }
        else {
            clampCaret();
            if (has_selection()) deselect();
        }
    }
    if (!key_event->is_ctrl_pressed() && !key_event->is_alt_pressed()) {
        const char32_t unicode = key_event->get_unicode();
        if (unicode == 0) return;
        if (const int64_t rel = getRelativeCaretIndex(); rel >= 0 && rel <= input.length()) {
            input = input.substr(0, rel) + godot::String::chr(unicode) + input.substr(rel);
        }
        else {
            clampCaret();
            if (has_selection()) deselect();
        }
    }
}

void LinuxHost::_process(double p_delta) {
    if (godot::Engine::get_singleton()->is_editor_hint()) return;

    const auto res = readFromTerminal();

    godot::String frame_text{""};

    getHighlighting(res, frame_text);

    if (frame_text.is_empty()) return;

    set_caret_line(get_line_count() - 1);
    set_caret_column(static_cast<int32_t>(get_line(get_line_count() - 1).length()));
    insert_text_at_caret(frame_text);

    if (const int excess = get_line_count() - static_cast<int>(segments.capacity()); excess > 0) {
        remove_text(0, 0, excess, 0);
        highlighter->clear_highlighting_cache();
        center_viewport_to_caret();
    }

    set_caret_line(get_line_count() - 1);
    set_caret_column(static_cast<int32_t>(get_line(get_line_count() - 1).length()));
    input_start_line_col = {get_line_count() - 1, static_cast<int32_t>(get_line(get_line_count() - 1).length())};
    queue_redraw();
}

void LinuxHost::_draw() {
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
            const godot::Rect2i rect = get_rect_at_line_column(line, char_column + 1); // EDIT: seg.starting_colum is 0-indexed, but char columns are 1-indexed in godot
            const godot::Rect2i drawRect{rect.position, godot::Size2i{static_cast<int>(cell_width * (seg.length + 1)), rect.size.height}};
            draw_rect(drawRect, godot::Color::hex(seg.bg_color << 8 | 0xFF));
        }
    }
}

void LinuxHost::_paste(int caret_index) {
    const godot::String pastedText = godot::DisplayServer::get_singleton()->clipboard_get();
    if (pastedText.is_empty()) return;
    if (const int64_t rel = getRelativeCaretIndex(); rel >= 0 && rel <= input.length()) {
        input = input.substr(0, rel) + pastedText + input.substr(rel);
        insert_text_at_caret(pastedText);
    }
    else {
        clampCaret();
        if (has_selection()) deselect();
    }
    accept_event();
}

const LineRingBuffer& LinuxHost::getSegments() const {return segments;}

#endif