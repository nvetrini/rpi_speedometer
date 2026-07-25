#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/adc_emul.h>
#include <stdlib.h>

/* Use the same paths as rpi_pico.overlay */
static const struct gpio_dt_spec reed   = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), wheel_sensor_gpios);
static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), button1_gpios);
static const struct gpio_dt_spec button2 = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), button2_gpios);
static const struct adc_dt_spec battery_adc = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

static int cmd_reed_toggle(const struct shell *sh, size_t argc, char **argv)
{
    static bool state;
    state = !state;
    gpio_emul_input_set(reed.port, reed.pin, state);
    shell_print(sh, "reed switch -> %d", state);
    return 0;
}

/* simulate N wheel revolutions at a given interval, e.g. `sim wheel 10 500` */
static int cmd_wheel(const struct shell *sh, size_t argc, char **argv)
{
    int revs = (argc > 1) ? atoi(argv[1]) : 10;
    int interval_ms = (argc > 2) ? atoi(argv[2]) : 500;

    for (int i = 0; i < revs; i++) {
        gpio_emul_input_set(reed.port, reed.pin, 1);
        k_msleep(20);
        gpio_emul_input_set(reed.port, reed.pin, 0);
        k_msleep(interval_ms);
    }
    shell_print(sh, "simulated %d wheel revolutions", revs);
    return 0;
}

static int cmd_button1_press(const struct shell *sh, size_t argc, char **argv)
{
    gpio_emul_input_set(button1.port, button1.pin, 1);
    k_msleep(50);
    gpio_emul_input_set(button1.port, button1.pin, 0);
    shell_print(sh, "button 1 pressed");
    return 0;
}

static int cmd_button2_press(const struct shell *sh, size_t argc, char **argv)
{
    gpio_emul_input_set(button2.port, button2.pin, 1);
    k_msleep(50);
    gpio_emul_input_set(button2.port, button2.pin, 0);
    shell_print(sh, "button 2 pressed");
    return 0;
}

static int cmd_battery_set(const struct shell *sh, size_t argc, char **argv)
{
    if (argc < 2) {
        shell_error(sh, "usage: sim battery <millivolts>");
        return -EINVAL;
    }
    /* check adc_emul.h in your Zephyr version — units/signature
       have changed across releases (mV vs raw counts) */
    adc_emul_const_value_set(battery_adc.dev, battery_adc.channel_id, atoi(argv[1]));
    shell_print(sh, "battery set to %s mV", argv[1]);
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sim_cmds,
    SHELL_CMD(reed, NULL, "Toggle reed switch once", cmd_reed_toggle),
    SHELL_CMD(wheel, NULL, "Simulate wheel revs [count] [ms]", cmd_wheel),
    SHELL_CMD(button1, NULL, "Simulate button 1 press", cmd_button1_press),
    SHELL_CMD(button2, NULL, "Simulate button 2 press", cmd_button2_press),
    SHELL_CMD(battery, NULL, "Set battery mV", cmd_battery_set),
    SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(sim, &sim_cmds, "Simulation controls", NULL);
