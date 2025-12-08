#include "main.h"
#include "segment_lcd_driver.h"

SPI_HandleTypeDef hspi1;

void SPI_SetSegments(uint8_t segment_mask) {
    // Отправляем маску сегментов через SPI
    HAL_SPI_Transmit(&hspi1, &segment_mask, 1, HAL_MAX_DELAY);
}

void SPI_SetDigit(uint8_t digit_index, uint8_t state) {
    // Управляем разрядами через сдвиговый регистр
    uint8_t digit_mask = (1 << digit_index);
    HAL_GPIO_WritePin(LATCH_GPIO_Port, LATCH_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, &digit_mask, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(LATCH_GPIO_Port, LATCH_Pin, GPIO_PIN_SET);
}

DisplayConfig* spi_display;

void Init_SPI_Display(void) {
    spi_display = Display_CreateConfig(8, COMMON_ANODE, SPI_SetSegments, SPI_SetDigit);
    Display_Init(spi_display);
}