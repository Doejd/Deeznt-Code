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
    bool hasBg{false};
    bool bold{false};
};

class LineRingBuffer {
    size_t capacity_{0}, size_{0};
    std::vector<godot::Vector<Segment>> data;
    size_t head{0};

    size_t GetPhysicalIndex(size_t index) const {
        return (head + index) % capacity_;
    }

public:
    explicit LineRingBuffer(const size_t &capacity) : capacity_(capacity) , data(capacity_) {}
    ~LineRingBuffer() = default;

    void clear() {
        for (auto &buf : data) buf.clear();
        head = 0;
        size_ = 0;
    }

    godot::Vector<Segment>& operator[](size_t index) {
        return data[GetPhysicalIndex(index)];
    }

    const godot::Vector<Segment>& operator[](size_t index) const {
        return data[GetPhysicalIndex(index)];
    }

    size_t size() const {return size_;}
    size_t capacity() const {return capacity_;}

    bool empty() const {return size_ == 0;}
    bool full() const {return size_ == capacity_;}

    void pushSegment(const Segment& seg) {
        if (empty()) initNewLine();
        data[GetPhysicalIndex(size_ - 1)].push_back(seg);
    }

    void initNewLine() {
        if (capacity_ == 0) return;

        size_t index;

        if (size_ < capacity_) {
            index = GetPhysicalIndex(size_);
            ++size_;
        }
        else {
            head = (head + 1) % capacity_;
            index = GetPhysicalIndex(size_ - 1);
        }

        data[index].clear();
    }
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

    CHAR buf[32768]; // 32KB read buffer(for ReadFile()), in order not to spike frame rate

    uint8_t MAX_COLS{120};

    godot::String input;
    godot::Vector2i input_start_line_col{0, 0};
    godot::Ref<godot::Font> font;

    Segment current;

    godot::String history_temp;
    int32_t history_index{0};
    godot::PackedStringArray history;

    godot::Ref<AnsiHighlighter> highlighter;

    LineRingBuffer segments{22560};

    static bool fileExists(const char *path);
    void loadHistory(const uint32_t &max_lines);

    bool clampCaret();
    [[nodiscard]] int64_t getRelativeCaretIndex() const;

    static int ansiToColor(const int &code);
    static int ansi256ToColor(const int &code);

    static void applyStyle(int code, Segment &seg);

    static void applyArgs(Segment &seg, const godot::String &args);

    void pushToSegments(godot::String &frame_text);
    void getHighlighting(godot::String &ansi_string, godot::String &frame_text);

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

    [[nodiscard]] LineRingBuffer getSegments() const;
};


#endif // CMD_HOST_H