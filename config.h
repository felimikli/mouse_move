/*
 * config.h - Configuration file for mouse_move
 *
 * This file contains all configurable settings for the mouse_move program,
 * which enables moving the mouse cursor using the keyboard.
 *
 * IMPORTANT SECURITY NOTE:
 * This program requires sudo/root privileges to read keyboard input devices.
 * Running programs with elevated privileges is a security risk.
 * Review the source code carefully before using.
 *
 * USAGE:
 *  - Edit this file to customize key bindings, device paths, and speed settings.
 *  - Compile and install using: sudo make install
 *  - For most cases, run the program in the background and start it automatically
 *    at system startup (e.g., via .xinitrc, your i3 config, or systemd user service).
 *  - You can still run it manually. Just type mouse_move
 *
 *  - Use the START_TOGGLE key combo to grab the keyboard and control the mouse.
 *  - Use the EXIT key to release control without terminating the program.
 *
 *  - The K_END key is provided to force terminate the program if absolutely necessary,
 *    but this should rarely be needed.
 *
 * DEVICE PATH:
 * To specify the keyboard device, you can either:
 *  1. Manually set KEYBOARD_DEVICE to the path of your keyboard device (recommended).
 *     Examples:
 *       #define KEYBOARD_DEVICE "/dev/input/by-id/usb-Logitech_USB_Keyboard-event-kbd"
 *     or
 *       #define KEYBOARD_DEVICE "/dev/input/event3"
 *
 *     To find your keyboard device:
 *       - Run: ls /dev/input/by-id/
 *       - Or check: cat /proc/bus/input/devices
 *         Look for the keyboard entry with EV=120013 or similar.
 *
 *  2. Leave KEYBOARD_DEVICE as an empty string ("") to enable auto-detection
 *     (may not always pick the correct device).
 *
 * KEYBOARD LAYOUT:
 * The program expects a QWERTY layout by default since it reads keys from evdev devices.
 * If you use a different layout (e.g., Dvorak), remap keys accordingly.
 * For example, to bind click to 'T' on Dvorak (which maps to 'K' on QWERTY), use:
 *   #define K_BTN_LEFT KEY_K
 *
 * Future versions may add automatic layout detection.
 *
 ***********************************************************************
 *                    BEGIN CONFIGURATION SETTINGS                    *
 ***********************************************************************
 */

#ifndef CONFIG_H
#define CONFIG_H

/* 
 * Path to the keyboard input device.
 * Set this manually for best results, or leave as "" for auto-detection.
 */
#define KEYBOARD_DEVICE ""

/* Maximum number of input devices to scan during auto-detection */
/* Shouldn't be changed for the most part */
#define MAX_DEVICES 64


/*
 * KEY BINDINGS
 * Modify these to change which keys control mouse movement, clicks,
 * scrolling, and program control.
 * Key codes are from evdev and correspond to a QWERTY layout by default.
 */

#define K_END       KEY_X      /* Kill the program (should rarely be needed if run in background) */

/* Start grabbing keyboard combo */
/* Put all the keys that need to be pressed for this combo in the Array */
#define START_COMBO_KEYS { KEY_TAB, KEY_LEFTMETA }


#define K_EXIT      KEY_SPACE  /* Release grabbing and exit mouse control mode */

/* Modifier keys for speed adjustment */
#define M_CTRL      KEY_LEFTCTRL
#define M_SHIFT     KEY_LEFTSHIFT
#define M_ALT       KEY_LEFTALT

/* Mouse movement keys */
#define K_UP        KEY_K
#define K_DOWN      KEY_J
#define K_LEFT      KEY_H
#define K_RIGHT     KEY_L

/* Mouse buttons */
#define K_BUTTON_LEFT    KEY_S
#define K_BUTTON_MIDDLE  KEY_D
#define K_BUTTON_RIGHT   KEY_F

/* Scroll keys */
#define K_SCROLL_UP      KEY_U
#define K_SCROLL_DOWN    KEY_D
#define K_SCROLL_LEFT    KEY_B
#define K_SCROLL_RIGHT   KEY_W


/*
 * SPEED MODIFIERS
 * Hold these modifier keys while moving or scrolling to adjust speed.
 */
#define SLOWER_MOD   M_CTRL
#define SLOW_MOD     M_SHIFT
#define FAST_MOD     M_ALT

/*
 * MOTION SPEEDS (pixels per second)
 */
#define SPEED_SLOWER  100
#define SPEED_SLOW   400
#define SPEED_NORMAL  800
#define SPEED_FAST   1200

/*
 * SCROLL SPEEDS (scroll steps per second)
 */
#define SCROLL_SPEED_SLOWER 8
#define SCROLL_SPEED_SLOW   12
#define SCROLL_SPEED_NORMAL 20
#define SCROLL_SPEED_FAST   30

/*
 * DELAYS (milliseconds)
 * Controls responsiveness and smoothness.
 * - CLICK_DELAY_MS: Minimum interval between clicks.
 * - SCROLL_DELAY_MS: Scroll event refresh rate.
 * - MOTION_DELAY_MS: Mouse movement refresh rate (lower = smoother).
 *   Values below 3 may cause motion to fail.
 */
#define CLICK_DELAY_MS  25
#define SCROLL_DELAY_MS 20
#define MOTION_DELAY_MS 10


/*
 ***********************************************************************
 *                            END CONFIGURATION                        *
 ***********************************************************************
 */

#endif /* CONFIG_H */
