from pathlib import Path

app = Path("Engine/App.c").read_text()
game = Path("Engine/game.c").read_text()

sequence = [
    "input_begin_frame();",
    "imgui_newframe(state.dt, state.fb->ww, state.fb->wh, state.fb->w, state.fb->h);",
    "game_handle_input();",
    "game_update();",
    "game_render();",
]

positions = [app.find(token) for token in sequence]
if any(position < 0 for position in positions):
    raise SystemExit(f"missing frame-loop token(s): {list(zip(sequence, positions))}")

if positions != sorted(positions):
    raise SystemExit("ImGui NewFrame must run after input_begin_frame and before scene input/update/render")

if "imgui_newframe(" in game:
    raise SystemExit("game_render must not start a second ImGui frame")
