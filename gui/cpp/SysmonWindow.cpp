#include "SysmonWindow.hpp"

extern "C" {
    #include "../../userland/sys.h"
}

static void num_to_str(uint64_t val, char* buf) {
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    char temp[32];
    int i = 0;
    while (val > 0) {
        temp[i++] = '0' + (val % 10);
        val /= 10;
    }
    int j = 0;
    while (i > 0) {
        buf[j++] = temp[--i];
    }
    buf[j] = '\0';
}

static void strcat_custom(char* dest, const char* src) {
    while (*dest) dest++;
    while (*src) *dest++ = *src++;
    *dest = '\0';
}

SysmonWindow::SysmonWindow(Widget* parent) : Window(parent), tick_timer_(0) {
    set_title("System Monitor");
    set_pos(400, 100);
    set_size(300, 150);

    mem_label_ = new Label(this);
    mem_label_->set_pos(10, 40);
    mem_label_->set_size(280, 20);
    mem_label_->set_text("Mem: Loading...");
    mem_label_->set_color(0xFFFFFF); // White text

    cpu_label_ = new Label(this);
    cpu_label_->set_pos(10, 70);
    cpu_label_->set_size(280, 20);
    cpu_label_->set_text("Procs: Loading...");
    cpu_label_->set_color(0xFFFFFF);
}

void SysmonWindow::tick(int dt_ms) {
    Window::tick(dt_ms);
    tick_timer_ += dt_ms;
    if (tick_timer_ > 1000) {
        tick_timer_ = 0;

        // Update Memory
        uint64_t minfo[2];
        sys_mem_info(minfo);
        uint64_t mem_total_kb = minfo[0] * 4;
        uint64_t mem_used_kb = minfo[1] * 4;
        
        char mbuf[128];
        mbuf[0] = '\0';
        strcat_custom(mbuf, "Mem: ");
        
        char num1[32];
        num_to_str(mem_used_kb, num1);
        strcat_custom(mbuf, num1);
        strcat_custom(mbuf, " KB / ");
        
        char num2[32];
        num_to_str(mem_total_kb, num2);
        strcat_custom(mbuf, num2);
        strcat_custom(mbuf, " KB");

        mem_label_->set_text(mbuf);

        // Update CPU/Procs
        struct proc_info pinfo[64];
        int n_procs = sys_proc_info(pinfo, 64);
        
        char pbuf[128];
        pbuf[0] = '\0';
        strcat_custom(pbuf, "Processes: ");
        
        char num3[32];
        num_to_str(n_procs > 0 ? n_procs : 0, num3);
        strcat_custom(pbuf, num3);

        cpu_label_->set_text(pbuf);
    }
}
