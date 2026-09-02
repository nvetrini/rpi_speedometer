/**
 * @file mocks_battery.c
 * @brief Mock implementations for battery testing
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>
#include <app.h>
#include <battery.h>

/* Mock ADC device */
static const struct device *mock_adc_dev;

/* Mock ADC sequence */
static struct adc_sequence mock_sequence;

/* Mock ADC reading buffer */
static int16_t mock_adc_buffer[1];

/* Mock adc_read function */
int mock_adc_read(const struct device *dev, const struct adc_sequence *sequence)
{
    if (!sequence || !sequence->buffer) {
        return -EINVAL;
    }
    
    /* Copy mock value to buffer */
    memcpy(sequence->buffer, mock_adc_buffer, sequence->buffer_size);
    return 0;
}

/* Mock device_is_ready function */
bool mock_device_is_ready(const struct device *dev)
{
    return dev != NULL;
}

/* Function to set mock ADC value */
void mock_set_adc_value(int16_t value)
{
    mock_adc_buffer[0] = value;
}

/* Function to set mock ADC device */
void mock_set_adc_device(const struct device *dev)
{
    mock_adc_dev = dev;
}

/* Function to get mock ADC device */
const struct device *mock_get_adc_device(void)
{
    return mock_adc_dev;
}

/* Mock implementation of battery_adc_to_voltage */
float battery_adc_to_voltage(int16_t adc_value)
{
    /* Simple linear conversion for testing */
    float adc_voltage = (adc_value * 3.3f) / 4095.0f;
    return adc_voltage * VOLTAGE_DIVIDER_RATIO;
}

/* Mock implementation of battery_voltage_to_percentage */
int battery_voltage_to_percentage(float voltage)
{
    /* Linear approximation for testing */
    int percentage = (int)((voltage - BATTERY_MIN_V) /
            (BATTERY_MAX_V - BATTERY_MIN_V) * 100.0f);
    return CLAMP(percentage, 0, 100);
}

/* Function to reset all mocks */
void mock_reset_all(void)
{
    mock_adc_dev = NULL;
    mock_adc_buffer[0] = 0;
    memset(&mock_sequence, 0, sizeof(mock_sequence));
}
