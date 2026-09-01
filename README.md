# kbextract

A small Qt 6/QML desktop application for extracting highlights and notes from
Kobo eReaders. The current increment detects Kobo databases, lists the books
that contain annotations, and shows the selected book's highlights and notes
as read-only Markdown source. Exporting will be added later.

## Requirements

- Qt 6.8 or later with QML, Quick, Quick Controls 2, and SQLite support
- CMake 3.21 or later
- A C++20 compiler

## Build and run

```bash
make build
make run
```

Install the executable and desktop entry under `~/.local` with:

```bash
make install
```

## Appearance

The app starts in Omarchy mode and follows the active palette from
`~/.local/state/omarchy/current/theme/colors.toml`. Palette changes are picked
up while the app is running across backgrounds, controls, text, borders,
selection, and accents. The interface also follows Omarchy's caption, body,
and heading sizes from the active theme's `shell.toml`, with
`~/.config/omarchy/shell.toml` applied as the user override. Changes made with
`omarchy display text size` are picked up while the app is running. The
Markdown document keeps its own reading sizes. When Omarchy state is
unavailable, the app uses built-in palette and typography defaults.

Use the button in the header to cycle between Omarchy, light, and dark modes.
The selected mode is saved across launches.

## Kobo devices

On startup, the app looks for mounted volumes containing
`.kobo/KoboReader.sqlite`. Use `REFRESH` after connecting a device, or
`BROWSE...` to choose a database manually.

Databases are opened with SQLite's read-only mode. The sidebar lists only books
with visible highlights or notes and shows separate counts for each. A passage
with an attached note counts as a note rather than both a note and a plain
highlight.

Select a book to open its annotation document. The read-only Markdown source
remains visible: highlighted passages use `> ` blockquote markers and notes are
plain paragraphs. Paired highlights and notes are separated by one blank line;
separate annotation records use two blank lines. Annotations from the same
chapter are grouped below a visible `## ` chapter heading; missing titles use
`## Untitled chapter`. These heading lines are displayed larger and bold while
their markup remains visible. Kobo punctuation and symbols remain verbatim,
without injected Markdown escapes or HTML entities. Single line breaks inside
highlights are reflowed as spaces, while blank lines remain paragraph boundaries
and note line breaks remain unchanged. The source can be selected and copied.
The reader soft-wraps at a centered maximum of 120 monospaced characters and
adapts to narrower windows without changing the copied source.

## Tests

```bash
ctest --test-dir build --output-on-failure
```

## License

Licensed under the [MIT License](LICENSE).
