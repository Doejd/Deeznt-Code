#ifndef LINUXHOST_H
#define LINUXHOST_H

#include "../shared/RingBuffer.h"
#include "../shared/LineRingBuffer.h"
#include <godot_cpp/classes/text_edit.hpp>
#include <godot_cpp/classes/syntax_highlighter.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
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

    // The properties that will be exposed to the engine
    int32_t MAX_COLS_PER_LINE{120};
    int32_t MAX_HISTORY_SIZE{100};
    int32_t MAX_TERMINAL_SCROLLBACK_LINES{22560};
    float_t AMOUNT_TO_DARKEN_NON_BOLD_COLORS{1.15};
    godot::String selectedShell{"/usr/bin/bash"};

    godot::String input;
    godot::Ref<godot::Font> font;
    godot::Vector2i input_start_line_col{0, 0};

    godot::String history_temp;
    int32_t history_index{0};
    RingBuffer<godot::String> history{static_cast<size_t>(MAX_HISTORY_SIZE)};

    godot::Ref<AnsiHighlighter> highlighter;

    Segment current;
    LineRingBuffer segments{static_cast<size_t>(MAX_TERMINAL_SCROLLBACK_LINES)};

    static bool fileExists(const char *path);

    static godot::PackedStringArray readEtcShells();
    static godot::String loadHistoryPath(const std::string &shell_name);
    void loadHistory(const std::string &shell_name);

    void clampCaret();
    [[nodiscard]] int32_t getRelativeCaretIndex() const;
    [[nodiscard]] bool isAfterReadOnlyText() const;

    static bool isNavigationKey(const godot::Ref<godot::InputEventKey> &key_event);

    void pushToSegments(godot::String &frame_text, godot::String &cur_text);
    void getHighlighting(const godot::String &ansi_string, godot::String &frame_text);


protected:
    static void _bind_methods();
    void _notification(int p_what);
    void _validate_property(godot::PropertyInfo &p_property) const;

public:
    void startTerminal();
    void endTerminal();
    void writeToTerminal(const godot::String &text);
    godot::String readFromTerminal();

    //getters and setters
    float_t get_amount_to_darken_non_bold_colors() const {return AMOUNT_TO_DARKEN_NON_BOLD_COLORS;}
    int32_t get_max_columns_per_line() const {return MAX_COLS_PER_LINE;}
    int32_t get_max_history_size() const {return MAX_HISTORY_SIZE;}
    int32_t get_max_terminal_scrollback_lines() const {return MAX_TERMINAL_SCROLLBACK_LINES;}

    void set_amount_to_darken_non_bold_colors(const float_t amount) {
        if (running) return;

        AMOUNT_TO_DARKEN_NON_BOLD_COLORS = amount;
    }
    void set_max_columns_per_line(const int32_t max_columns_per_line) {
        if (running) return;

        MAX_COLS_PER_LINE = max_columns_per_line;
    }
    void set_max_history_size(const int32_t new_size) {
        if (running) return;

        MAX_HISTORY_SIZE = new_size;
        history.resize(static_cast<size_t>(MAX_HISTORY_SIZE));
        loadHistory(selectedShell.utf8().get_data());
    }
    void set_max_terminal_scrollback_lines(const int32_t new_size) {
        if (running) return;

        MAX_TERMINAL_SCROLLBACK_LINES = new_size;
        segments.resize(static_cast<size_t>(MAX_TERMINAL_SCROLLBACK_LINES));
    }

    void _ready() override;
    void _exit_tree() override;
    void _gui_input(const godot::Ref<godot::InputEvent> &event) override;
    void _process(double p_delta) override;
    void _draw() override;
    void _paste(int caret_index) override;

    [[nodiscard]] const LineRingBuffer& getSegments() const;
};
#endif