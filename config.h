/*
 * IMPORTANT:
 * this program uses sudo and root access to read keyboard devices
 * this is bad and insecure so read the source code and use it at your own risk
 */

/*
 * Install:
 * Clone the repository or just download the files
 * Edit the config.h file to your liking.
 * sudo make install
 */


/*
 * Path to the keyboard input device.
 *
 * You can set this manually by finding your keyboard:
 *
 *   - Run: ls /dev/input/by-id/
 *     Look for a device like: usb-Logitech_USB_Keyboard-event-kbd
 *     Then set:
 *       #define KEYBOARD_DEVICE "/dev/input/by-id/usb-Logitech_USB_Keyboard-event-kbd"
 *
 *   - Or run: cat /proc/bus/input/devices
 *     Look for a keyboard with EV=120013 (or similar) and note the "eventX".
 *     Then set:
 *       #define KEYBOARD_DEVICE "/dev/input/eventX"
 *
 * If you leave this as an empty string (""), the program will try to auto-detect
 * a keyboard using libevdev by scanning devices in /dev/input.
 */
#define KEYBOARD_DEVICE ""

#define MAX_DEVICES 64

/* 
 * Note that most probably the keyboard layout from the evdev device is qwerty.
 * If you have changed your layout, take that into account when setting this bindings.
 *
 * Example:
 *	If you want to click with T, but you changed your layout to dvorak,
 *	you should set:
 *		K_BTN_LEFT KEY_K
 *	because T in dvorak is in the position of K in qwerty
 *
 * Alternatively mess around with the bindings until you find the setup you want :)
 *	
 *
 * Support for automatic layout detection may be coming soon...
 */

/*
 * Usage:
 * Either manually run 
 * mouse_move 
 * in the terminal,
 * or setup a shortcut for it
 *
 * example for i3:
 * bindsym $mod+space exec mouse_move
 *
 *
 * Once the program is running, press the START_TOGGLE key combo to grab the keyboard
 * while grabbing the keyboard you can move the mouse
 * to stop grabbing the keyboard press the EKIT key
 * to completely kill the program press the END key while grabbing the keyboard.
 */

/*
 * The START_TOGGLE_1 must always be a key
 * if you want to just use that one key to start grabbing the keyboard,
 * set 2 and 3 to -1
 * if not you can activate each one by just assigning the keys you'd like for your combo
 */
#define START_TOGGLE_1	KEY_TAB
#define START_TOGGLE_2	KEY_LEFTMETA
#define START_TOGGLE_3	-1

#define K_END		KEY_X

#define M_CTRL		KEY_LEFTCTRL
#define M_SHIFT		KEY_LEFTSHIFT
#define M_ALT		KEY_LEFTALT

#define K_EXIT		KEY_SPACE

#define K_UP		KEY_K
#define K_DOWN		KEY_J
#define K_LEFT		KEY_H
#define K_RIGHT		KEY_L

#define K_BUTTON_LEFT	KEY_S
#define K_BUTTON_MIDDLE	KEY_D
#define K_BUTTON_RIGHT	KEY_F

#define K_SCROLL_UP	KEY_U
#define K_SCROLL_DOWN	KEY_D

#define K_SCROLL_LEFT	KEY_B
#define K_SCROLL_RIGHT	KEY_W


#define SLOWER_MOD	M_CTRL
#define SLOW_MOD	M_SHIFT
#define FAST_MOD	M_ALT


// motion speed in pixels per second
#define SPEED_SLOWER	100
#define SPEED_SLOW	400
#define SPEED_NORMAL	800
#define SPEED_FAST	1200

// scroll speed in detents per second
#define SCROLL_SPEED_SLOWER	8
#define SCROLL_SPEED_SLOW	12
#define SCROLL_SPEED_NORMAL	20
#define SCROLL_SPEED_FAST	30



#define CLICK_DELAY_MS	25
#define SCROLL_DELAY_MS	20
#define MOTION_DELAY_MS	10 // less is smoother, anything less than 3 will result in no motion.
