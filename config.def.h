/*
 * config.h — Configuration for mouse_move
 *
 * This file contains all configurable settings for the mouse_move program,
 * which enables moving the mouse cursor using the keyboard.
 *
 * ─────────────────────────────────────────────────────────────────────────────
 *  SECURITY WARNING:
 *  This program requires sudo/root privileges to access input devices.
 *  Always review the source code before running with elevated permissions.
 * ─────────────────────────────────────────────────────────────────────────────
 *  INSPIRATION:
 *
 *  This project is greatly inspired by the Go-based program:
 *      "mouseless" by jbensmann — https://github.com/jbensmann/mouseless
 *
 *  This C version reimplements similar functionality with a focus on minimal
 *  dependencies, performance, and simplicity for low-level users.
 *  Highly recommend checking mouseless for a better product overall.
 *
 * ─────────────────────────────────────────────────────────────────────────────
 *
 * USAGE:
 *  • Edit this file to customize keybindings, device paths, and speed settings.
 *  • Compile and install with:
 *      sudo make install
 *  • Launch automatically (recommended) via:
 *      - .xinitrc
 *      - your i3 config
 *      - systemd user service
 *    or run manually with:
 *      sudo mouse_move
 *
 *  • To begin controlling the mouse, press the START_COMBO_KEYS.
 *  • To exit mouse control mode (without quitting the program), press K_EXIT.
 *  • To terminate the program completely, use K_END (rarely needed).
 *
 * ─────────────────────────────────────────────────────────────────────────────
 *  NO ROOT MODE:
 *
 *  By default, the program must be run with `sudo` because:
 *      • Reading raw keyboard events requires root.
 *      • Creating a virtual mouse/keyboard device through uinput also requires root.
 *
 *  To run without sudo, add the following `udev` rules:
 *
 *  1. Create a file at:
 *      /etc/udev/rules.d/99-$USER.rules
 *
 *  2. Paste the following content:
 *
 *      KERNEL=="uinput", GROUP="$USER", MODE:="0660"
 *      KERNEL=="event*", GROUP="$USER", NAME="input/%k", MODE="0660"
 *
 *  3. Then reload your system:
 *      - Reboot your machine.
 *
 *  If it still doesn't work, the uinput module might not be loaded. You can fix this with:
 *
 *      echo "uinput" | sudo tee /etc/modules-load.d/uinput.conf
 *
 *  Then reboot again.
 *
 * ─────────────────────────────────────────────────────────────────────────────
 *  DEVICE PATH:
 *
 *  Manually set KEYBOARD_DEVICE to your keyboard input path for best accuracy:
 *
 *      #define KEYBOARD_DEVICE "/dev/input/by-id/usb-Logitech_USB_Keyboard-event-kbd"
 *      or
 *      #define KEYBOARD_DEVICE "/dev/input/event3"
 *
 *  To find your keyboard:
 *      • ls /dev/input/by-id/
 *      • cat /proc/bus/input/devices
 *        Look for entries with EV=120013 or similar.
 *
 *  Leaving KEYBOARD_DEVICE as "" enables auto-detection (not always reliable).
 *
 * ─────────────────────────────────────────────────────────────────────────────
 *  KEYBOARD LAYOUT:
 *
 *  This config assumes a QWERTY layout (evdev keycodes). If using another layout
 *  like Dvorak or Colemak, manually remap keybindings below.
 *
 *  Example: On Dvorak, to use 'T' (which is KEY_K in QWERTY):
 *      #define K_BUTTON_LEFT KEY_K
 *
 */

#ifndef CONFIG_H
#define CONFIG_H

/******************************************************************************
 * DEVICE SETTINGS
 ******************************************************************************/

/* Path to keyboard device — set manually or leave "" for auto-detect */
#define KEYBOARD_DEVICE ""

/* Maximum number of input devices to scan during auto-detect */
#define MAX_DEVICES 64


/******************************************************************************
 * KEY BINDINGS
 ******************************************************************************/

/* Emergency kill key — terminates program completely */
#define K_END       KEY_X

/* Start combo — press all keys in this array simultaneously */
#define START_COMBO_KEYS { KEY_TAB, KEY_LEFTMETA }

/* Exit control mode without quitting the program */
#define K_EXIT      KEY_SPACE

/* Modifier keys used for speed control */
#define M_CTRL      KEY_LEFTCTRL
#define M_SHIFT     KEY_LEFTSHIFT
#define M_ALT       KEY_LEFTALT

/* Mouse movement */
#define K_UP        KEY_K
#define K_DOWN      KEY_J
#define K_LEFT      KEY_H
#define K_RIGHT     KEY_L

/* Mouse buttons */
#define K_BUTTON_LEFT     KEY_S
#define K_BUTTON_MIDDLE   KEY_D
#define K_BUTTON_RIGHT    KEY_F

/* Scroll directions */
#define K_SCROLL_UP       KEY_U
#define K_SCROLL_DOWN     KEY_D
#define K_SCROLL_LEFT     KEY_B
#define K_SCROLL_RIGHT    KEY_W


/******************************************************************************
 * SPEED SETTINGS
 ******************************************************************************/

/* Speed modifiers (used with movement/scroll keys) */
#define SLOWER_MOD   M_CTRL
#define SLOW_MOD     M_SHIFT
#define FAST_MOD     M_ALT

/* Movement speeds (in pixels per second) */
#define SPEED_SLOWER   100
#define SPEED_SLOW     400
#define SPEED_NORMAL   800
#define SPEED_FAST     1200

/* Scrolling speeds (scroll steps per second) */
#define SCROLL_SPEED_SLOWER   8
#define SCROLL_SPEED_SLOW     12
#define SCROLL_SPEED_NORMAL   20
#define SCROLL_SPEED_FAST     30


/******************************************************************************
 * TIMING & DELAYS
 ******************************************************************************/

/* Minimum delay between clicks (ms) */
#define CLICK_DELAY_MS    25

/* Scroll update interval (ms) */
#define SCROLL_DELAY_MS   20

/* Motion update interval (ms) — lower = smoother, but <3ms may break motion */
#define MOTION_DELAY_MS   10


#endif /* CONFIG_H */
