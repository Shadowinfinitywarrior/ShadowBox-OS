#include "procfs.h"
#include "kernel.h"
#include "vfs.h"
#include "malloc.h"
#include "kstring.h"
#include "task.h"
#include "pmm.h"

vfs_node_t *procfs_root;

// Enhanced procfs with Linux-like entries

static uint32_t proc_meminfo_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node;
    char info_buf[512];
    uint64_t total_mem, used_mem;
    extern void pmm_get_info(uint64_t *total, uint64_t *used);
    pmm_get_info(&total_mem, &used_mem);
    uint64_t free_mem = total_mem - used_mem;
    
    // Convert to KB
    uint64_t total_kb = (total_mem * 4096) / 1024;
    uint64_t free_kb = (free_mem * 4096) / 1024;
    uint64_t used_kb = (used_mem * 4096) / 1024;
    
    // Build the meminfo string
    int pos = 0;
    const char *prefix = "MemTotal: ";
    while (*prefix) info_buf[pos++] = *prefix++;
    
    // Simple number to string conversion
    char num_buf[32];
    int num_pos = 0;
    uint64_t temp = total_kb;
    if (temp == 0) num_buf[num_pos++] = '0';
    else {
        char rev[32];
        int rev_pos = 0;
        while (temp > 0) {
            rev[rev_pos++] = '0' + (temp % 10);
            temp /= 10;
        }
        while (rev_pos > 0) num_buf[num_pos++] = rev[--rev_pos];
    }
    for (int i = 0; i < num_pos; i++) info_buf[pos++] = num_buf[i];
    
    const char *suffix = " kB\nMemFree: ";
    while (*suffix) info_buf[pos++] = *suffix++;
    
    num_pos = 0;
    temp = free_kb;
    if (temp == 0) num_buf[num_pos++] = '0';
    else {
        char rev[32];
        int rev_pos = 0;
        while (temp > 0) {
            rev[rev_pos++] = '0' + (temp % 10);
            temp /= 10;
        }
        while (rev_pos > 0) num_buf[num_pos++] = rev[--rev_pos];
    }
    for (int i = 0; i < num_pos; i++) info_buf[pos++] = num_buf[i];
    
    suffix = " kB\nMemAvailable: ";
    while (*suffix) info_buf[pos++] = *suffix++;
    
    num_pos = 0;
    temp = free_kb;
    if (temp == 0) num_buf[num_pos++] = '0';
    else {
        char rev[32];
        int rev_pos = 0;
        while (temp > 0) {
            rev[rev_pos++] = '0' + (temp % 10);
            temp /= 10;
        }
        while (rev_pos > 0) num_buf[num_pos++] = rev[--rev_pos];
    }
    for (int i = 0; i < num_pos; i++) info_buf[pos++] = num_buf[i];
    
    suffix = " kB\n";
    while (*suffix) info_buf[pos++] = *suffix++;
    
    uint32_t len = pos;
    
    if (offset >= len) return 0;
    if (offset + size > len) size = len - offset;
    
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = info_buf[offset + i];
    }
    return size;
}

static uint32_t proc_stat_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node;
    char stat_buf[512];
    extern struct process *proc_list;
    
    int pos = 0;
    
    // CPU stats (simplified)
    const char *cpu_line = "cpu  0 0 0 0 0 0 0 0 0 0\n";
    while (*cpu_line) stat_buf[pos++] = *cpu_line++;
    
    // Count processes
    int total_procs = 0;
    int running_procs = 0;
    struct process *p = proc_list;
    while (p) {
        total_procs++;
        if (p->state == TASK_RUNNING || p->state == TASK_READY) running_procs++;
        p = p->next;
    }
    
    const char *procs_prefix = "processes ";
    while (*procs_prefix) stat_buf[pos++] = *procs_prefix++;
    
    char num_buf[32];
    int num_pos = 0;
    int temp = total_procs;
    if (temp == 0) num_buf[num_pos++] = '0';
    else {
        char rev[32];
        int rev_pos = 0;
        while (temp > 0) {
            rev[rev_pos++] = '0' + (temp % 10);
            temp /= 10;
        }
        while (rev_pos > 0) num_buf[num_pos++] = rev[--rev_pos];
    }
    for (int i = 0; i < num_pos; i++) stat_buf[pos++] = num_buf[i];
    
    const char *newline = "\n";
    while (*newline) stat_buf[pos++] = *newline++;
    
    const char *running_prefix = "procs_running ";
    while (*running_prefix) stat_buf[pos++] = *running_prefix++;
    
    num_pos = 0;
    temp = running_procs;
    if (temp == 0) num_buf[num_pos++] = '0';
    else {
        char rev[32];
        int rev_pos = 0;
        while (temp > 0) {
            rev[rev_pos++] = '0' + (temp % 10);
            temp /= 10;
        }
        while (rev_pos > 0) num_buf[num_pos++] = rev[--rev_pos];
    }
    for (int i = 0; i < num_pos; i++) stat_buf[pos++] = num_buf[i];
    
    while (*newline) stat_buf[pos++] = *newline++;
    
    const char *blocked_prefix = "procs_blocked 0\n";
    while (*blocked_prefix) stat_buf[pos++] = *blocked_prefix++;
    
    uint32_t len = pos;
    
    if (offset >= len) return 0;
    if (offset + size > len) size = len - offset;
    
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = stat_buf[offset + i];
    }
    return size;
}

static uint32_t proc_cpuinfo_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node;
    const char *cpuinfo = 
        "processor\t: 0\n"
        "vendor_id\t: Unknown\n"
        "cpu family\t: 6\n"
        "model\t\t: 1\n"
        "model name\t: x86_64 CPU\n"
        "stepping\t: 1\n"
        "microcode\t: 0x1\n"
        "cpu MHz\t\t: 1000.000\n"
        "cache size\t: 4096 KB\n"
        "physical id\t: 0\n"
        "siblings\t: 1\n"
        "core id\t\t: 0\n"
        "cpu cores\t: 1\n"
        "apicid\t\t: 0\n"
        "initial apicid\t: 0\n"
        "fpu\t\t: yes\n"
        "fpu_exception\t: yes\n"
        "cpuid level\t: 1\n"
        "wp\t\t: yes\n"
        "flags\t\t: fpu de tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 syscall\n"
        "bugs\t\t:\n"
        "bogomips\t: 2000.00\n"
        "clflush size\t: 64\n"
        "cache_alignment\t: 64\n"
        "address sizes\t: 40 bits physical, 48 bits virtual\n"
        "power management:\n";
    
    uint32_t len = 0;
    while(cpuinfo[len]) len++;
    
    if (offset >= len) return 0;
    if (offset + size > len) size = len - offset;
    
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = cpuinfo[offset + i];
    }
    return size;
}

static uint32_t proc_version_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node;
    const char *version = 
        "Linux version 1.0.0 (darkdevil404@OS) (gcc version 10.0.0) #1 SMP PREEMPT\n";
    
    uint32_t len = 0;
    while(version[len]) len++;
    
    if (offset >= len) return 0;
    if (offset + size > len) size = len - offset;
    
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = version[offset + i];
    }
    return size;
}

static uint32_t proc_filesystems_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node;
    const char *filesystems = 
        "nodev\tsysfs\n"
        "nodev\tproc\n"
        "nodev\tdevfs\n"
        "\ttarfs\n"
        "\text2\n";
    
    uint32_t len = 0;
    while(filesystems[len]) len++;
    
    if (offset >= len) return 0;
    if (offset + size > len) size = len - offset;
    
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = filesystems[offset + i];
    }
    return size;
}

static vfs_node_t *procfs_finddir(vfs_node_t *node, const char *name) {
    (void)node;
    vfs_node_t *n;
    
    if (strcmp(name, "meminfo") == 0) {
        n = kmalloc(sizeof(vfs_node_t));
        if (!n) return NULL;
        memset(n, 0, sizeof(vfs_node_t));
        strcpy(n->name, "meminfo");
        n->flags = FS_FILE;
        n->read = proc_meminfo_read;
        n->length = 512;
        return n;
    }
    if (strcmp(name, "stat") == 0) {
        n = kmalloc(sizeof(vfs_node_t));
        if (!n) return NULL;
        memset(n, 0, sizeof(vfs_node_t));
        strcpy(n->name, "stat");
        n->flags = FS_FILE;
        n->read = proc_stat_read;
        n->length = 512;
        return n;
    }
    if (strcmp(name, "cpuinfo") == 0) {
        n = kmalloc(sizeof(vfs_node_t));
        if (!n) return NULL;
        memset(n, 0, sizeof(vfs_node_t));
        strcpy(n->name, "cpuinfo");
        n->flags = FS_FILE;
        n->read = proc_cpuinfo_read;
        n->length = 512;
        return n;
    }
    if (strcmp(name, "version") == 0) {
        n = kmalloc(sizeof(vfs_node_t));
        if (!n) return NULL;
        memset(n, 0, sizeof(vfs_node_t));
        strcpy(n->name, "version");
        n->flags = FS_FILE;
        n->read = proc_version_read;
        n->length = 128;
        return n;
    }
    if (strcmp(name, "filesystems") == 0) {
        n = kmalloc(sizeof(vfs_node_t));
        if (!n) return NULL;
        memset(n, 0, sizeof(vfs_node_t));
        strcpy(n->name, "filesystems");
        n->flags = FS_FILE;
        n->read = proc_filesystems_read;
        n->length = 128;
        return n;
    }
    return NULL;
}

static struct dirent *procfs_readdir(vfs_node_t *node, uint32_t index) {
    (void)node;
    struct dirent *d;
    
    if (index == 0) {
        d = kmalloc(sizeof(struct dirent));
        if (!d) return NULL;
        memset(d, 0, sizeof(struct dirent));
        strcpy(d->name, "meminfo");
        d->ino = 1;
        return d;
    }
    if (index == 1) {
        d = kmalloc(sizeof(struct dirent));
        if (!d) return NULL;
        memset(d, 0, sizeof(struct dirent));
        strcpy(d->name, "stat");
        d->ino = 2;
        return d;
    }
    if (index == 2) {
        d = kmalloc(sizeof(struct dirent));
        if (!d) return NULL;
        memset(d, 0, sizeof(struct dirent));
        strcpy(d->name, "cpuinfo");
        d->ino = 3;
        return d;
    }
    if (index == 3) {
        d = kmalloc(sizeof(struct dirent));
        if (!d) return NULL;
        memset(d, 0, sizeof(struct dirent));
        strcpy(d->name, "version");
        d->ino = 4;
        return d;
    }
    if (index == 4) {
        d = kmalloc(sizeof(struct dirent));
        if (!d) return NULL;
        memset(d, 0, sizeof(struct dirent));
        strcpy(d->name, "filesystems");
        d->ino = 5;
        return d;
    }
    return NULL;
}

void procfs_init(void) {
    printk(KERN_INFO "PROCFS: Initializing enhanced /proc virtual filesystem...\n");
    
    procfs_root = kmalloc(sizeof(vfs_node_t));
    if (!procfs_root) return;
    memset(procfs_root, 0, sizeof(vfs_node_t));
    strcpy(procfs_root->name, "proc");
    procfs_root->flags = FS_DIRECTORY;
    procfs_root->finddir_func = procfs_finddir;
    procfs_root->readdir_func = procfs_readdir;
    
    printk(KERN_INFO "PROCFS: Registered entries: meminfo, stat, cpuinfo, version, filesystems\n");
}
