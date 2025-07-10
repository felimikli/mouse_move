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

#define MAX_DEVICE_PATH_SIZE 64
#define KEY_STATE_MAX ((KEY_MAX + 7) / 8)
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#include "config.h"

#define CLICK_DELAY_NS  (CLICK_DELAY_MS * 1000000)
#define SCROLL_DELAY_NS (SCROLL_DELAY_MS * 1000000)
#define MOTION_DELAY_NS (MOTION_DELAY_MS * 1000000)

#define POLL_DELAY_MS MIN(MIN(CLICK_DELAY_MS, SCROLL_DELAY_MS), MOTION_DELAY_MS)

static const int START_COMBO_KEYS[] = { START_TOGGLE_1, START_TOGGLE_2, START_TOGGLE_3 };
static const size_t START_COMBO_KEYS_SIZE = sizeof(START_COMBO_KEYS) / sizeof(START_COMBO_KEYS[0]);

typedef struct {
	struct libevdev_uinput* uidev;
	int* key_states;
	int speed;
	int scroll_speed;
	float scroll_fraction_x;
	float scroll_fraction_y;
	struct timespec click_tick;
	struct timespec scroll_tick;
	struct timespec motion_tick;
	bool button_left_pressed;
	bool button_middle_pressed;
	bool button_right_pressed;
} Mouse;

static uint64_t time_diff_ns(struct timespec* x, struct timespec* y) {
	uint64_t nx = (uint64_t)x->tv_sec * 1000000000ULL + x->tv_nsec;
	uint64_t ny = (uint64_t)y->tv_sec * 1000000000ULL + y->tv_nsec;
	if(ny > nx) return 0;
	return nx - ny;
}

static void handle_motion(Mouse* m) {
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

	// speed per second
	float motion_factor = m->speed * (MOTION_DELAY_MS / 1e3f);
	int x_pixels = (int) ( x * motion_factor );
	int y_pixels = (int) ( y * motion_factor );

	libevdev_uinput_write_event(m->uidev, EV_REL, REL_X, x_pixels);
	libevdev_uinput_write_event(m->uidev, EV_REL, REL_Y, y_pixels);
	libevdev_uinput_write_event(m->uidev, EV_SYN, SYN_REPORT, 0);
}

static void handle_scroll(Mouse* m) {
	int x = 0;
	int y = 0;
	if(m->key_states[K_SCROLL_UP]) y++;
	if(m->key_states[K_SCROLL_DOWN]) y--;
	if(m->key_states[K_SCROLL_LEFT]) x--;
	if(m->key_states[K_SCROLL_RIGHT]) x++;
	if(x == 0 && y == 0) return;

	m->scroll_speed = SCROLL_SPEED_NORMAL;
	if(m->key_states[FAST_MOD]) m->scroll_speed = SCROLL_SPEED_FAST;
	if(m->key_states[SLOW_MOD]) m->scroll_speed = SCROLL_SPEED_SLOW;
	if(m->key_states[SLOWER_MOD]) m->scroll_speed = SCROLL_SPEED_SLOWER;

	// scroll speed per second
	float scroll_factor = m->scroll_speed * (SCROLL_DELAY_MS / 1e3f);

	// store small scroll deltas, scroll if 1 is reached
	m->scroll_fraction_x += x * scroll_factor;
	m->scroll_fraction_y += y * scroll_factor;
	int x_scroll = (int) m->scroll_fraction_x;
	int y_scroll = (int) m->scroll_fraction_y;
	m->scroll_fraction_x -= x_scroll;
	m->scroll_fraction_y -= y_scroll;

	if(x_scroll != 0) {
		libevdev_uinput_write_event(m->uidev, EV_REL, REL_HWHEEL, x_scroll);
	}
	if(y_scroll != 0) {
		libevdev_uinput_write_event(m->uidev, EV_REL, REL_WHEEL, y_scroll);
	}
	libevdev_uinput_write_event(m->uidev, EV_SYN, SYN_REPORT, 0);
}

static void update_button(struct libevdev_uinput* uidev, bool is_pressed, bool* pressed_flag, int ui_btn) {
	if(is_pressed != *pressed_flag) {
			*pressed_flag = is_pressed;
			libevdev_uinput_write_event(uidev, EV_KEY, ui_btn, is_pressed);
	}
}
static void handle_click(Mouse* m) {
	update_button(m->uidev, (bool) m->key_states[K_BUTTON_LEFT], &m->button_left_pressed, BTN_LEFT);
	update_button(m->uidev, (bool) m->key_states[K_BUTTON_MIDDLE], &m->button_middle_pressed, BTN_MIDDLE);
	update_button(m->uidev, (bool) m->key_states[K_BUTTON_RIGHT], &m->button_right_pressed, BTN_RIGHT);
	libevdev_uinput_write_event(m->uidev, EV_SYN, SYN_REPORT, 0);
}


static void handle_mouse(Mouse* m) {
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

	if(time_diff_ns(&now, &m->click_tick) > CLICK_DELAY_NS) {
		handle_click(m);
		clock_gettime(CLOCK_MONOTONIC, &m->click_tick);
	}

	if(time_diff_ns(&now, &m->scroll_tick) > SCROLL_DELAY_NS) {
		handle_scroll(m);
		clock_gettime(CLOCK_MONOTONIC, &m->scroll_tick);
	}

	if(time_diff_ns(&now, &m->motion_tick) > MOTION_DELAY_NS) {
		handle_motion(m);
		clock_gettime(CLOCK_MONOTONIC, &m->motion_tick);
	}
}


static struct libevdev_uinput* create_uinput_mouse_dev(int uifd) {
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
		libevdev_free(mouse_dev);
		return NULL;
	}
	libevdev_free(mouse_dev);
	return ui_mouse_dev;
}

static int guess_keyboard_device_path(char* buff, size_t size) {
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
			// device is keyboard
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

static int check_keyboard_device_path(char* buff) {
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

int init_keyboard_fd(char* keyboard_path, size_t size) {
	int err;
	bool path_provided = (KEYBOARD_DEVICE[0] != '\0');

	if(strlen(KEYBOARD_DEVICE) >= size) {
		fprintf(stderr, "keyboard device path provided is too long.\n");
		keyboard_path[0] = '\0';
		return 1;
	}

	if(path_provided) {
		strncpy(keyboard_path, KEYBOARD_DEVICE, size);
		keyboard_path[size - 1] = '\0';
		err = check_keyboard_device_path(keyboard_path);
		if(err != 0){
			fprintf(stderr, "Incorrect path provided in config.h, make sure its a keyboard device path\n");
			return 1;
		}
	}
	else {
		err = guess_keyboard_device_path(keyboard_path, size);
		if(err != 0){
			fprintf(stderr, "Error guessing keyboard device, please provide device path in config.h\n");
			return 1;
		}
	}
	return 0;
}

int init_mouse(Mouse* m, int uifd) {
	m->key_states = calloc(KEY_MAX + 1, sizeof(int));
	if(m->key_states == NULL) {
		perror("error allocating key_states array");
		return 1;
	}

	m->speed = SPEED_NORMAL;
	m->scroll_speed = SCROLL_SPEED_NORMAL;

	m->scroll_fraction_x = 0;
	m->scroll_fraction_y = 0;

	m->click_tick = (struct timespec){0};
	m->scroll_tick = (struct timespec){0};
	m->motion_tick = (struct timespec){0};

	m->button_left_pressed = false;
	m->button_middle_pressed = false;
	m->button_right_pressed = false;

	m->uidev = create_uinput_mouse_dev(uifd);
	if(m->uidev == NULL) {
		perror("Error creating mouse device");
		free(m->key_states);
		return 1;
	}
	return 0;
}

void destroy_mouse(Mouse* m) {
	libevdev_uinput_destroy(m->uidev);
	free(m->key_states);
}

static int grab_keyboard(int fd) {
	int err = ioctl(fd, EVIOCGRAB, 1);
	if(err < 0) {
		perror("Failed to grab input device");
	}
	return err;
}

static int ungrab_keyboard(int fd) {
	int err = ioctl(fd, EVIOCGRAB, 0);
	if(err < 0) {
		perror("Failed to ungrab input device");
	}
	return err;
}


static bool are_keys_released(int fd) {
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


static bool key_combo_pressed(int* key_states, const int* keys, size_t n) {
	if(!key_states[keys[0]]) return false; // first key is required
	// rest are optional, check only if != -1
	for(size_t i = 1; i < n; i++) {
		if(keys[i] != -1 && !key_states[keys[i]]) { // if key is NOT -1 AND the key is not pressed
			return false;
		}
	}
	return true;
}

static void wait_grab_until_release(int keyboard_fd) {
	int err;
	while(1) {
		err = grab_keyboard(keyboard_fd);
		if(err < 0) {
			perror("error grabbing keyboard");
			usleep(1000);
			continue;
		}
		if(are_keys_released(keyboard_fd)) {
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

static void process_event(struct input_event* event, Mouse* m, bool* grabbing, bool* quit, int keyboard_fd) {
	if(event->type != EV_KEY || event->code >= KEY_MAX) {
		return;
	}
	int code = event->code;
	int value = event->value;

	m->key_states[code] = value;

	bool start_combo_triggered = key_combo_pressed(m->key_states, START_COMBO_KEYS, START_COMBO_KEYS_SIZE);

	if(!(*grabbing) && start_combo_triggered) {
		*grabbing = true;
		wait_grab_until_release(keyboard_fd);
		memset(m->key_states, 0, sizeof(int) * KEY_MAX);
	}

	if(*grabbing) {
		if(code == K_EXIT) {
			memset(m->key_states, 0, sizeof(int) * KEY_MAX);
			*grabbing = false;
			ungrab_keyboard(keyboard_fd);
		}
		else if(code == K_END) {
			*quit = true;
			ungrab_keyboard(keyboard_fd);
		}
	}
}


static void run_event_loop(int keyboard_fd, Mouse* mouse) {
	bool quit = false;
	bool grabbing = false;

	struct pollfd fds[1];
	fds[0].events = POLLIN;
	fds[0].fd = keyboard_fd;
	int poll_result;

	struct input_event event;
	ssize_t bytes_read;

	while(!quit) {
		poll_result = poll(fds, 1, POLL_DELAY_MS);
		if(poll_result > 0) {
			bytes_read = read(keyboard_fd, &event, sizeof(event));
			if(bytes_read == (ssize_t)sizeof(event)) {
				process_event(&event, mouse, &grabbing, &quit, keyboard_fd);
			} 
			else if(errno != EAGAIN && errno != EWOULDBLOCK) {
					perror("Error reading event");
					quit = true;
					break;
			}
		}
		else if(poll_result < 0) {
			perror("poll failed");
			quit = true;
			break;
		}
		if(grabbing) {
			handle_mouse(mouse);
		}
	}
}


int main() {
	int keyboard_fd;
	int mouse_uifd;
	char keyboard_path[MAX_DEVICE_PATH_SIZE];
	Mouse mouse;
	
	if(init_keyboard_fd(keyboard_path, sizeof keyboard_path) != 0){
		return 1;
	}

	keyboard_fd = open(keyboard_path, O_RDONLY | O_NONBLOCK);
	if(keyboard_fd < 0) {
		perror("Error opening input device");
		return 1;
	}
	
	mouse_uifd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	if(mouse_uifd < 0) {
		perror("Failed to open /dev/uinput. Is the uinput module loaded?");
		close(keyboard_fd);
		return 1;
	}
	if(init_mouse(&mouse, mouse_uifd) != 0) {
		close(mouse_uifd);
		close(keyboard_fd);
		return 1;
	}

	run_event_loop(keyboard_fd, &mouse);
	
	destroy_mouse(&mouse);
	close(mouse_uifd);
	close(keyboard_fd);
	return 0;
}
