#include "segment_lcd_driver.h"

void Arduino_SetSegments(uint8_t segment_mask) {
    // Устанавливаем сегменты через порт
    PORTD = segment_mask; // Пины 0-7
}

void Arduino_SetDigit(uint8_t digit_index, uint8_t state) {
    // Управляем разрядами через мультиплексор
    digitalWrite(10, (digit_index & 0x01) ? HIGH : LOW);
    digitalWrite(11, (digit_index & 0x02) ? HIGH : LOW);
    digitalWrite(12, (digit_index & 0x04) ? HIGH : LOW);
    digitalWrite(13, state ? HIGH : LOW); // Enable pin
}

DisplayConfig* arduino_display;

void setup() {
    pinMode(10, OUTPUT);
    pinMode(11, OUTPUT);
    pinMode(12, OUTPUT);
    pinMode(13, OUTPUT);
    DDRD = 0xFF; // Пины 0-7 как выходы
    
    arduino_display = Display_CreateConfig(8, COMMON_CATHODE, Arduino_SetSegments, Arduino_SetDigit);
    Display_Init(arduino_display);
}