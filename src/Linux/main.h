#ifndef LINUXHOST_H
#define LINUXHOST_H

#include <godot_cpp/classes/text_edit.hpp>
#include <godot_cpp/classes/syntax_highlighter.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/font.hpp>
#include <sys/types.h>
#include <queue>
#include <deque>

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
    GDCLASS(AnsiHighlighter, godot::SyntaxHighlighter);

    protected:
        static void _bind_methods();

    public:
     [[nodiscard]] godot::Dictionary _get_line_syntax_highlighting(int line) const override;
};

class LinuxHost : public godot::TextEdit {
    GDCLASS(LinuxHost, godot::TextEdit);


    int master_fd{-1};
    int slave_fd{-1};
    pid_t child_pid{-1};
    bool running{false};

    Segment current;
    godot::String leftoverRead;

    godot::String input;
    godot::Ref<godot::Font> font;

    godot::String history_temp;
    int32_t history_index{0};
    godot::PackedStringArray history;

    godot::Vector2i input_start_line_col{0, 0};

    int MAX_LINES_PER_FRAME{50};
    int TOTAL_MAX_LINES{22560};

    godot::Ref<AnsiHighlighter> highlighter;

    std::deque<godot::Vector<Segment>> segments_to_line;

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

    [[nodiscard]] std::deque<godot::Vector<Segment>> get_segments_to_line() const;
};
#endif