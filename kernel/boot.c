#include "boot.h"
#include "kernel.h"
#include "serial.h"

static struct boot_stage stages[MAX_BOOT_STAGES];
static int stage_count = 0;
static uint64_t stage_start = 0;
static const char *current_stage = 0;

static uint64_t read_tick(void) {
    uint32_t lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

void boot_stage_begin(const char *name) {
    if (current_stage) boot_stage_end();
    if (stage_count >= MAX_BOOT_STAGES) return;
    stage_start = read_tick();
    current_stage = name;
    int idx = stage_count;
    int i;
    for (i = 0; name[i] && i < BOOT_STAGE_NAME_MAX - 1; i++)
        stages[idx].name[i] = name[i];
    stages[idx].name[i] = 0;
    stages[idx].start_tick = stage_start;
    stages[idx].end_tick = 0;
    stage_count++;
    printk(KERN_INFO "BOOT: %s...\n", name);
}

void boot_stage_end(void) {
    if (!current_stage) return;
    uint64_t now = read_tick();
    int idx = stage_count - 1;
    stages[idx].end_tick = now;
    current_stage = 0;
    uint64_t elapsed = (now - stages[idx].start_tick) / 1000000;
    printk(KERN_INFO "BOOT: %s done (%llu us)\n", stages[idx].name, elapsed);
}

void boot_stages_summary(void) {
    if (current_stage) boot_stage_end();
    printk(KERN_INFO "BOOT: --- Boot Stage Summary ---\n");
    uint64_t total = 0;
    for (int i = 0; i < stage_count; i++) {
        uint64_t elapsed = stages[i].end_tick - stages[i].start_tick;
        elapsed /= 1000000;
        total += elapsed;
        printk(KERN_INFO "BOOT:   %d. %s %llu us\n", i + 1, stages[i].name, elapsed);
    }
    printk(KERN_INFO "BOOT: Total boot time: %llu us\n", total);
}
