#ifndef LINUXHOST_H
#define LINUXHOST_H

#include "../shared/RingBuffer.h"
#include "../shared/LineRingBuffer.h"
#include <godot_cpp/classes/text_edit.hpp>
#include <godot_cpp/classes/syntax_highlighter.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/font.hpp>
#include <bits/basic_string.h>
#include <sys/types.h>


enum class ParseState {
    Normal,
    Escape,
    CSI
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

    godot::String input;
    godot::Ref<godot::Font> font;

    godot::String history_temp;
    int32_t history_index{0};
    RingBuffer<godot::String> history{500};

    godot::Vector2i input_start_line_col{0, 0};

    static constexpr uint8_t TOTAL_MAX_COLS{120};

    char buffer[32768]; // 32KB read buffer(for read()), in order not to spike the frame rate

    godot::Ref<AnsiHighlighter> highlighter;

    LineRingBuffer segments{22560};

    static bool fileExists(const char *path);

    static godot::PackedStringArray readEtcShells();
    static godot::String loadHistoryPath(const std::string &shell_name);
    void loadHistory(uint32_t max_lines, const std::string &shell_name);

    void clampCaret();
    [[nodiscard]] int64_t getRelativeCaretIndex() const;

    void pushToSegments(godot::String &frame_text, godot::String &cur_text);
    void getHighlighting(const godot::String &ansi_string, godot::String &frame_text);


protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    void startTerminal();
    void endTerminal();
    void writeToTerminal(const godot::String &text);
    godot::String readFromTerminal();

    void _ready() override;
    void _exit_tree() override;
    void _gui_input(const godot::Ref<godot::InputEvent> &event) override;
    void _process(double p_delta) override;
    void _draw() override;
    void _paste(int caret_index) override;

    [[nodiscard]] const LineRingBuffer& getSegments() const;
};
#endif