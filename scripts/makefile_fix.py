#!/usr/bin/env python3
import io
from pathlib import Path

path = Path('/home/darkdevil404/OS/Makefile')
text = path.read_text()

# 1) compiler flags: add explicit -fpermissive / -Wno-narrowing for desktop only
old_desktop_link = '''$(CXX) $(USER_CXXFLAGS) -no-pie -nostdlib userland/desktop.c gui/c/freestanding.o gui/c/draw.o gui/c/fb_draw.o gui/cpp/cpprt.o gui/cpp/gui_wrappers.o gui/cpp/Compositor.o gui/cpp/InputRouter.o gui/cpp/Window.o gui/cpp/Widget.o gui/cpp/Button.o gui/cpp/Label.o gui/cpp/TextBox.o gui/cpp/ScrollView.o gui/asm/draw.o -o desktop.elf'''
new_desktop_link = '''$(CXX) $(USER_CXXFLAGS) -no-pie -nostdlib -fpermissive -Wno-narrowing userland/desktop.c gui/c/freestanding.o gui/c/draw.o gui/c/fb_draw.o gui/cpp/cpprt.o gui/cpp/gui_wrappers.o gui/cpp/Compositor.o gui/cpp/InputRouter.o gui/cpp/Window.o gui/cpp/Widget.o gui/cpp/Button.o gui/cpp/Label.o gui/cpp/TextBox.o gui/cpp/ScrollView.o gui/asm/draw.o -o desktop.elf'''
if old_desktop_link in text:
    text = text.replace(old_desktop_link, new_desktop_link)
else:
    print('ERROR: desktop link line not found')
    raise SystemExit(1)

# 2) compile rule: pass explicit C++ flags to avoid g++ mismatches / narrow errors
old_cpp = '''%.o: %.cpp
\t$(CXX) $(USER_CXXFLAGS) -c $< -o $@'''
new_cpp = '''%.o: %.cpp
\t$(CXX) $(USER_CXXFLAGS) -fpermissive -c $< -o $@'''
text = text.replace(old_cpp, new_cpp)

# 3) rule to compile desktop.c into object first to allow cleaner flags
# we insert a dedicated rule before the generic %.o
needle = '''%.o: %.c
\t$(CC) $(CFLAGS) -c $< -o $@
'''
insert = '''%.o: %.c
\t$(CC) $(CFLAGS) -c $< -o $@

userland/desktop.o: userland/desktop.c
\t$(CC) $(USER_CFLAGS) -fpermissive -Wno-narrowing -c $< -o $@
'''
if needle in text:
    text = text.replace(needle, insert, 1)
else:
    print('WARNING: C pattern rule not found')

# 4) link line: link desktop.o instead of desktop.c
text = text.replace('userland/desktop.c', 'userland/desktop.o')

path.write_text(text)
print('Makefile patched')
