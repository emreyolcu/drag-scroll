# DragScroll

[![Downloads](https://img.shields.io/github/downloads/emreyolcu/drag-scroll/total.svg)](https://github.com/emreyolcu/drag-scroll/releases)

This small utility provides a toggleable drag-to-scroll mechanism for macOS.
It runs in the background and does not interfere
until a designated mouse button is pressed (button 4 by default).
Pressing the button toggles drag scrolling,
where the mouse cursor is locked in place
and mouse movement events are interpreted as scrolling events.
This is especially useful with a trackball:
you can toggle on drag scrolling and roll the ball
to quickly scroll through a large website or a document.

### Supported versions

As of May 2024, this application works on macOS versions 10.9–14.0.

### Installation

You may download the binary [here](https://github.com/emreyolcu/drag-scroll/releases/download/v1.0.0/DragScroll.zip).

It needs to be run each time you boot.
If you want this to be automatic, do the following:

1. On macOS 13.0 and later, go to `System Settings > General > Login Items`;
   otherwise, go to `System Preferences > Users & Groups > Login Items`.
2. Add `DragScroll` to the list.

If you want to undo the effect, do the following:

1. Launch `Activity Monitor`.
2. Search for `DragScroll` and select it.
3. Click the stop button in the upper-left corner and choose Quit.

### Configuration

The default button for toggling drag scrolling is mouse button 4.
If you want to use a different mouse button, run the following command,
replacing `BUTTON` with a button number between 2 and 31.
(Button numbers are zero-based,
so 0 and 1 represent left and right mouse buttons.)

```
defaults write com.emreyolcu.DragScroll button -int BUTTON
```

If you want to change scrolling speed, run the following command,
replacing `SCALE` with a small number (default is 3).
This number may even be negative, which inverts scrolling direction.

```
defaults write com.emreyolcu.DragScroll scale -int SCALE
```

You should restart the application for these settings to take effect.

### Potential problems

Recent versions of macOS have made it difficult to run unsigned binaries.

If you experience issues launching the application, try the following:

- Remove the quarantine attribute by running the command
  `xattr -dr com.apple.quarantine /path/to/DragScroll.app`,
  where the path points to the application bundle.
- Disable Gatekeeper by running the command
  `spctl --add /path/to/DragScroll.app`,
  where the path points to the application bundle.

If on startup the application asks for accessibility permissions
even though you have previously granted it access, try the following:

1. On macOS 13.0 and later, go to `System Settings > Privacy & Security > Accessibility`;
   otherwise, go to `System Preferences > Security & Privacy > Privacy > Accessibility`.
2. Remove `DragScroll` from the list and add it again.
