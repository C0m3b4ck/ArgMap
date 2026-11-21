# ArgMap

A small FLTK app for mapping **Oxford-style debate** arguments, counterarguments,
weights, colours and optional speaker ownership (speakers 1–4) for two sides:

- **Proposition**
- **Opposition**

Counter links are drawn as arrows from the rebutting argument to the argument it
answers. The business rules require a counter to come from the *opposite* side.

## Build

### Quick Linux build (installed FLTK)

Requirements: a C++17 compiler, FLTK 1.3 (`fltk-config`, `fluid`) and zlib
development headers.

```bash
make            # generates UI code from gui/MapWindow.fl and builds build/argmap
make test       # domain/data tests (no GUI needed)
./build/argmap
```

`make clean` removes `build/`. `make regenerate` forces FLUID code regeneration.

### Full build (vendored FLTK + bundled zlib/png/jpeg)

The complete, unmodified FLTK 1.3.11 source tree is included in
`third_party/fltk-1.3.11/`. CMake builds FLTK, FLUID and its bundled image
libraries from source, so no system FLTK installation is needed.

```bash
cmake -S . -B build-cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build-cmake --parallel
ctest --test-dir build-cmake --output-on-failure
./build-cmake/argmap
```

### Windows

Both scripts use the vendored FLTK source and CMake:

```bat
scripts\build_windows_msvc.bat                  REM Visual Studio 17 2022, x64
scripts\build_windows_msvc.bat "Visual Studio 17 2019"  REM optional generator override
scripts\build_windows_mingw.bat                 REM MinGW-w64 / MinGW Makefiles
```

Outputs are `build-windows-msvc\Release\argmap.exe` or
`build-windows-mingw\argmap.exe`. Requirements: CMake 3.15+ and a C++17 compiler
(Visual Studio C++ workload or MinGW-w64). No separate FLTK/zlib install is needed.

## Using the app

- **Add arguments** with the `+ Proposition` / `+ Opposition` buttons in the
  right-hand command panel.
  Adding puts a new box on that side immediately and focuses the editor so you
  can type its text straight away. You can also right-click empty canvas space
  and choose where to add one.
- **Add a counterargument**: select the argument being countered, tick **As
  counter to selected** in the right-hand panel, then click either `+` button. A new
  argument is created on the opposing side and linked automatically. The toggle
  turns itself off after one add.
- **Select** a box by clicking it or its row in the left list. The right panel
  then shows `Save changes` and `Delete argument`; edit text, weight (1–5),
  colour and speaker (None/1–4), then click **Save changes**. Weight, colour and
  speaker apply immediately; text also applies on Enter.
- **Link two existing arguments as counters**: select the rebutting argument,
  click **Mark Counter**, then click the argument it answers. The button changes
  to `2. Click target...`. `Esc` or **Cancel** aborts. Links must cross sides;
  duplicates/self-links are rejected.
- **Delete links** by selecting them in the counter list and pressing **Remove Link**.
- **Rearrange** by dragging boxes, **Auto Layout** by side, middle-drag to pan,
  the mouse wheel to zoom, and `0` to reset the view.
- The window is responsive: sidebar, canvas and editor panels reflow as the
  window is resized, down to 760×520.
- **Save / Open**: click the `Save` / `Open...` buttons in the right-hand panel (or use
  `File > Save` / `File > Open...`). Maps are stored as human-readable `.argmap`
  JSON files containing the title, arguments, positions, weights, colours,
  speakers and counter links. The first save prompts for a filename (Save As);
  later saves update the same file. Reopen a saved `.argmap` file any time to
  keep editing it.
- **File > Export Image (PNG)** saves a 1600×1000 picture of the whole map,
  rendered with the same code used on screen.

## Layer separation

| Directory | Responsibility | May depend on |
|---|---|---|
| `domain/` | Debate rules, model, layout geometry, statistics. Pure C++ (no FLTK, no file I/O). | STL only |
| `app/` | Use cases / controller plus ports (`IView`, `IAppController`, `IMapImageRenderer`). | `domain/` |
| `data/` | JSON repository and PNG writer implementing domain ports. zlib only. | `domain/` |
| `gui/` | FLTK widgets, canvas, shared renderer, dialogs, view adapter. | `app/`, `domain/`, FLTK |
| `main.cpp` | Wires concrete implementations together. | all |

The dependency direction is one-way: **gui → app → domain ← data**. The domain
never includes a GUI or storage header; data implements interfaces declared in
the domain; the GUI talks to the application only through `app/Ports.h`.

## Changing the GUI with FLUID

`gui/MapWindow.fl` is the real source of the main window layout. Open it with:

```bash
fluid gui/MapWindow.fl
```

Move/resize widgets, change labels, tooltips or menu entries, then save and run
`make`. FLUID regenerates `build/generated/MapWindow.H/.cpp` automatically.

Three things in the `.fl` file are wiring rather than decoration; keep them if
you rebuild the UI from scratch:

1. The custom window class `AppWindow` (`class AppWindow` on the `Fl_Window`
   node) which forwards resizes to the responsive layout code.
2. The custom canvas widget `ArgumentCanvas` (`class ArgumentCanvas` on the
   `Fl_Box canvas` node).
3. The public `argmap::gui::WindowView* view` declaration and the callbacks that
   call `view->on...()`; they connect widgets to the application controller.

The visual drawing itself lives in `gui/Renderer.cpp`, shared by the interactive
canvas and the PNG exporter, so layout changes and exported images stay in sync.
For CMake builds, FLUID output goes to `build-cmake/generated/MapWindow.H/.cpp`.

## Credits and licensing

ArgMap itself is licensed under the **Apache License 2.0** — see [`LICENSE`](LICENSE).

ArgMap builds on:

- **FLTK 1.3.11** — Bill Spitzak and the FLTK developers (Greg Ercolano,
  Manolo Gouy, Ian MacArthur, Matthias Melcher, Albrecht Schlosser, and many
  past contributors). GNU Library General Public License version 2 with
  FLTK's static-linking exceptions; license in
  `third_party/fltk-1.3.11/COPYING`, credits in
  `third_party/fltk-1.3.11/CREDITS`.
- **zlib 1.3.1** — Jean-loup Gailly and Mark Adler (used for PNG compression).
- **libpng** — PNG Reference Library Authors, Cosmin Truta, Glenn Randers-Pehrson
  and others (bundled with FLTK).
- **Independent JPEG Group libjpeg** — Tom Lane, Guido Vollbeding and the IJG
  contributors (bundled with FLTK).

Full component details, upstream URLs, checksums and license file locations are
in [`THIRD_PARTY.md`](THIRD_PARTY.md). The vendored FLTK source is unmodified and
its copyright/license files are kept intact. Keep those notices when redistributing.
