#ifndef CMD_HOST_H
#define CMD_HOST_H

#include <godot_cpp/classes/text_edit.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_action.hpp>
#include <godot_cpp/classes/syntax_highlighter.hpp>
#include <godot_cpp/classes/font.hpp>
#include <windows.h>
#include <deque>
#include <vector>

enum class ParseState {
    Normal,
    Escape,
    CSI
};


struct Segment {
    godot::String text{""};
    uint32_t color{0xffffff};
    uint32_t bg_color{0x000000};
    int32_t starting_column{0};
    bool bold{false};
};

class AnsiHighlighter : public godot::SyntaxHighlighter {
    GDCLASS(AnsiHighlighter, SyntaxHighlighter);

protected:
    static void _bind_methods();

public:
    godot::Dictionary _get_line_syntax_highlighting(int line) const override;
};

class WindowsHost : public godot::TextEdit {
    GDCLASS(WindowsHost, TextEdit);

    SECURITY_ATTRIBUTES sa{};
    HANDLE child_stdin_read, parent_stdin_write;
    HANDLE parent_stdout_read, child_stdout_write;
    HPCON hPC;
    COORD size{80, 25};
    SIZE_T attrSize = 0;
    STARTUPINFOEXW si;
    PROCESS_INFORMATION pi{};
    bool running = false;

    uint16_t MAX_TOTAL_LINES{22560};
    CHAR buf[32768]; // 32KB read buffer(for ReadFile()), in order not to spike frame rate

    godot::String input;
    godot::Vector2i input_start_line_col{0, 0};
    godot::Ref<godot::Font> font;

    Segment current;
    godot::String leftoverRead{""};

    godot::String history_temp;
    int32_t history_index{0};
    godot::PackedStringArray history;

    godot::Ref<AnsiHighlighter> highlighter;

    std::deque<std::vector<Segment>> segments;

    static bool fileExists(const char *path);
    void loadHistory(const uint32_t &max_lines);

    bool clampCaret();
    [[nodiscard]] int64_t getRelativeCaretIndex() const;

    void bulkRemove(const int32_t &to_line);

    static int ansiToColor(const int &code);
    static int ansi256ToColor(const int &code);

    static void applyStyle(int code, Segment &seg);

    static void applyArgs(Segment &seg, const godot::String &args);

    void getHighlighting(const godot::String &ansi_string, godot::String &frame_text);

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    void startTerminal();
    void endTerminal();
    void writeToTerminal(const godot::String &text);
    void readFromTerminal();

    void _ready() override;
    void _exit_tree() override;
    void _gui_input(const godot::Ref<godot::InputEvent> &event) override;
    void _process(double p_delta) override;
    void _draw() override;


    [[nodiscard]] std::deque<godot::Vector<Segment>> getSegments() const;
};


#endif // CMD_HOST_H