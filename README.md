# kbextract

A small cross-platform desktop application for browsing and copying highlights
and notes from Kobo eReaders. It reads Kobo databases without modifying them
and produces standard Markdown, Obsidian-compatible Markdown, or plain text.

![kbextract displaying Kobo highlights as Markdown](screenshot.png)

## Credits and project origin

kbextract began as an adaptation of
[Omadji](https://github.com/bjarneo/omadji), created by
[bjarneo](https://github.com/bjarneo). Full credit goes to bjarneo for Omadji
and the original application foundation that made this project possible.

Omadji provided the initial code organization, build foundation, basic QML
interface layout, and Omarchy theming. kbextract retained those foundations
while replacing Omadji's DJI media workflow with Kobo database discovery,
read-only annotation extraction, Markdown formatting, and clipboard export.

## Requirements

- Linux, macOS, or Windows
- Qt 6.9 or later with QML, Quick, Quick Controls 2, SQLite, and SVG support
- CMake 3.21 or later
- A C++20 compiler

Official packages use Qt 6.9.3. Linux builds use GCC, macOS builds use Clang,
and Windows builds use MSVC 2022.

## Build and run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/kbextract
```

On Linux, `make build`, `make run`, and installation under `~/.local` are
available as shortcuts:

```bash
make install
```

On multi-configuration generators, pass `--config Release` when building,
testing, installing, or packaging.

## Appearance

On macOS, the app uses Qt's macOS-style controls, system UI fonts, native menus,
and native file dialogs. It always follows the system's light/dark appearance
and accent, including changes while running. There is no Mac theme picker or
Settings window. Previously saved theme preferences are retained but ignored.
The sidebar is resizable from 246 to 420 logical pixels and remembers its width.

On Linux and Windows, the app retains its custom controls and defaults to System
mode, following the operating system's palette, accent, light/dark preference,
UI font family, fixed-width font family, and UI font point size. Use the theme
icon in the header to open Settings, then choose System, Light, or Dark under
Appearance. On Omarchy systems a fourth Omarchy mode follows
`~/.local/state/omarchy/current/theme/colors.toml`. A saved Omarchy selection
falls back to System if Omarchy is unavailable.

The custom Linux/Windows interface derives captions, body text, headings, reader
text, and reader headings from that system point size. Appearance also provides
an 80–200% text-size override in 10% steps. The override is saved independently,
and changing between System, Light, Dark, and Omarchy never changes typography.
Qt applies display scaling without multiplying these point sizes again. The
macOS reader retains its fixed 15-pixel text and 20-pixel heading sizes in this
iteration.

On macOS, File → Open Database… (`⌘O`) opens a database, Refresh Devices (`⌘R`)
reloads devices, and Close Window (`⌘W`) exits the app. Closing the red window
button also exits; `⌘Q` remains available in the application menu. In the reader,
Edit → Copy (`⌘C`) copies selected text and Select All (`⌘A`) selects the document.
The reader also has a native context menu. Edit → Copy All As and the three footer
buttons export the complete document in the chosen format.

## Kobo devices

The app checks mounted volumes every two seconds for
`.kobo/KoboReader.sqlite`. Connected devices appear automatically; disconnected
databases are closed and restored if the same mount returns. Refresh forces a
database reload; Browse (Linux/Windows) or Open…
(macOS) selects a database manually.

Linux sandbox packages need removable-media access to locations such as
`/media` and `/run/media`. macOS may request removable-volume access. Windows
Kobos are discovered through their drive-letter volume. The native file dialog
is available as a fallback on every platform.

Databases are opened with SQLite's read-only mode. The sidebar lists only books
with visible highlights or notes and shows separate counts for each. A passage
with an attached note counts as a note rather than both a note and a plain
highlight.

## Annotation output

Select a book to open its annotation document. The read-only Markdown source
remains visible: highlighted passages use `> ` blockquote markers and notes are
plain paragraphs. Paired highlights and notes are separated by one blank line;
separate annotation records use two blank lines. Annotations from the same
chapter are grouped below a visible `## ` heading derived from Kobo's
navigation metadata. Split content files are grouped beneath their containing
chapter, and missing titles use `## Untitled chapter`. Heading lines are
displayed larger and bold while their markup remains visible.

Characters are kept unescaped, without injected Markdown escapes or HTML
entities. Single line breaks inside highlights are reflowed as spaces, while
blank lines remain paragraph boundaries and note line breaks remain unchanged.
The source can still be selected and copied with keyboard shortcuts.

The footer provides three complete-document copy formats:

- Copy text (`COPY TEXT` on Linux/Windows) copies plain text suitable for a word processor.
- Copy Obsidian Markdown (`COPY OBS MD` on Linux/Windows) copies Markdown with highlights formatted as Obsidian quote
  callouts.
- Copy Markdown (`COPY MD` on Linux/Windows) copies standard Markdown with blockquoted highlights.

The reader soft-wraps at a centered maximum of 120 monospaced characters and
adapts to narrower windows without changing the copied source.

## Tests

```bash
ctest --test-dir build --output-on-failure
```

CI runs the suite on Linux x86-64, Windows x64, and both Apple Silicon and
Intel macOS runners. Mac UI tests use isolated database fixtures and settings;
the custom UI regression tests continue to run on all platforms. Native menus,
file dialogs, VoiceOver, and live system appearance changes also require an
interactive Mac check; offscreen tests do not verify their native rendering.

## Packages

Linux packaging requires `linuxdeploy`, `linuxdeploy-plugin-qt`, and
`appimagetool`; keep the plugin executable beside `linuxdeploy`, and ensure the
Qt 6.9.3 `qmake` is on `PATH`. Build a self-contained AppImage and DEB with:

```bash
LINUXDEPLOY=/path/to/linuxdeploy-x86_64.AppImage \
APPIMAGETOOL=/path/to/appimagetool-x86_64.AppImage \
packaging/linux/build-packages.sh build build/packages 0.1.0
```

On macOS, CPack produces a deployed DMG. On Windows, install NSIS and CPack
produces an installer:

```bash
cpack --config build/CPackConfig.cmake -C Release -B build/packages
```

The `Packages` GitHub Actions workflow performs these builds for semantic
version tags such as `v0.1.0`, or manually with an explicit version. Produced
artifacts are unsigned; signing, Apple notarization, and store publication are
not part of this repository yet.

## License

Licensed under the [MIT License](LICENSE).
