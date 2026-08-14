#!/usr/bin/env python3
"""GUI test driver for the ShadowBox OS terminal.

Boots os.iso headless with a QEMU HMP monitor on a unix socket, opens the
terminal via ctrl-alt-t, then types commands and verifies output by decoding
screendumps with decode_term.py.

Usage:
    python3 tests/gui_drive.py            # run all tests
    python3 tests/gui_drive.py 0 10       # run tests [lo, hi)
"""
import os, socket, subprocess, sys, time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from decode_term import decode

ROOT = "/home/darkdevil404/OS"
SOCK = "/tmp/opencode/mon.sock"
SHOTS = "/tmp/opencode/shots"

QEMU = [
    "qemu-system-x86_64", "-cdrom", ROOT + "/os.iso",
    "-drive", "id=disk,file=" + ROOT + "/ahci_disk.img,if=none,format=raw",
    "-device", "ahci,id=ahci", "-device", "ide-hd,drive=disk,bus=ahci.0",
    "-device", "qemu-xhci,id=xhci", "-device", "intel-hda,debug=4",
    "-device", "hda-output", "-display", "none", "-no-reboot",
    "-monitor", "unix:" + SOCK + ",server,nowait",
]

KEYMAP = {
    " ": "spc", "/": "slash", ".": "dot", "-": "minus", "_": "shift-minus",
    "=": "equal", "+": "shift-equal", "(": "shift-9", ")": "shift-0",
    "*": "shift-8", "%": "shift-5", ":": "shift-semicolon", "<": "shift-comma",
    ",": "comma", ">": "shift-dot", "|": "shift-backslash", "\\": "backslash",
    '"': "shift-apostrophe", "'": "shift-apostrophe",
}

TESTS = [
    ("help", "banner <text>", 1.2),
    ("info", "x86_64 Native OS", 1.2),
    ("uname -a", "x86_64", 1.2),
    ("date", "2026", 1.2),
    ("pwd", "/", 1.2),
    ("whoami", "root", 1.2),
    ("echo hello world", "hello world", 1.2),
    ("echo -n nolf", "nolf", 1.2),
    ("ls -l", "test_data.txt", 2.0),
    ("ls -la /", "test_data.txt", 2.0),
    ("sort /test_data.txt", "cherry", 1.5),
    ("rev /test_data.txt", "raep", 1.5),
    ("tr a e < /test_data.txt", "epple", 1.5),
    ("fold 5 /test_data.txt", "banan", 1.5),
    ("cut , 2 /test_data.txt", "", 1.5),
    ("basename /a/b/c.txt", "c.txt", 1.2),
    ("dirname /a/b/c.txt", "/a/b", 1.2),
    ("stat /shell.elf", "Size", 1.5),
    ("file /shell.elf", "ELF", 1.5),
    ("file /logo.bmp", "BMP", 1.5),
    ("file /test_data.txt", "text", 1.5),
    ("find / test_data", "/test_data.txt", 2.0),
    ("tree /", "test_data", 2.5),
    ("du /", "test_data", 2.0),
    ("which sort", "sort.elf", 1.2),
    ("type ls", "builtin", 1.2),
    ("printenv PATH", "/", 1.2),
    ("calc 7+5*(3-1)", "17", 1.2),
    ("calc (10+2)/3", "4", 1.2),
    ("crc32 /shell.elf", "  /shell.elf", 3.0),
    ("sum /shell.elf", "  /shell.elf", 3.5),
    ("printf %s hello", "hello", 1.2),
    ("printf %d 42", "42", 1.2),
    ("rand 100", "", 1.2),
    ("bench 1", "iter", 3.0),
    ("yes hello 3", "hello:3", 1.2),
    ("banner HI", "H", 1.5),
    ("alias ll=ls", "alias", 1.2),
    ("alias ll", "ll=ls", 1.2),
    ("ll", "shell.elf", 2.0),
    ("unalias ll", "", 1.2),
    ("type ll", "not", 1.2),
    ("history", "which sort", 2.5),
    ("pushd /tmp", "1", 1.2),
    ("pwd", "/tmp", 1.2),
    ("dirs", "tmp", 1.2),
    ("popd", "0", 1.2),
    ("pwd", "/", 1.2),
    ("sort /test_data.txt", "apple", 1.5),
    ("execute sort.elf /test_data.txt", "cherry", 1.5),
    ("sort.elf /test_data.txt | tee.elf /tmp/teeout", "date", 2.0),
    ("which.elf sort", "sort.elf", 1.5),
    ("search.elf test_data /", "test_data.txt", 2.0),
    ("sync", "", 1.2),
    ("reset", "root@shadowbox", 2.5),
]


def wait_sock(seconds):
    end = time.time() + seconds
    while time.time() < end:
        try:
            s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            s.settimeout(0.3)
            s.connect(SOCK)
            s.close()
            return True
        except (OSError, socket.error):
            time.sleep(0.2)
    return False


def send(s, cmd, timeout=0.3):
    s.settimeout(timeout)
    try:
        s.sendall(cmd.encode() + b"\n")
        s.recv(4096)
    except socket.timeout:
        pass
    except OSError:
        pass


def type_cmd(s, cmd):
    for ch in cmd:
        if "a" <= ch <= "z" or "A" <= ch <= "Z":
            if "A" <= ch <= "Z":
                send(s, "sendkey shift-" + ch.lower())
            else:
                send(s, "sendkey " + ch)
        elif "0" <= ch <= "9":
            send(s, "sendkey " + ch)
        else:
            key = KEYMAP.get(ch)
            if key:
                send(s, "sendkey " + key)
        time.sleep(0.04)
    send(s, "sendkey ret")
    time.sleep(0.2)


def check_want(text, want):
    if want == "":
        return True
    if ":" in want and want.rsplit(":", 1)[1].isdigit():
        sub, _, n = want.rpartition(":")
        return text.count(sub) >= int(n)
    return want in text


def main():
    os.makedirs(SHOTS, exist_ok=True)
    for f in (SOCK,):
        if os.path.exists(f):
            os.unlink(f)
    for f in os.listdir(SHOTS):
        os.unlink(os.path.join(SHOTS, f))
    p = subprocess.Popen(QEMU, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if not wait_sock(20):
        print("monitor socket never appeared")
        p.kill()
        return 1
    time.sleep(15)
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.connect(SOCK)
    send(s, "sendkey ctrl-alt-t")
    time.sleep(6)

    lo = int(sys.argv[1]) if len(sys.argv) > 1 else 0
    hi = int(sys.argv[2]) if len(sys.argv) > 2 else len(TESTS)
    results = []
    try:
        for i, (cmd, want, wait) in enumerate(TESTS):
            if i < lo or i >= hi:
                continue
            type_cmd(s, cmd)
            time.sleep(wait)
            name = "sh%02d" % i
            send(s, "screendump %s/%s.ppm" % (SHOTS, name), timeout=3.0)
            ppm = os.path.join(SHOTS, name + ".ppm")
            for _ in range(10):
                if os.path.exists(ppm):
                    break
                time.sleep(0.3)
            lines = decode(ppm)
            text = "\n".join(lines)
            ok = check_want(text, want)
            results.append((cmd, "PASS" if ok else "FAIL", want if ok else text))
            print("[%02d] %-44s %s  %s" % (i, cmd, "PASS" if ok else "FAIL", want), flush=True)
    finally:
        try:
            s.close()
        except OSError:
            pass
        p.kill()
        p.wait()

    with open("/tmp/opencode/gui_report.txt", "w") as f:
        passed = 0
        for cmd, status, detail in results:
            f.write("%-6s %-44s %s\n" % (status, cmd, detail))
            if status == "PASS":
                passed += 1
        f.write("\n%d / %d\n" % (passed, len(results)))
    return 0 if passed == len(results) else 1


if __name__ == "__main__":
    sys.exit(main())
