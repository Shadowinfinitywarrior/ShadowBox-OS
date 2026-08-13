#pragma once

#include "Window.hpp"
#include "Button.hpp"
#include "Label.hpp"

class CalculatorWindow : public Window {
public:
    explicit CalculatorWindow(Widget* parent = nullptr);

    static void on_digit_clicked(Widget* sender);
    static void on_op_clicked(Widget* sender);
    static void on_eq_clicked(Widget* sender);
    static void on_clear_clicked(Widget* sender);

private:
    Label* display_label_;
    char display_buffer_[64];
    int current_val_;
    int acc_val_;
    char current_op_;
    bool new_num_;

    void append_digit(int d);
    void apply_op(char op);
    void calculate();
    void update_display();
};
