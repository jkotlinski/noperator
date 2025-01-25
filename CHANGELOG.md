# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Unreleased

### Added

- Placeholder music ("lightforce" by Glenn Rune Gallefors)

### Changed

- Keyframe editor key presses
    - Space: go to next keyframe
    - Shift+Space: go to previous keyframe
    - Right: go to next key
    - 1-9: set beat count

### Fixed

- Directory listing for many files.

## v2.0.0 - 2016-12-17

### Fixed

- Reset reverse, background colors and character rotation at movie start.

### Removed

- Undocumented Ctrl+B keypress to reset cursor (?).

### Added

New key presses:

- F7, m: mirror characters vertically around cursor midpoint. Exit mirror mode by F7, m/M.
- F7, M: mirror characters vertically around cursor right edge. Exit mirror mode by F7, m/M.
- F7, cursor, 0-9, character: rotate character. 0-9 sets speed; 0 means off, 1 is fastest, 9 is slowest.

## v1.1.1 - 2016-12-04

### Fixed

- Fixed crash when copy-pasting on bottom row.
- Screen was erroneously cleared after loading animation.

## v1.1.0 - 2016-07-02

### Added

It is now possible to rotate characters. These new key presses were added to animation editor:

 - F7, char: rotate char up
 - F7, F7, char: rotate char right
 - F7, F7, F7, char: rotate char down
 - F7, F7, F7, F7, char: rotate char left
 - F7, F7, F7, F7, F7, char: stop rotating char

## v1.0 - 2014-10-28

### Added

- Initial release.
