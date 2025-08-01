# mouse_move

**mouse_move** is a minimal, single-file C program that lets you move the mouse cursor using your keyboard.  
Designed for Linux users who value simplicity, speed, and control — especially on minimal setups like i3 or other tiling window managers.

---

## Features

- Keyboard-based mouse movement, mouse buttons and scrollig
- Configurable keybindings, speeds, and device paths
- Fast, minimal, and dependency-free
- Can run without root using udev rules
- Just one C file + one config file

---

## Inspired By

This project is inspired by:

- [mouseless](https://github.com/jbensmann/mouseless) — a Go-based program with similar functionality

This C version is a ground-up rewrite focusing on low-level simplicity and speed.

---

## Security Notice

This program reads raw input events and creates a virtual device via uinput.  
**It requires root privileges** unless configured otherwise.  
Always read and understand the code before running with elevated permissions.

---

## Installation

Clone and build:

```
git clone https://felimikli/mouse_move
cd mouse_move
sudo make install
```

Then make sure to have a config.h file:
```
cp config.def.h config.h
```

---

## Configuration

Edit `config.h` to adjust all settings:

- Keybindings (e.g. HJKL for movement)
- Speed levels
- Device paths (keyboard input)
- Combo keys to enter/exit control mode

---

## Usage

Run the program:

```
sudo mouse_move
```

Or launch automatically from:

- `.xinitrc`
- your i3 config
- a user systemd service

Control flow:

- Press the `START_COMBO_KEYS` to enter mouse control mode
- Use the keybindings you defined to move and click
- Press `EXIT_COMBO_KEYS` to pause control mode
- Press `KILL_COMBO_KEYS` to fully terminate the program (rarely needed)

---

## No Root Mode (via udev rules)

To run without `sudo`, follow these steps:

1. Create the udev rules file:

```
sudo nano /etc/udev/rules.d/99-input.rules
```

Paste this:

```
KERNEL=="uinput", GROUP="input", MODE="0660"
KERNEL=="event*", GROUP="input", MODE="0660"
```

2. Add your user to the `input` group:

```
sudo usermod -aG input $USER
```

3. Reload and trigger the rules:

```
sudo udevadm control --reload-rules
sudo udevadm trigger
```

4. Make sure `uinput` is loaded at boot:

```
echo "uinput" | sudo tee /etc/modules-load.d/uinput.conf
sudo modprobe uinput
```

5. Reboot.

Now you should be able to run the program without `sudo`.

---

## Device Setup

Set the `KEYBOARD_DEVICE` manually in `config.h` for better reliability:

```c
#define KEYBOARD_DEVICE "/dev/input/event3"
```

Leave it blank to enable auto-detection:

```c
#define KEYBOARD_DEVICE ""
```

To find your keyboard device:

- Run: `ls /dev/input/by-id/`
- Or check: `cat /proc/bus/input/devices`
- Look for event nodes tied to `EV=120013` or similar

---

## License

This project is licensed under the **GNU General Public License v3.0**.  
See the `LICENSE` file or https://www.gnu.org/licenses/gpl-3.0.html for more.

---

## Final Notes

This tool is small by design. If you want more features, look at `mouseless`

---
