# GUI Interaction Improvement Plan

## Goal
Enhance the OS GUI subsystem so that user interactions (mouse movement, clicks, keyboard focus) work correctly.

## Steps
1. **Explore current GUI code** – Use `Read`/`Grep` to locate relevant structs and functions (already done).
2. **Add cursor state** in `ui/gui_impl.c`:
   - Introduce `static int32_t cursor_x, cursor_y`.
   - Update them on `LIBINPUT_EVENT_POINTER_MOTION` and `LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE` events.
3. **Improve hit‑testing**:
   - Change `gui_hit_test` to use stored `cursor_x/ cursor_y` when called without explicit coords.
   - Provide an overload `gui_hit_test_at(window_t *win, int32_t x, int32_t y)` for explicit tests.
4. **Track focused widget**:
   - Add `static widget_t *focused_widget`.
   - On click, set focus to the clicked widget (`focused_widget = w`).
   - Dispatch keyboard events to `focused_widget` if it has `on_key`.
5. **Update event dispatch** in `gui_dispatch_event`:
   - Handle motion events to update cursor.
   - Handle button events to perform hit‑test, call `on_click`, and set focus.
   - Handle keyboard events to forward to `focused_widget`.
6. **Add helper functions** in `ui/widget.c`:
   - `widget_set_focus(widget_t *w)` to mark a widget focused and update `focused_widget`.
   - Ensure existing widgets expose `on_click`/`on_key` callbacks (already present).
7. **Write minimal stubs for drawing focus** (optional, just a placeholder comment).
8. **Compile sanity check** – run `make` or appropriate build command to ensure no errors.
9. **Run a quick interactive test** (if possible) by simulating libinput events via a small test program.
10. **Generate updated Graphify visual** (optional) – regenerate graph if needed.

## Validation
- Cursor position updates on motion events.
- Clicking a widget triggers its `on_click` and gives it focus.
- Keyboard events are received by the focused widget.
- Build succeeds without warnings.

## Notes
- Keep changes minimal and confined to `ui/` files.
- Do not modify unrelated subsystems.
- All code follows existing style conventions.
