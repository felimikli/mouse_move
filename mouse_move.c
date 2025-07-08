#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <libudev.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>
#include <sys/ioctl.h>
#include <linux/input.h>

#include "config.h"

typedef struct {
	struct libevdev_uinput* uidev;
	int* key_states;
	int speed;
	struct timespec click_tick;
	struct timespec scroll_tick;
	struct timespec motion_tick;

	bool button_left_pressed;
	bool button_middle_pressed;
	bool button_right_pressed;
} Mouse;

uint64_t time_diff_ns_abs(struct timespec* x, struct timespec* y) {
	return llabs((x->tv_sec - y->tv_sec)*1e9 + (x->tv_nsec - y->tv_nsec));
}

void handle_motion(Mouse* m) {
	int x = 0;
	int y = 0;
	if(m->key_states[K_UP]) y--;
	if(m->key_states[K_DOWN]) y++;
	if(m->key_states[K_LEFT]) x--;
	if(m->key_states[K_RIGHT]) x++;
	if(x == 0 && y == 0) return;
	
	m->speed = SPEED_NORMAL;
	if(m->key_states[FAST_MOD]) m->speed = SPEED_FAST;
	if(m->key_states[SLOW_MOD]) m->speed = SPEED_SLOW;
	if(m->key_states[SLOWER_MOD]) m->speed = SPEED_SLOWER;

	int x_pixels = x * (int)( ((float)m->speed) * (MOTION_DELAY_NS / 1e9) );
	int y_pixels = y * (int)( ((float)m->speed) * (MOTION_DELAY_NS / 1e9) );

	libevdev_uinput_write_event(m->uidev, EV_REL, REL_X, x_pixels);
	libevdev_uinput_write_event(m->uidev, EV_REL, REL_Y, y_pixels);
	libevdev_uinput_write_event(m->uidev, EV_SYN, SYN_REPORT, 0);
}

void handle_scroll(Mouse* m) {
	int x = 0;
	int y = 0;
	if(m->key_states[K_SCROLL_UP]) y++;
	if(m->key_states[K_SCROLL_DOWN]) y--;
	if(m->key_states[K_SCROLL_LEFT]) x--;
	if(m->key_states[K_SCROLL_RIGHT]) x++;
	if(x == 0 && y == 0) return;

	libevdev_uinput_write_event(m->uidev, EV_REL, REL_WHEEL, y);
	libevdev_uinput_write_event(m->uidev, EV_REL, REL_HWHEEL, x);
	libevdev_uinput_write_event(m->uidev, EV_SYN, SYN_REPORT, 0);
}

void update_button(struct libevdev_uinput* uidev, bool is_pressed, bool* pressed_flag, int ui_btn) {
	if(is_pressed != *pressed_flag) {
			*pressed_flag = is_pressed;
			libevdev_uinput_write_event(uidev, EV_KEY, ui_btn, is_pressed);
			libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0);
	}
}
void handle_click(Mouse* m) {
	update_button(m->uidev, (bool) m->key_states[K_BUTTON_LEFT], &m->button_left_pressed, BTN_LEFT);
	update_button(m->uidev, (bool) m->key_states[K_BUTTON_MIDDLE], &m->button_middle_pressed, BTN_MIDDLE);
	update_button(m->uidev, (bool) m->key_states[K_BUTTON_RIGHT], &m->button_right_pressed, BTN_RIGHT);
}


void handle_mouse(Mouse* m) {
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

	if(time_diff_ns_abs(&now, &m->click_tick) > CLICK_DELAY_NS) {
		handle_click(m);
		clock_gettime(CLOCK_MONOTONIC, &m->click_tick);
	}

	if(time_diff_ns_abs(&now, &m->scroll_tick) > SCROLL_DELAY_NS) {
		handle_scroll(m);
		clock_gettime(CLOCK_MONOTONIC, &m->scroll_tick);
	}

	if(time_diff_ns_abs(&now, &m->motion_tick) > MOTION_DELAY_NS) {
		handle_motion(m);
		clock_gettime(CLOCK_MONOTONIC, &m->motion_tick);
	}
}


int create_uinput_mouse_dev(int uifd, struct libevdev_uinput** ui_mouse_dev) {
	struct libevdev* mouse_dev;
	int err;
	mouse_dev = libevdev_new();
	libevdev_set_name(mouse_dev, "test device");
	libevdev_enable_event_type(mouse_dev, EV_REL);
	libevdev_enable_event_code(mouse_dev, EV_REL, REL_X, NULL);
	libevdev_enable_event_code(mouse_dev, EV_REL, REL_Y, NULL);
	libevdev_enable_event_code(mouse_dev, EV_REL, REL_WHEEL, NULL);
	libevdev_enable_event_code(mouse_dev, EV_REL, REL_HWHEEL, NULL);

	libevdev_enable_event_type(mouse_dev, EV_KEY);
	libevdev_enable_event_code(mouse_dev, EV_KEY, BTN_LEFT, NULL);
	libevdev_enable_event_code(mouse_dev, EV_KEY, BTN_MIDDLE, NULL);
	libevdev_enable_event_code(mouse_dev, EV_KEY, BTN_RIGHT, NULL);
	err = libevdev_uinput_create_from_device(mouse_dev, uifd, ui_mouse_dev);
	libevdev_free(mouse_dev);
	return err;
}

int find_keyboard_device_path(char* buff, size_t size) {
	int fd;
	int rc;
	struct libevdev* dev = NULL;

	for(int i = 0; i < MAX_DEVICES; i++) { 
		snprintf(buff, size, "/dev/input/event%d", i);

		fd = open(buff, O_RDONLY | O_NONBLOCK);
		if(fd == -1) {
			// If no such file, device does not exist, skip
			if (errno == ENOENT)
				continue;
			// Permission denied or other errors - skip but warn maybe
			if (errno == EACCES || errno == EPERM) {
				fprintf(stderr, "Permission denied opening %s, skipping\n", buff);
				continue;
			}
			// Unexpected error: stop scanning
			perror("Error opening device");
			break;
		}
		rc = libevdev_new_from_fd(fd, &dev);
		if (rc < 0) {
			close(fd);
			fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
			return 1;
		}
		if(libevdev_has_event_code(dev, EV_KEY, KEY_A) && libevdev_has_event_type(dev, EV_KEY) && libevdev_has_event_type(dev, EV_REP)) {
			libevdev_free(dev);
			close(fd);
			return 0;
		}
		libevdev_free(dev);
		close(fd);
	}
	fprintf(stderr, "No keyboard device found\n");
	return 1;
}

int check_keyboard_device_path(char* buff) {
	int fd;
	int rc;
	struct libevdev* dev = NULL;

	fd = open(buff, O_RDONLY | O_NONBLOCK);
	if(fd == -1) {
		// If no such file, device does not exist, skip
		if (errno == ENOENT){
			fprintf(stderr, "No keyboard device found\n");
		}
		// Permission denied or other errors - skip but warn maybe
		if (errno == EACCES || errno == EPERM) {
			fprintf(stderr, "Permission denied opening %s, if this is your keyboard device, make sure to run the program with root access (sudo)\n", buff);
		}
		perror("Error opening device");
		return 1;
	}
	rc = libevdev_new_from_fd(fd, &dev);
	if (rc < 0) {
		close(fd);
		fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
		return 1;
	}
	if(libevdev_has_event_code(dev, EV_KEY, KEY_A) && libevdev_has_event_type(dev, EV_KEY) && libevdev_has_event_type(dev, EV_REP)) {
		libevdev_free(dev);
		close(fd);
		return 0;
	}
	libevdev_free(dev);
	close(fd);
	fprintf(stderr, "Device provided does not look like a keyboard\n");
	return 1;
}


void grab_keyboard(int fd) {
	if(ioctl(fd, EVIOCGRAB, 1) < 0) {
		perror("Failed to grab input device");
		close(fd);
		return;
	}
}

void ungrab_keyboard(int fd) {
	ioctl(fd, EVIOCGRAB, 0);
	close(fd);
}


void release_keys(int fd) {
	uint8_t keys[KEY_STATE_MAX];
	memset(keys, 0, sizeof(keys));

	bool any_press = true;
	while(any_press == true) {
		ioctl(fd, EVIOCGKEY(sizeof(keys)), keys);
		any_press = false;
		for(int i = 0; i < KEY_STATE_MAX; i++) {
			if(keys[i]) {
				any_press = true;
				break;
			}
		}
	}
}

int main() {
	int err;
	int fd;
	int uifd;

	struct libevdev_uinput* ui_mouse_dev;

	char keyboard_path[64];

	struct input_event event;
	int quit = 0;
	ssize_t n;

	int key_states[KEY_MAX] = {0};


	if(strlen(KEYBOARD_DEVICE) >= sizeof keyboard_path) {
		fprintf(stderr, "keyboard device path provided is too long.\n");
		keyboard_path[0] = '\0';
	}
	else {
		strncpy(keyboard_path, KEYBOARD_DEVICE, sizeof(keyboard_path) - 1);
		keyboard_path[sizeof(keyboard_path) - 1] = '\0';
	}
	if(keyboard_path[0] == '\0') {
		err = find_keyboard_device_path(keyboard_path, sizeof keyboard_path);
		if(err != 0){
			fprintf(stderr, "Error guessing keyboard device, please provide device path in config.h\n");
			return 1;
		}
	}
	else {
		err = check_keyboard_device_path(keyboard_path);
		if(err != 0){
			fprintf(stderr, "Incorrect path provided in config.h, make sure its a keyboard device path\n");
			return 1;
		}
	}

	fd = open(keyboard_path, O_RDONLY | O_NONBLOCK);
	if(fd < 0) {
		perror("Error opening input device");
		return 1;
	}

	uifd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

	err = create_uinput_mouse_dev(uifd, &ui_mouse_dev);
	if(err != 0) {
		perror("Error creating mouse device");
		return 1;
	}

	Mouse m;
	m.uidev = ui_mouse_dev;
	m.key_states = key_states;
	m.speed = SPEED_NORMAL;
	m.click_tick.tv_sec = 0;
	m.click_tick.tv_nsec = 0;
	m.scroll_tick.tv_sec = 0;
	m.scroll_tick.tv_nsec = 0;
	m.motion_tick.tv_sec = 0;
	m.motion_tick.tv_nsec = 0;

	release_keys(fd);
	grab_keyboard(fd);
	while(!quit) {
		n = read(fd, &event, sizeof(event));

		if(n == (ssize_t)sizeof(event)) {
			if(event.type == EV_KEY) {
				if(event.code == K_EXIT) {
					quit = 1;
				}
				m.key_states[event.code] = event.value;
			}
		} else {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				perror("Error reading event");
				quit = 1;
				break;
			}
		}
		handle_mouse(&m);
	}
	ungrab_keyboard(fd);

	libevdev_uinput_destroy(ui_mouse_dev);
	close(uifd);
	close(fd);
	return 0;
}
