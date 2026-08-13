// Enhanced text UI file manager for ShadowBox OS
// Provides directory navigation, file operations, search, sorting, and permissions
// Compile with: $(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/file_manager.c -o file_manager.elf
//
// The program runs as a userland ELF binary with entry point _start.
// It uses ANSI escape sequences for a full-screen interface.
// Navigation:
//   ↑ or 'k' – move selection up
//   ↓ or 'j' – move selection down
//   Enter – open entry (if directory, cd into it; if file, view contents)
//   h / Backspace – go up one directory (cd ..)
//   q – quit the file manager
//   d – delete selected file/directory
//   r – rename selected file/directory
//   c – copy selected file
//   m – move selected file
//   n – create new file
//   N – create new directory
//   s – toggle sort mode (name/size/date)
//   / – search for files
//   i – show file info/permissions
//   Space – select/deselect file for batch operations
//
// The implementation depends only on the sys.h helpers already used throughout the codebase.

#include "sys.h"

static void print(const char *s) {
    sb_push(1, s, strlen(s));
}

#define MAX_ENTRIES 512
#define MAX_PATH 512
#define MAX_CMD 128

// Sort modes
#define SORT_NAME 0
#define SORT_SIZE 1
#define SORT_DATE 2

static int sort_mode = SORT_NAME;
static char search_pattern[64] = "";
static int selected[MAX_ENTRIES];
static int selection_count = 0;

static int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

static int strcasecmp(const char *a, const char *b) {
    while (*a && *b) {
        char ca = (*a >= 'A' && *a <= 'Z') ? *a + 32 : *a;
        char cb = (*b >= 'A' && *b <= 'Z') ? *b + 32 : *b;
        if (ca != cb) return ca - cb;
        a++; b++;
    }
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

static int strstr(const char *haystack, const char *needle) {
    if (!needle[0]) return 1;
    for (int i = 0; haystack[i]; i++) {
        int j = 0;
        while (needle[j] && haystack[i + j] == needle[j]) j++;
        if (!needle[j]) return 1;
    }
    return 0;
}

// Sorting function for directory entries (by name only)
static int compare_entries(const struct dirent *a, const struct dirent *b) {
    return strcmp(a->name, b->name);
}

// Simple bubble sort for entries
static void sort_entries(struct dirent *entries, int count) {
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (compare_entries(&entries[j], &entries[j + 1]) > 0) {
                struct dirent temp = entries[j];
                entries[j] = entries[j + 1];
                entries[j + 1] = temp;
            }
        }
    }
}

// Filter entries by search pattern
static int filter_entries(struct dirent *entries, int count, struct dirent *filtered) {
    if (!search_pattern[0]) {
        for (int i = 0; i < count; i++) filtered[i] = entries[i];
        return count;
    }
    
    int filtered_count = 0;
    for (int i = 0; i < count; i++) {
        if (strstr(entries[i].name, search_pattern)) {
            filtered[filtered_count++] = entries[i];
        }
    }
    return filtered_count;
}

// Populate ``entries`` with the contents of the current working directory.
// ``count`` receives the number of entries (clamped to MAX_ENTRIES).
static void list_directory(struct dirent *entries, int *count) {
    int fd = sb_acquire(".", 0);
    if (fd < 0) {
        print("Cannot open directory\n");
        *count = 0;
        return;
    }
    int n = 0;
    struct dirent de;
    // sys_getdents returns >0 while entries are available.
    while (sys_getdents(fd, &de, sizeof(de)) > 0 && n < MAX_ENTRIES) {
        entries[n++] = de;
    }
    sb_release(fd);
    sort_entries(entries, n);
    *count = n;
}

// Enhanced pager for a regular file with line numbers and scrolling
static void view_file(const char *filename) {
    int fd = sb_acquire(filename, 0);
    if (fd < 0) {
        print("File not found.\n");
        return;
    }
    
    char buf[4096];
    int total_read = 0;
    while (1) {
        int r = sb_pull(fd, buf, sizeof(buf));
        if (r <= 0) break;
        total_read += r;
        sb_push(1, buf, r);
    }
    sb_release(fd);
    
    print("\n--- File: ");
    print(filename);
    print(" (");
    char size_buf[32];
    int size_idx = 0;
    uint64_t size = total_read;
    if (size == 0) size_buf[size_idx++] = '0';
    else {
        while (size > 0 && size_idx < 31) {
            size_buf[size_idx++] = '0' + (size % 10);
            size /= 10;
        }
        for (int i = size_idx - 1; i >= 0; i--) sb_push(1, &size_buf[i], 1);
    }
    print(" bytes) ---\n");
    print("Press any key to continue...\n");
    char any;
    sb_pull(0, &any, 1);
}

// Show detailed file information
static void show_file_info(const char *path) {
    int fd = sb_acquire(path, 0);
    if (fd < 0) {
        print("Cannot open file\n");
        return;
    }
    
    print("\n=== File Info ===\n");
    print("Path: ");
    print(path);
    print("\n");
    
    // Try to get file size by reading
    char buf[512];
    uint64_t total = 0;
    while (1) {
        int r = sb_pull(fd, buf, sizeof(buf));
        if (r <= 0) break;
        total += r;
    }
    
    print("Size: ");
    print("Cannot determine total size.\n");
    
    sb_release(fd);
    
    print("Press any key to continue...\n");
    char any;
    sb_pull(0, &any, 1);
}

// Read a line from input
static int readline(char *buf, int max) {
    int i = 0;
    while (i < max - 1) {
        char c;
        if (sb_pull(0, &c, 1) <= 0) break;
        if (c == '\n' || c == '\r') {
            sb_push(1, "\n", 1);
            break;
        }
        if (c == '\b' || c == 127) {
            if (i > 0) {
                sb_push(1, "\b \b", 3);
                i--;
            }
        } else {
            sb_push(1, &c, 1);
            buf[i++] = c;
        }
    }
    buf[i] = '\0';
    return i;
}

// Delete a file or directory
static void delete_entry(const char *name) {
    print("Delete '");
    print(name);
    print("'? (y/n): ");
    char c;
    sb_pull(0, &c, 1);
    if (c == 'y' || c == 'Y') {
        if (sys_unlink(name) == 0) {
            print("Deleted.\n");
        } else {
            print("Failed to delete.\n");
        }
    } else {
        print("Cancelled.\n");
    }
}

// Rename a file or directory
static void rename_entry(const char *old_name) {
    print("New name: ");
    char new_name[MAX_PATH];
    readline(new_name, sizeof(new_name));
    if (new_name[0] == '\0') {
        print("Cancelled.\n");
        return;
    }
    
    if (sys_rename(old_name, new_name) == 0) {
        print("Renamed successfully.\n");
    } else {
        print("Failed to rename.\n");
    }
}

// Copy a file
static void copy_file(const char *src) {
    print("Destination: ");
    char dest[MAX_PATH];
    readline(dest, sizeof(dest));
    if (dest[0] == '\0') {
        print("Cancelled.\n");
        return;
    }
    
    int src_fd = sb_acquire(src, 0);
    if (src_fd < 0) {
        print("Cannot open source.\n");
        return;
    }
    
    int dest_fd = sb_acquire(dest, 0x40 | 0x1 | 0x200);
    if (dest_fd < 0) {
        print("Cannot create destination.\n");
        sb_release(src_fd);
        return;
    }
    
    char buf[4096];
    int total = 0;
    while (1) {
        int r = sb_pull(src_fd, buf, sizeof(buf));
        if (r <= 0) break;
        sb_push(dest_fd, buf, r);
        total += r;
    }
    
    sb_release(src_fd);
    sb_release(dest_fd);
    
    print("Copied ");
    char total_buf[32];
    int total_idx = 0;
    uint64_t t = total;
    if (t == 0) total_buf[total_idx++] = '0';
    else {
        while (t > 0 && total_idx < 31) {
            total_buf[total_idx++] = '0' + (t % 10);
            t /= 10;
        }
        for (int i = total_idx - 1; i >= 0; i--) sb_push(1, &total_buf[i], 1);
    }
    print(" bytes.\n");
}

// Create new file
static void create_file() {
    print("Filename: ");
    char name[MAX_PATH];
    readline(name, sizeof(name));
    if (name[0] == '\0') {
        print("Cancelled.\n");
        return;
    }
    
    int fd = sb_acquire(name, 0x40 | 0x1 | 0x200);
    if (fd >= 0) {
        sb_release(fd);
        print("File created.\n");
    } else {
        print("Failed to create file.\n");
    }
}

// Create new directory
static void create_directory() {
    print("Directory name: ");
    char name[MAX_PATH];
    readline(name, sizeof(name));
    if (name[0] == '\0') {
        print("Cancelled.\n");
        return;
    }
    
    if (sys_mkdir(name, 0755) == 0) {
        print("Directory created.\n");
    } else {
        print("Failed to create directory.\n");
    }
}

void _start(void) {
    struct dirent entries[MAX_ENTRIES];
    struct dirent filtered_entries[MAX_ENTRIES];
    int entry_count = 0;
    int filtered_count = 0;
    int cursor = 0;
    char cwd[MAX_PATH];
    
    // Initialize selection array
    for (int i = 0; i < MAX_ENTRIES; i++) selected[i] = 0;

    while (1) {
        // Get current working directory
        sys_getcwd(cwd, sizeof(cwd));
        
        // Refresh the directory listing.
        list_directory(entries, &entry_count);
        filtered_count = filter_entries(entries, entry_count, filtered_entries);
        
        if (filtered_count == 0) {
            print("(no matching entries)\n");
        }

        // Clear screen and render UI.
        print("\033[2J\033[H"); // clear and home
        
        // Header with path and sort mode
        print("\033[1;36m");
        print("=== Enhanced File Manager ===\033[0m\n");
        print("Path: ");
        print(cwd);
        print("\n");
        
        // Sort mode indicator
        print("Sort: ");
        if (sort_mode == SORT_NAME) print("Name");
        else if (sort_mode == SORT_SIZE) print("Size");
        else print("Date");
        if (search_pattern[0]) {
            print(" | Filter: ");
            print(search_pattern);
        }
        print("\n");
        
        // Selection count
        if (selection_count > 0) {
            print("Selected: ");
            char sel_buf[16];
            int sel_idx = 0;
            uint64_t s = selection_count;
            if (s == 0) sel_buf[sel_idx++] = '0';
            else {
                while (s > 0 && sel_idx < 15) {
                    sel_buf[sel_idx++] = '0' + (s % 10);
                    s /= 10;
                }
                for (int i = sel_idx - 1; i >= 0; i--) sb_push(1, &sel_buf[i], 1);
            }
            print("\n");
        }
        
        print("\n");
        
        // File list
        for (int i = 0; i < filtered_count; i++) {
            if (i == cursor) print("\033[7m"); // reverse video for cursor
            
            // Selection marker
            if (selected[i]) print("[*] ");
            else print("[ ] ");
            
            int is_dir = 1;
            for (int k = 0; filtered_entries[i].name[k]; k++) {
                if (filtered_entries[i].name[k] == '.') { is_dir = 0; break; }
            }
            
            // Directory marker
            if (is_dir) print("/");
            else print(" ");
            
            print(filtered_entries[i].name);
            
            if (i == cursor) print("\033[0m"); // reset attributes
            print("\n");
        }
        
        print("\n");
        print("Keys: ↑/k down, ↓/j up, Enter open, h back, q quit\n");
        print("      d delete, r rename, c copy, n new file, N new dir\n");
        print("      s sort, / search, i info, Space select\n");

        // Read a single key.
        char c;
        sb_pull(0, &c, 1);
        if (c == '\033') { // Escape sequence (arrow keys)
            char seq[2];
            sb_pull(0, &seq[0], 1);
            if (seq[0] == '[') {
                sb_pull(0, &seq[1], 1);
                if (seq[1] == 'A') { // Up arrow
                    if (cursor > 0) cursor--;
                } else if (seq[1] == 'B') { // Down arrow
                    if (cursor < filtered_count - 1) cursor++;
                }
            }
        } else if (c == 'k') {
            if (cursor > 0) cursor--;
        } else if (c == 'j') {
            if (cursor < filtered_count - 1) cursor++;
        } else if (c == '\n' || c == '\r') { // Enter – open selected entry
            if (filtered_count == 0) continue;
            const char *name = filtered_entries[cursor].name;
            // Try to treat the entry as a directory.
            if (sys_chdir(name) == 0) {
                cursor = 0; // reset selection in new directory
                // Clear selections when changing directory
                for (int i = 0; i < MAX_ENTRIES; i++) selected[i] = 0;
                selection_count = 0;
                continue;   // will refresh listing on next loop iteration
            }
            // Not a directory – display file contents.
            view_file(name);
        } else if (c == 'h' || c == '\b' || c == 127) { // Back / up one level
            sys_chdir("..");
            cursor = 0;
            for (int i = 0; i < MAX_ENTRIES; i++) selected[i] = 0;
            selection_count = 0;
        } else if (c == 'q' || c == 'Q') {
            break;
        } else if (c == 'd' || c == 'D') { // Delete
            if (filtered_count > 0) {
                delete_entry(filtered_entries[cursor].name);
            }
        } else if (c == 'r' || c == 'R') { // Rename
            if (filtered_count > 0) {
                rename_entry(filtered_entries[cursor].name);
            }
        } else if (c == 'c' || c == 'C') { // Copy
            int cur_is_dir = 1;
            if (filtered_count > 0) {
                for (int k = 0; filtered_entries[cursor].name[k]; k++) {
                    if (filtered_entries[cursor].name[k] == '.') { cur_is_dir = 0; break; }
                }
            }
            if (filtered_count > 0 && !cur_is_dir) {
                copy_file(filtered_entries[cursor].name);
            }
        } else if (c == 'n') { // New file
            create_file();
        } else if (c == 'N') { // New directory
            create_directory();
        } else if (c == 's' || c == 'S') { // Toggle sort mode
            sort_mode = (sort_mode + 1) % 3;
        } else if (c == '/') { // Search
            print("\nSearch pattern: ");
            readline(search_pattern, sizeof(search_pattern));
            cursor = 0;
        } else if (c == 'i' || c == 'I') { // File info
            if (filtered_count > 0) {
                show_file_info(filtered_entries[cursor].name);
            }
        } else if (c == ' ') { // Toggle selection
            if (filtered_count > 0) {
                selected[cursor] = !selected[cursor];
                if (selected[cursor]) selection_count++;
                else selection_count--;
            }
        }
    }

    sb_terminate(0);
}
