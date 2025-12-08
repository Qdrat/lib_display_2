#include "main.h"
#include "segment_lcd_driver.h"

// Callback-функции для STM32
void STM32_SetSegments(uint8_t segment_mask) {
    // segment_mask: битовая маска сегментов (a=bit0, b=bit1, ..., dp=bit7)
    HAL_GPIO_WritePin(SEG_A_GPIO_Port, SEG_A_Pin, (segment_mask & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SEG_B_GPIO_Port, SEG_B_Pin, (segment_mask & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SEG_C_GPIO_Port, SEG_C_Pin, (segment_mask & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SEG_D_GPIO_Port, SEG_D_Pin, (segment_mask & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SEG_E_GPIO_Port, SEG_E_Pin, (segment_mask & 0x10) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SEG_F_GPIO_Port, SEG_F_Pin, (segment_mask & 0x20) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SEG_G_GPIO_Port, SEG_G_Pin, (segment_mask & 0x40) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SEG_DP_GPIO_Port, SEG_DP_Pin, (segment_mask & 0x80) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void STM32_SetDigit(uint8_t digit_index, uint8_t state) {
    // Включаем/выключаем конкретный разряд
    switch(digit_index) {
        case 0: HAL_GPIO_WritePin(DIG1_GPIO_Port, DIG1_Pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET); break;
        case 1: HAL_GPIO_WritePin(DIG2_GPIO_Port, DIG2_Pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET); break;
        case 2: HAL_GPIO_WritePin(DIG3_GPIO_Port, DIG3_Pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET); break;
        case 3: HAL_GPIO_WritePin(DIG4_GPIO_Port, DIG4_Pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET); break;
    }
}

DisplayConfig* stm32_display;

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM2_Init();
    
    // Создаем дисплей с callback-функциями
    stm32_display = Display_CreateConfig(4, COMMON_CATHODE, STM32_SetSegments, STM32_SetDigit);
    Display_Init(stm32_display);
    
    Display_SetNumber(1234);
    
    HAL_TIM_Base_Start_IT(&htim2);
    
    while(1) {
        // Основная программа
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        Display_Update(stm32_display);
    }
}