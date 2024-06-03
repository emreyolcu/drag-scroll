# DragScroll

[![Downloads](https://img.shields.io/github/downloads/emreyolcu/drag-scroll/total.svg)](https://github.com/emreyolcu/drag-scroll/releases)

This small utility provides a drag-to-scroll mechanism for macOS.
It runs in the background and does not interfere until
either a mouse button is pressed (button 5 by default)
or some modifier keys are held down (the Shift key by default),
both of which activate drag scrolling mode.
In this mode, the mouse cursor is locked in place
and mouse movement is interpreted as scrolling.
Pressing the mouse button again or releasing the modifier keys
deactivates drag scrolling mode.

This application is especially useful with a trackball:
you can activate drag scrolling and roll the ball
to quickly scroll through a large website or a document.
It also works with the trackpad, for instance allowing you
to drag scroll with a single finger
while holding down the modifier keys.

> [!NOTE]
> The two means of activation operate independently of each other:
> if you first press the mouse button to activate
> and then press and release the modifier keys,
> drag scrolling mode stays active until you press the mouse button again.

### Supported versions

As of May 2024, this application works on macOS versions 10.9–14.0.

### Installation

You may download the binary [here](https://github.com/emreyolcu/drag-scroll/releases/download/v1.2.0/DragScroll.zip).
DragScroll requires access to accessibility features.
Upon startup, if it does not have access, it will prompt you and wait.
You do not need to restart the application
after you grant it access to accessibility features.

> [!CAUTION]
> You should not revoke accessibility access
> for DragScroll while it is running.
> Otherwise, your mouse might become unresponsive, requiring a reboot to fix.

If you want the application to run automatically when you log in,
do the following:

1. On macOS 13.0 and later, go to `System Settings > General > Login Items`;
   otherwise, go to `System Preferences > Users & Groups > Login Items`.
2. Add `DragScroll` to the list.

If you want to quit the application, either run `killall DragScroll`
or do the following:

1. Launch `Activity Monitor`.
2. Search for `DragScroll` and select it.
3. Click the stop button in the upper-left corner and choose Quit.

### Configuration

- **Mouse button**:
  The default mouse button for toggling drag scrolling is button 5.
  If you want to use a different mouse button, run the following command,
  replacing `BUTTON` with a button number between 3 and 32.
  (Button numbers are one-based,
  so 1 and 2 represent left and right mouse buttons.
  You cannot use those in DragScroll.)

  ```
  defaults write com.emreyolcu.DragScroll button -int BUTTON
  ```

  If you do not want to use mouse buttons with DragScroll,
  set `button` to 0.

- **Modifier keys**:
  The default modifier key for activating drag scrolling is the Shift key.
  If you want to use a different set of modifiers, run the following command,
  replacing `[KEYS...]` by a space-separated set of modifier keys
  chosen from among `capslock`, `shift`, `control`, `option`, `command`.
  (Unlike the other modifiers, Caps Lock works as a toggle.)

  ```
  defaults write com.emreyolcu.DragScroll keys -array [KEYS...]
  ```

  For instance, if you want to activate drag scrolling
  by holding down Control and Command together, run:

  ```
  defaults write com.emreyolcu.DragScroll keys -array control command
  ```

  If you do not want to use modifier keys with DragScroll,
  set `keys` to an empty array:

  ```
  defaults write com.emreyolcu.DragScroll keys -array
  ```

- **Scrolling speed:**
  If you want to change scrolling speed, run the following command,
  replacing `SCALE` with a small number (default is 3).
  This number may even be negative, which inverts scrolling direction.

  ```
  defaults write com.emreyolcu.DragScroll scale -int SCALE
  ```

> [!WARNING]
> If you set a preference to an unexpected value (e.g., of the wrong type),
> then its default value is used as a fallback.

You should restart the application for these settings to take effect.

### Uninstallation

To uninstall DragScroll, quit the application, move it to trash,
and remove it from the lists for accessibility access and login items.
You can remove any stored preferences by running the following:

```
defaults delete com.emreyolcu.DragScroll
```

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
