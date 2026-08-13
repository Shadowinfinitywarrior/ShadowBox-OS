#include "CalculatorWindow.hpp"
#include "c_std.h"

static void int_to_str(int val, char* buf) {
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    int is_neg = 0;
    if (val < 0) {
        is_neg = 1;
        val = -val;
    }
    char temp[32];
    int i = 0;
    while (val > 0) {
        temp[i++] = '0' + (val % 10);
        val /= 10;
    }
    int j = 0;
    if (is_neg) {
        buf[j++] = '-';
    }
    while (i > 0) {
        buf[j++] = temp[--i];
    }
    buf[j] = '\0';
}

CalculatorWindow::CalculatorWindow(Widget* parent)
    : Window(parent), current_val_(0), acc_val_(0), current_op_('+'), new_num_(true)
{
    set_title("Calculator");
    set_size(190, 240);

    display_label_ = new Label(this);
    display_label_->set_pos(10, 30);
    display_label_->set_size(170, 30);
    display_label_->set_align(TextAlign::Right);
    update_display();

    const char* buttons[4][4] = {
        {"7", "8", "9", "/"},
        {"4", "5", "6", "*"},
        {"1", "2", "3", "-"},
        {"C", "0", "=", "+"}
    };

    int start_x = 10;
    int start_y = 70;
    int btn_w = 38;
    int btn_h = 35;
    int spacing = 5;

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            Button* btn = new Button(this);
            btn->set_pos(start_x + col * (btn_w + spacing), start_y + row * (btn_h + spacing));
            btn->set_size(btn_w, btn_h);
            btn->set_label(buttons[row][col]);
            btn->user_data = this;

            char c = buttons[row][col][0];
            if (c >= '0' && c <= '9') {
                btn->on_clicked = on_digit_clicked;
            } else if (c == 'C') {
                btn->on_clicked = on_clear_clicked;
            } else if (c == '=') {
                btn->on_clicked = on_eq_clicked;
            } else {
                btn->on_clicked = on_op_clicked;
            }
        }
    }
}

void CalculatorWindow::append_digit(int d) {
    if (new_num_) {
        current_val_ = d;
        new_num_ = false;
    } else {
        current_val_ = current_val_ * 10 + d;
    }
    update_display();
}

void CalculatorWindow::apply_op(char op) {
    calculate();
    current_op_ = op;
    acc_val_ = current_val_;
    new_num_ = true;
}

void CalculatorWindow::calculate() {
    if (new_num_) return;
    
    if (current_op_ == '+') current_val_ = acc_val_ + current_val_;
    else if (current_op_ == '-') current_val_ = acc_val_ - current_val_;
    else if (current_op_ == '*') current_val_ = acc_val_ * current_val_;
    else if (current_op_ == '/') {
        if (current_val_ != 0) current_val_ = acc_val_ / current_val_;
        else current_val_ = 0;
    }
    new_num_ = true;
    update_display();
}

void CalculatorWindow::update_display() {
    int_to_str(current_val_, display_buffer_);
    display_label_->set_text(display_buffer_);
}

void CalculatorWindow::on_digit_clicked(Widget* sender) {
    CalculatorWindow* self = static_cast<CalculatorWindow*>(sender->user_data);
    Button* btn = static_cast<Button*>(sender);
    self->append_digit(btn->label()[0] - '0');
}

void CalculatorWindow::on_op_clicked(Widget* sender) {
    CalculatorWindow* self = static_cast<CalculatorWindow*>(sender->user_data);
    Button* btn = static_cast<Button*>(sender);
    self->apply_op(btn->label()[0]);
}

void CalculatorWindow::on_eq_clicked(Widget* sender) {
    CalculatorWindow* self = static_cast<CalculatorWindow*>(sender->user_data);
    self->calculate();
}

void CalculatorWindow::on_clear_clicked(Widget* sender) {
    CalculatorWindow* self = static_cast<CalculatorWindow*>(sender->user_data);
    self->current_val_ = 0;
    self->acc_val_ = 0;
    self->current_op_ = '+';
    self->new_num_ = true;
    self->update_display();
}
