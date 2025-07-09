#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <libudev.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>
#include <sys/ioctl.h>
#include <linux/input.h>

#include "config.h"

#define KEY_STATE_MAX ((KEY_MAX + 7) / 8)

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define CLICK_DELAY_NS  (CLICK_DELAY_MS * 1000000)
#define SCROLL_DELAY_NS (SCROLL_DELAY_MS * 1000000)
#define MOTION_DELAY_NS (MOTION_DELAY_MS * 1000000)

#define POLL_DELAY_MS MIN(MIN(CLICK_DELAY_MS, SCROLL_DELAY_MS), MOTION_DELAY_MS)


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
	uint64_t nx = (uint64_t)x->tv_sec * 1000000000ULL + x->tv_nsec;
	uint64_t ny = (uint64_t)y->tv_sec * 1000000000ULL + y->tv_nsec;
	return nx > ny ? nx - ny : ny - nx;
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

	float motion_factor = m->speed * (MOTION_DELAY_NS / 1e9f);
	int x_pixels = (int) ( x * motion_factor );
	int y_pixels = (int) ( y * motion_factor );

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


struct libevdev_uinput* create_uinput_mouse_dev(int uifd) {
	struct libevdev_uinput* ui_mouse_dev;
	struct libevdev* mouse_dev;
	int err;

	mouse_dev = libevdev_new();
	if(mouse_dev == NULL) {
		perror("Error creating virtual mouse device");
		return NULL;
	}
	libevdev_set_name(mouse_dev, "virtual mouse");
	libevdev_enable_event_type(mouse_dev, EV_REL);
	libevdev_enable_event_code(mouse_dev, EV_REL, REL_X, NULL);
	libevdev_enable_event_code(mouse_dev, EV_REL, REL_Y, NULL);
	libevdev_enable_event_code(mouse_dev, EV_REL, REL_WHEEL, NULL);
	libevdev_enable_event_code(mouse_dev, EV_REL, REL_HWHEEL, NULL);

	libevdev_enable_event_type(mouse_dev, EV_KEY);
	libevdev_enable_event_code(mouse_dev, EV_KEY, BTN_LEFT, NULL);
	libevdev_enable_event_code(mouse_dev, EV_KEY, BTN_MIDDLE, NULL);
	libevdev_enable_event_code(mouse_dev, EV_KEY, BTN_RIGHT, NULL);

	err = libevdev_uinput_create_from_device(mouse_dev, uifd, &ui_mouse_dev);
	if(err != 0) {
		perror("Error creating uinput mouse device");
		return NULL;
	}
	libevdev_free(mouse_dev);
	return ui_mouse_dev;
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


int grab_keyboard(int fd) {
	int err = ioctl(fd, EVIOCGRAB, 1);
	if(err < 0) {
		perror("Failed to grab input device");
	}
	return err;
}

void ungrab_keyboard(int fd) {
	int err = ioctl(fd, EVIOCGRAB, 0);
	if(err < 0) {
		perror("Failed to ungrab input device");
	}
	return err;
}


bool are_keys_released(int fd) {
	uint8_t keys[KEY_STATE_MAX];
	if(ioctl(fd, EVIOCGKEY(sizeof(keys)), keys) < 0) {
		perror("EVIOCGKEY failed");
		return false;
	}
	for(int i = 0; i < KEY_STATE_MAX; i++) {
		if(keys[i]) {
			return false;
		}
	}
	return true;
}


bool key_combo(int* key_states, int* keys, size_t n) {
	if(!key_states[keys[0]]) return false; // first key is required
	// rest are optional, check only if != -1
	for(size_t i = 1; i < n; i++) {
		if(keys[i] != -1 && !key_states[keys[i]]) { // if key is NOT -1 AND the key is not pressed
			return false;
		}
	}
	return true;
}

int main() {
	int err;
	int keyboard_fd;
	int uifd_mouse;
	struct libevdev_uinput* ui_mouse_dev;

	char keyboard_path[64];

	struct input_event event;
	bool quit = false;
	ssize_t n;

	Mouse m;
	m.key_states = calloc(KEY_MAX + 1, sizeof(int));
	m.speed = SPEED_NORMAL;
	m.click_tick.tv_sec = 0;
	m.click_tick.tv_nsec = 0;
	m.scroll_tick.tv_sec = 0;
	m.scroll_tick.tv_nsec = 0;
	m.motion_tick.tv_sec = 0;
	m.motion_tick.tv_nsec = 0;
	m.button_left_pressed = false;
	m.button_middle_pressed = false;
	m.button_right_pressed = false;

	bool grabbing = false;

	bool start_combo;
	int start_combo_keys[] = { START_TOGGLE_1, START_TOGGLE_2, START_TOGGLE_3 };
	int start_combo_keys_size = sizeof(start_combo_keys) / sizeof(start_combo_keys[0]);

	struct pollfd fds[1];
	fds[0].events = POLLIN;
	int poll_ret;


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

	keyboard_fd = open(keyboard_path, O_RDONLY | O_NONBLOCK);
	if(keyboard_fd < 0) {
		perror("Error opening input device");
		return 1;
	}
	fds[0].fd = keyboard_fd;

	uifd_mouse = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	ui_mouse_dev = create_uinput_mouse_dev(uifd_mouse);
	if(ui_mouse_dev == NULL) {
		perror("Error creating mouse device");
		return 1;
	}
	m.uidev = ui_mouse_dev;

	while(!quit) {
		poll_ret = poll(fds, 1, POLL_DELAY_MS);
		if(poll_ret > 0) {
			n = read(keyboard_fd, &event, sizeof(event));

			if(n == (ssize_t)sizeof(event)) {
				if(event.type == EV_KEY) {
					m.key_states[event.code] = event.value;
					start_combo = key_combo(m.key_states, start_combo_keys, start_combo_keys_size);

					if(start_combo && !grabbing) {
						grabbing = true;
						while(1) {
							err = grab_keyboard(keyboard_fd);
							if(err < 0) {
								perror("error grabbing keyboard");
								usleep(1000);
								continue;
							}
							if(are_keys_released(keyboard_fd)) {
								memset(m.key_states, 0, sizeof(int) * KEY_MAX);
								break;
							}
							err = ungrab_keyboard(keyboard_fd);
							if(err < 0) {
								perror("error ungrabbing keyboard");
								usleep(1000);
								continue;
							}
							usleep(1000);
						}
					}

					if(grabbing && event.code == K_EXIT) {
						memset(m.key_states, 0, sizeof(int) * KEY_MAX);
						grabbing = false;
						ungrab_keyboard(keyboard_fd);
					}

					if(grabbing && event.code == K_END) {
						quit = true;
						ungrab_keyboard(keyboard_fd);
					}

				}
			} else {
				if (errno != EAGAIN && errno != EWOULDBLOCK) {
					perror("Error reading event");
					quit = true;
					break;
				}
			}
		}
		else if(poll_ret < 0) {
			perror("poll failed");
			quit = true;
			break;
		}
		if(grabbing) handle_mouse(&m);
	}
	libevdev_uinput_destroy(ui_mouse_dev);
	close(uifd_mouse);
	close(keyboard_fd);
	free(m.key_states);
	return 0;
}
