#ifndef SEGMENT_LCD_DRIVER_H
#define SEGMENT_LCD_DRIVER_H

#include <stdint.h>

// Тип индикатора
typedef enum {
    COMMON_CATHODE,
    COMMON_ANODE
} DisplayType;

// Callback-функции для управления аппаратурой
typedef void (*SegmentCallback)(uint8_t segment_mask);
typedef void (*DigitCallback)(uint8_t digit_index, uint8_t state);

// Структура для конфигурации дисплея
typedef struct {
    SegmentCallback set_segments;    // Функция установки сегментов
    DigitCallback set_digit;         // Функция управления разрядом
    uint8_t digits_count;            // Количество разрядов
    DisplayType type;                // Тип индикатора
} DisplayConfig;

// Структура для управления временем
typedef struct
{
    uint32_t (*get_time_ms)(void); // Функция получения времени (опционально)
    uint32_t internal_counter;     // Внутренний счетчик миллисекунд
} DisplayTime;

// Функции для работы со временем
void Display_Tick(DisplayConfig *config); // Вызывать каждую 1мс
void Display_DelayMs(DisplayConfig *config, uint32_t ms);

// Структура бегущей строки
typedef struct
{
    uint8_t *text;             // Текст для отображения (массив кодов символов)
    uint16_t text_length;      // Длина текста
    uint16_t current_position; // Текущая позиция в тексте
    uint16_t display_length;   // Длина дисплея (количество разрядов)
    uint8_t direction;         // Направление: 0 - вправо, 1 - влево
    uint16_t scroll_delay;     // Задержка между сдвигами в мс
    uint32_t last_scroll_time; // Время последнего сдвига
    uint8_t enabled;           // Включена ли бегущая строка
    uint8_t loop;              // Зациклить бесконечно или остановиться
} ScrollingText;

// Функции бегущей строки
void Scroll_Init(ScrollingText *scroll, uint8_t *text, uint16_t length,
                 uint16_t display_len, uint8_t direction, uint16_t delay_ms, uint8_t loop);
void Scroll_Update(ScrollingText *scroll, DisplayConfig *display);
void Scroll_Start(ScrollingText *scroll);
void Scroll_Stop(ScrollingText *scroll);
void Scroll_Reset(ScrollingText *scroll);
uint8_t Scroll_IsFinished(ScrollingText *scroll);

// Вспомогательные функции для работы со строками
uint16_t String_ToSegmentCodes(const char *str, uint8_t *buffer, uint16_t buffer_size);

// API функции
void Display_Init(DisplayConfig* config);
void Display_Update(DisplayConfig* config);
void Display_SetNumber(uint32_t number);
void Display_SetFloat(float number, uint8_t decimal_places);
void Display_SetCharacters(const uint8_t* characters);
void Display_SetCharacter(uint8_t digit, uint8_t character);
void Display_SetDot(uint8_t digit, uint8_t state);
void Display_Clear(DisplayConfig* config);
void Display_SetBrightness(uint8_t brightness); // Для поддержки ШИМ


// Методы для создания конфигураций
DisplayConfig* Display_CreateConfig(uint8_t digits_count, DisplayType type, 
                                    SegmentCallback seg_cb, DigitCallback dig_cb);
                                    
void Display_DestroyConfig(DisplayConfig* config);


// Константы для символов
#define CHAR_EMPTY     0xFF

#define CHAR_A 10
#define CHAR_B 11
#define CHAR_C 12
#define CHAR_D 13
#define CHAR_E 14
#define CHAR_F 15
#define CHAR_G 16
#define CHAR_H 17
#define CHAR_I 18
#define CHAR_J 19
#define CHAR_L 20 // Было 21
#define CHAR_N 21 // Было 23
#define CHAR_O 22 // Было 24
#define CHAR_P 23 // Было 25
#define CHAR_Q 24 // Было 26
#define CHAR_R 25 // Было 27
#define CHAR_S 26 // Было 28
#define CHAR_T 27 // Было 29
#define CHAR_U 28 // Было 30
#define CHAR_Y 29 // Было 34
#define CHAR_Z 30 // Было 35

#endif