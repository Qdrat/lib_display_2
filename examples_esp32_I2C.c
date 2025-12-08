#include "driver/i2c.h"
#include "segment_lcd_driver.h"

void I2C_SetSegments(uint8_t segment_mask) {
    // Отправляем маску сегментов по I2C
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (0x20 << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, segment_mask, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
}

void I2C_SetDigit(uint8_t digit_index, uint8_t state) {
    // Управляем разрядами через второй I2C расширитель
    uint8_t digit_mask = (1 << digit_index);
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (0x21 << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, state ? digit_mask : 0x00, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
}

DisplayConfig* i2c_display;

void Init_I2C_Display(void) {
    i2c_display = Display_CreateConfig(4, COMMON_CATHODE, I2C_SetSegments, I2C_SetDigit);
    Display_Init(i2c_display);
}