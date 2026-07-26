/*
 * Simple cycling computer - wheel sensor input with wheel diameter setting
 *
 * Reads a reed switch / Hall-effect sensor via interrupt-driven GPIO.
 * Each time the wheel magnet passes the sensor, an interrupt fires,
 * a revolution counter is incremented (with debouncing), and the
 * main loop periodically derives speed/distance from the count.
 *
 * Button 1 (+1cm) and Button 2 (-1cm) allow setting wheel diameter.
 * Two consecutive presses of button 1 within MODE_SWITCH_DELAY_MSms enter/exit settings mode.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/atomic.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/sys/util.h>
#include <zephyr/sd/mmc.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/fs/fs_interface.h>
#include <errno.h>

#if IS_ENABLED(CONFIG_DISPLAY) && IS_ENABLED(CONFIG_CHARACTER_FRAMEBUFFER) && \
	DT_HAS_CHOSEN(zephyr_display)
#include <zephyr/display/cfb.h>
#define APP_HAS_DISPLAY 1
#else
#define APP_HAS_DISPLAY 0
#endif

/* Constants */
#define MIN_WHEEL_DIAMETER_CM 10
#define MAX_WHEEL_DIAMETER_CM 100
#define DEBOUNCE_MS 50
#define REPORT_INTERVAL_MS 1000
#define MODE_SWITCH_DELAY_MS 500

/* Battery monitoring constants */
#define BATTERY_MIN_V 2.0f    /* Voltage at 0% (2x AA depleted) */
#define BATTERY_MAX_V 3.0f    /* Voltage at 100% (2x AA fresh) */
#define VOLTAGE_DIVIDER_RATIO (3.0f / 2.0f)  /* V_battery = V_adc * (100k+200k)/200k */
#define BATTERY_SAMPLE_INTERVAL_S 60  /* Sample battery every 60 seconds */

/* Wheel configuration */
#define DEFAULT_WHEEL_DIAMETER_CM 660

/*
 * Application state structures
 */

/* Wheel sensor state - shared between ISR and main thread */
struct wheel_sensor_state {
	atomic_t revolution_count;
	volatile int64_t last_trigger_ms;
};

/* Button state - shared between ISR and main thread */
struct button_state {
	volatile int64_t last_press_ms[2];
	volatile bool in_settings_mode;
	volatile bool save_wheel_diameter_pending;
	int64_t previous_press_time[2];
};

/* Wheel configuration */
struct wheel_config {
	int diameter_cm;
};

/* Runtime tracking state (main thread only) */
struct runtime_state {
	uint32_t last_count;
	uint32_t total_distance_m;
	float last_speed_kmh;
	int battery_percentage;
	float battery_voltage;
	bool battery_valid;
};

/* Pulls the gpio spec (port/pin/flags) straight from the devicetree
 * overlay's "zephyr,user" node - no custom binding needed. */
static const struct gpio_dt_spec wheel_sensor =
	GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), wheel_sensor_gpios);
static const struct gpio_dt_spec button1 =
	GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), button1_gpios);
static const struct gpio_dt_spec button2 =
	GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), button2_gpios);

/* Battery voltage ADC channel from devicetree */
static const struct adc_dt_spec battery_adc =
	ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

/* SD card device reference */
static const struct device *sd_card_dev;

/* Filesystem mount point */
static const char *mount_point = "/SD:";
static struct fs_mount_t sd_fs_mount = {
	.mnt_point = "/SD:",
};

static void app_state_work_handler(struct k_work *work);

/* Logging to SD card */
static struct fs_file_t log_file;
static char log_file_path[32];
#define LOG_BUFFER_SIZE 256

static struct gpio_callback wheel_sensor_cb_data;
static struct gpio_callback button1_cb_data;
static struct gpio_callback button2_cb_data;

/* Global state instances */
static struct wheel_sensor_state wheel_sensor_state = {
	.revolution_count = ATOMIC_INIT(0),
	.last_trigger_ms = 0,
};

static struct button_state button_state = {
	.last_press_ms = {0, 0},
	.in_settings_mode = false,
	.previous_press_time = {0, 0},
};

static struct wheel_config wheel_config = {
	.diameter_cm = DEFAULT_WHEEL_DIAMETER_CM,
};

static struct runtime_state runtime_state = {
	.last_count = 0U,
	.total_distance_m = 0U,
	.last_speed_kmh = 0.0f,
	.battery_percentage = -1,
	.battery_voltage = 0.0f,
	.battery_valid = false,
};

K_WORK_DEFINE(app_state_work, app_state_work_handler);

/* Forward declarations */
static void save_wheel_diameter_setting(void);
static int initialize_sd_card(void);
static int mount_sd_filesystem(void);
static int open_log_file(void);
static void log_to_sd(const char *msg);
static void queue_app_state_update(void);
static void render_display_status(void);

static void wheel_sensor_triggered(const struct device *dev,
			    struct gpio_callback *cb,
			    uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	int64_t now = k_uptime_get();

	if ((now - wheel_sensor_state.last_trigger_ms) < DEBOUNCE_MS) {
		return;
	}
	wheel_sensor_state.last_trigger_ms = now;
	atomic_inc(&wheel_sensor_state.revolution_count);
}

/* Read battery voltage and calculate remaining percentage */
static void read_and_report_battery(void)
{
	if (!device_is_ready(battery_adc.dev)) {
		return;
	}

	int16_t adc_value;
	struct adc_sequence sequence = {
		.channels = BIT(battery_adc.channel_id),
		.buffer = &adc_value,
		.buffer_size = sizeof(adc_value),
		.resolution = 12,
	};

	int ret = adc_read(battery_adc.dev, &sequence);
	if (ret < 0) {
		printk("Battery ADC read error: %d\n", ret);
		return;
	}

	/* Convert ADC value to voltage (12-bit ADC, 3.3V reference) */
	float adc_voltage = (adc_value * 3.3f) / 4095.0f;

	/* Apply voltage divider ratio: V_battery = V_adc * (R1+R2)/R2 = V_adc * 3/2 */
	float battery_voltage = adc_voltage * VOLTAGE_DIVIDER_RATIO;

	/* Calculate battery percentage (linear approximation for alkaline 2xAA) */
	int percentage = (int)((battery_voltage - BATTERY_MIN_V) /
			(BATTERY_MAX_V - BATTERY_MIN_V) * 100.0f);
	percentage = CLAMP(percentage, 0, 100);

	runtime_state.battery_percentage = percentage;
	runtime_state.battery_voltage = battery_voltage;
	runtime_state.battery_valid = true;

	printk("Battery: %d%% (%.2fV)\n", percentage, (double)battery_voltage);
	queue_app_state_update();
}


static void button_pressed(const struct device *dev,
		   struct gpio_callback *cb,
		   uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);

	int64_t now = k_uptime_get();

	int button_idx = -1;
	if (pins & BIT(button1.pin)) {
		button_idx = 0;
	} else if (pins & BIT(button2.pin)) {
		button_idx = 1;
	}
	if (button_idx < 0) {
		return;
	}

	// Debounce
	if ((now - button_state.last_press_ms[button_idx]) < DEBOUNCE_MS) {
		return;
	}
	button_state.last_press_ms[button_idx] = now;

	if (button_idx == 0) {
		/* Button 1: double press toggles settings mode; single press increments diameter. */
		if ((now - button_state.previous_press_time[button_idx]) < MODE_SWITCH_DELAY_MS) {
			button_state.in_settings_mode = !button_state.in_settings_mode;
			if (button_state.in_settings_mode) {
				printk("Entering settings mode. Current diameter: %d cm\n",
				       wheel_config.diameter_cm);
			} else {
				printk("Exiting settings mode. Wheel diameter set to: %d cm\n",
				       wheel_config.diameter_cm);
				button_state.save_wheel_diameter_pending = true;
			}
			queue_app_state_update();
		} else {
			if (button_state.in_settings_mode) {
				wheel_config.diameter_cm =
					MIN(wheel_config.diameter_cm + 1, MAX_WHEEL_DIAMETER_CM);
				printk("Wheel diameter: %d cm\n", wheel_config.diameter_cm);
				queue_app_state_update();
			}
		}
		button_state.previous_press_time[button_idx] = now;
	} else {
		/* Button 2: single press decrements diameter in settings mode. */
		if (button_state.in_settings_mode) {
			wheel_config.diameter_cm =
				MAX(wheel_config.diameter_cm - 1, MIN_WHEEL_DIAMETER_CM);
			printk("Wheel diameter: %d cm\n", wheel_config.diameter_cm);
			queue_app_state_update();
		}
	}
}

/* Settings handler for wheel diameter */
static int settings_wheel_diameter_handler(const char *key, size_t len,
				    settings_read_cb read_cb, void *cb_arg)
{
	int rc;
	int val;

	if (strcmp(key, "wheel_diameter") == 0) {
		if (len != sizeof(int)) {
			return -EINVAL;
		}

		rc = read_cb(cb_arg, &val, sizeof(int));
		if (rc < 0) {
			return rc;
		}

		if (val >= MIN_WHEEL_DIAMETER_CM && val <= MAX_WHEEL_DIAMETER_CM) {
			wheel_config.diameter_cm = val;
			printk("Loaded wheel diameter: %d cm\n", wheel_config.diameter_cm);
		} else {
			printk("Invalid wheel diameter value: %d cm\n", val);
		}

		return 0;
	}

	return -ENOENT;
}

static struct settings_handler wheel_diameter_handler = {
	.name = "wheel_diameter",
	.h_set = settings_wheel_diameter_handler,
};

/* Save wheel diameter setting */
static void save_wheel_diameter_setting(void)
{
	int rc = settings_save_one("wheel_diameter/wheel_diameter",
					   &wheel_config.diameter_cm, sizeof(int));
	if (rc < 0) {
		printk("Failed to save wheel diameter setting: %d\n", rc);
	}
}

/* Load wheel diameter setting */
static void load_wheel_diameter_setting(void)
{
	int rc = settings_load_subtree("wheel_diameter");
	if (rc < 0 && rc != -ENOENT) {
		printk("Failed to load wheel diameter setting: %d\n", rc);
	}
}

#if APP_HAS_DISPLAY
static const struct device *display_dev;
static uint8_t display_font_height;

static int display_initialize(void)
{
	int ret;
	uint8_t font_width;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		printk("Warning: display device not ready\n");
		display_dev = NULL;
		return -ENODEV;
	}

	ret = display_set_pixel_format(display_dev, PIXEL_FORMAT_MONO10);
	if (ret < 0) {
		ret = display_set_pixel_format(display_dev, PIXEL_FORMAT_MONO01);
	}
	if (ret < 0) {
		printk("Warning: display does not support mono pixel formats: %d\n", ret);
		display_dev = NULL;
		return ret;
	}

	ret = cfb_framebuffer_init(display_dev);
	if (ret < 0) {
		printk("Warning: failed to initialize display framebuffer: %d\n", ret);
		display_dev = NULL;
		return ret;
	}

	ret = cfb_framebuffer_set_font(display_dev, 0);
	if (ret < 0) {
		printk("Warning: failed to select default display font: %d\n", ret);
		display_dev = NULL;
		return ret;
	}

	ret = cfb_get_font_size(display_dev, 0, &font_width, &display_font_height);
	if (ret < 0 || display_font_height == 0U) {
		display_font_height = 8U;
	}

	ret = display_blanking_off(display_dev);
	if (ret < 0 && ret != -ENOSYS) {
		printk("Warning: failed to unblank display: %d\n", ret);
	}

	return 0;
}

static void display_print_lines(const char *line1, const char *line2,
				const char *line3, const char *line4)
{
	int y = 0;

	if (display_dev == NULL) {
		return;
	}

	cfb_framebuffer_clear(display_dev, false);

	if (line1 != NULL) {
		cfb_print(display_dev, line1, 0, y);
	}
	y += display_font_height;

	if (line2 != NULL) {
		cfb_print(display_dev, line2, 0, y);
	}
	y += display_font_height;

	if (line3 != NULL) {
		cfb_print(display_dev, line3, 0, y);
	}
	y += display_font_height;

	if (line4 != NULL) {
		cfb_print(display_dev, line4, 0, y);
	}

	cfb_framebuffer_finalize(display_dev);
}

static void render_display_status(void)
{
	char line1[32];
	char line2[32];
	char line3[32];
	char line4[32];
	int speed_whole;
	unsigned int speed_frac;

	if (display_dev == NULL) {
		return;
	}

	if (button_state.in_settings_mode) {
		snprintk(line1, sizeof(line1), "Settings mode");
		snprintk(line2, sizeof(line2), "Wheel: %d cm", wheel_config.diameter_cm);
		snprintk(line3, sizeof(line3), "Btn1 +1  Btn2 -1");
		snprintk(line4, sizeof(line4), "Double press B1 to exit");
	} else {
		speed_whole = (int)runtime_state.last_speed_kmh;
		speed_frac = (unsigned int)((runtime_state.last_speed_kmh - speed_whole) * 10.0f);
		if (speed_frac > 9U) {
			speed_frac = 9U;
		}

		snprintk(line1, sizeof(line1), "Speed: %d.%01u km/h", speed_whole, speed_frac);
		snprintk(line2, sizeof(line2), "Distance: %u m", runtime_state.total_distance_m);
		snprintk(line3, sizeof(line3), "Wheel: %d cm", wheel_config.diameter_cm);
		if (runtime_state.battery_valid) {
			snprintk(line4, sizeof(line4), "Battery: %d%% %.2fV",
				 runtime_state.battery_percentage,
				 (double)runtime_state.battery_voltage);
		} else {
			snprintk(line4, sizeof(line4), "Battery: n/a");
		}
	}

	display_print_lines(line1, line2, line3, line4);
}
#else
static int display_initialize(void)
{
	return 0;
}

static void render_display_status(void)
{
}
#endif

static void queue_app_state_update(void)
{
	k_work_submit(&app_state_work);
}

static void app_state_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (button_state.save_wheel_diameter_pending) {
		button_state.save_wheel_diameter_pending = false;
		save_wheel_diameter_setting();
	}

	render_display_status();
}

/* Initialize SD card over SPI */
static int initialize_sd_card(void)
{
	/* Get the SDHC SPI device from devicetree */
	sd_card_dev = DEVICE_DT_GET(DT_NODELABEL(sdhc0));
	if (!device_is_ready(sd_card_dev)) {
		printk("Error: SD card device not ready\n");
		return -ENODEV;
	}

	printk("SD card device found and ready\n");
	return 0;
}

/* Mount SD card filesystem */
static int mount_sd_filesystem(void)
{
	int ret;

	/* Initialize the filesystem mount structure */
	sd_fs_mount.fs_data = NULL;

	/* Mount the filesystem */
	ret = fs_mount(&sd_fs_mount);
	if (ret < 0) {
		printk("Error: Failed to mount SD card filesystem: %d\n", ret);
		return ret;
	}

	printk("SD card filesystem mounted at %s\n", mount_point);
	return 0;
}

/* Open log file on SD card */
static int open_log_file(void)
{
	int ret;
	char logs_dir_path[20];

	/* Initialize file object */
	fs_file_t_init(&log_file);

	/* Construct paths */
	snprintk(logs_dir_path, sizeof(logs_dir_path), "%slogs", mount_point);
	snprintk(log_file_path, sizeof(log_file_path), "%slogs/app.log", mount_point);

	/* Create logs directory if it doesn't exist */
	struct fs_dirent dir_entry;
	ret = fs_stat(logs_dir_path, &dir_entry);
	if (ret < 0 && ret != -ENOENT) {
		printk("Error: Failed to check logs directory: %d\n", ret);
		return ret;
	}

	if (ret == -ENOENT) {
		/* Directory doesn't exist, create it */
		ret = fs_mkdir(logs_dir_path);
		if (ret < 0) {
			printk("Error: Failed to create logs directory: %d\n", ret);
			return ret;
		}
		printk("Created logs directory\n");
	}

	/* Open log file for appending */
	ret = fs_open(&log_file, log_file_path, FS_O_WRITE | FS_O_APPEND | FS_O_CREATE);
	if (ret < 0) {
		printk("Error: Failed to open log file: %d\n", ret);
		log_file.mp = NULL;  /* Mark as not open */
		return ret;
	}

	printk("Opened log file at %s\n", log_file_path);
	return 0;
}

/* Write log message to SD card */
static void log_to_sd(const char *msg)
{
	if (log_file.mp != NULL) {
		int ret;
		size_t len = strlen(msg);

		/* Write the message to the log file */
		ret = fs_write(&log_file, msg, len);
		if (ret < 0) {
			printk("Error: Failed to write to log file: %d\n", ret);
		} else {
			/* Ensure data is written to disk */
			fs_sync(&log_file);
		}
	}
}

int main(void)
{
	int ret;

	// Initialize settings subsystem
	settings_subsys_init();
	settings_register(&wheel_diameter_handler);
	settings_load();

	// Load wheel diameter setting
	load_wheel_diameter_setting();

	printk("Wheel diameter: %d cm\n", wheel_config.diameter_cm);

	ret = display_initialize();
	if (ret < 0) {
		printk("Warning: display initialization failed, continuing without display\n");
	}
	queue_app_state_update();

	// Initialize SD card
	ret = initialize_sd_card();
	if (ret < 0) {
		printk("Warning: SD card initialization failed, continuing without SD card\n");
	} else {
		// Mount SD card filesystem
		ret = mount_sd_filesystem();
		if (ret < 0) {
			printk("Warning: SD card filesystem mount failed, continuing without SD card\n");
		} else {
			// Open log file on SD card
			ret = open_log_file();
			if (ret < 0) {
				printk("Warning: Failed to open log file, continuing without SD card logging\n");
			} else {
				printk("SD card logging initialized\n");
			}
		}
	}

	if (!gpio_is_ready_dt(&wheel_sensor)) {
		printk("Error: wheel sensor GPIO device not ready\n");
		return 0;
	}

	/* Configure wheel sensor */
	ret = gpio_pin_configure_dt(&wheel_sensor, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d configuring wheel sensor pin\n", ret);
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&wheel_sensor,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d configuring wheel sensor interrupt\n", ret);
		return 0;
	}

	gpio_init_callback(&wheel_sensor_cb_data, wheel_sensor_triggered,
			   BIT(wheel_sensor.pin));
	gpio_add_callback(wheel_sensor.port, &wheel_sensor_cb_data);

	/* Configure buttons */
	if (!gpio_is_ready_dt(&button1) || !gpio_is_ready_dt(&button2)) {
		printk("Error: button GPIO devices not ready\n");
		// Continue without buttons
	} else {
		ret = gpio_pin_configure_dt(&button1, GPIO_INPUT);
		if (ret != 0) {
			printk("Error %d configuring button1 pin\n", ret);
		} else {
			ret = gpio_pin_interrupt_configure_dt(&button1,
						      GPIO_INT_EDGE_TO_ACTIVE);
			if (ret != 0) {
				printk("Error %d configuring button1 interrupt\n", ret);
			} else {
				gpio_init_callback(&button1_cb_data, button_pressed,
						   BIT(button1.pin));
				gpio_add_callback(button1.port, &button1_cb_data);
			}
		}

		ret = gpio_pin_configure_dt(&button2, GPIO_INPUT);
		if (ret != 0) {
			printk("Error %d configuring button2 pin\n", ret);
		} else {
			ret = gpio_pin_interrupt_configure_dt(&button2,
						      GPIO_INT_EDGE_TO_ACTIVE);
			if (ret != 0) {
				printk("Error %d configuring button2 interrupt\n", ret);
			} else {
				gpio_init_callback(&button2_cb_data, button_pressed,
						   BIT(button2.pin));
				gpio_add_callback(button2.port, &button2_cb_data);
			}
		}
	}

	printk("Wheel sensor ready, waiting for revolutions...\n");
	printk("Press button 1 twice quickly to enter settings mode\n");

	/* Initialize runtime state - already done at file scope */
	uint32_t battery_sample_counter = 0;

	while (1) {
		k_msleep(REPORT_INTERVAL_MS);

		// Sample battery voltage every BATTERY_SAMPLE_INTERVAL_S seconds
		battery_sample_counter++;
		if (battery_sample_counter >= BATTERY_SAMPLE_INTERVAL_S) {
			read_and_report_battery();
			battery_sample_counter = 0;
		}

		if (button_state.in_settings_mode) {
			continue;
		}

		/* Snapshot the counter atomically */
		uint32_t current_count = atomic_get(&wheel_sensor_state.revolution_count);
		uint32_t delta = current_count - runtime_state.last_count;
		runtime_state.last_count = current_count;

		/* Circumference = π * diameter (convert cm to m) */
		float circumference_m = 3.1415926535f * (wheel_config.diameter_cm / 100.0f);
		float distance_this_period_m = delta * circumference_m;
		runtime_state.total_distance_m = (uint32_t)(current_count * circumference_m);
		runtime_state.last_speed_kmh = (distance_this_period_m /
						(REPORT_INTERVAL_MS / 1000.0f)) * 3.6f;

		/* speed (km/h) = distance (m) / time (s) * 3.6 */
		float speed_kmh = runtime_state.last_speed_kmh;
		unsigned int speed_frac = (unsigned int)((speed_kmh - (int)speed_kmh) * 10.0f);
		if (speed_frac > 9U) {
			speed_frac = 9U;
		}

		printk("revs=%u  speed=%d.%01u km/h  distance=%u m  diameter=%d cm\n",
		       current_count,
		       (int)speed_kmh,
		       speed_frac,
		       runtime_state.total_distance_m,
		       wheel_config.diameter_cm);

		/* Log to SD card if logging is initialized */
		if (log_file.mp != NULL) {
			char log_msg[LOG_BUFFER_SIZE];
			snprintk(log_msg, sizeof(log_msg),
				"[%lld] revs=%u, speed=%.1f km/h, distance=%u m, diameter=%d cm\n",
				k_uptime_get(), current_count, (double)speed_kmh,
				runtime_state.total_distance_m, wheel_config.diameter_cm);
			log_to_sd(log_msg);
		}

		queue_app_state_update();
	}

	return 0;
}
