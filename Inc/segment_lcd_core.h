#ifndef SEGMENT_LCD_CORE_H
#define SEGMENT_LCD_CORE_H

#include <stdint.h>
#include <stdbool.h>

// ============== Типы ошибок (ядро) ==============
typedef enum
{
    LCD_CORE_OK = 0,
    LCD_CORE_ERROR_NULL_POINTER,
    LCD_CORE_ERROR_INVALID_DIGIT,
    LCD_CORE_ERROR_MEMORY,
    LCD_CORE_ERROR_NOT_INITIALIZED,
    LCD_CORE_ERROR_INVALID_CONFIG,
    LCD_CORE_ERROR_TIMER_NOT_SET
} LcdCoreError;

// ============== Типы подключения ==============
typedef enum
{
    LCD_TYPE_COMMON_CATHODE = 0,
    LCD_TYPE_COMMON_ANODE = 1
} LcdType;

// ============== Callback-типы ==============
// Функция установки сегментов (маска 8 бит: 7-сегменты + точка)
typedef void (*LcdSegmentCallback)(uint8_t segment_mask);

// Функция управления разрядом (digit_index, состояние)
typedef void (*LcdDigitCallback)(uint8_t digit_index, bool state);

// Функция управления яркостью (0-100%)
typedef void (*LcdBrightnessCallback)(uint8_t brightness);

// Функция получения времени (миллисекунды)
typedef uint32_t (*LcdTimeCallback)(void);

// ============== Структура конфигурации ядра ==============
typedef struct
{
    // Аппаратные callback
    LcdSegmentCallback set_segments;
    LcdDigitCallback set_digit;
    LcdBrightnessCallback set_brightness;
    
    // Конфигурация дисплея
    uint8_t digits_count;          // Количество разрядов
    LcdType type;                  // Тип подключения
    uint16_t refresh_rate_hz;      // Частота обновления (Гц)
    
    // Пользовательская таблица кодов (NULL для стандартной)
    const uint8_t* custom_segment_table;
    
    // Системные callback
    LcdTimeCallback get_time_ms;
    
    // Внутренние поля (инициализируются библиотекой)
    struct
    {
        uint8_t* segment_buffer;    // Буфер масок сегментов
        uint8_t* dot_buffer;        // Буфер состояний точек
        uint16_t digit_time_us;     // Время горения разряда (мкс)
        uint32_t last_update_time;  // Время последнего обновления
        uint8_t current_digit;      // Текущий активный разряд
        uint8_t brightness;         // Текущая яркость (0-100%)
        bool initialized;           // Флаг инициализации
    } _internal;
} LcdCoreConfig;

// ============== API ядра ==============

/**
 * @brief Инициализация ядра дисплея
 * 
 * @param config Конфигурация ядра
 * @return LcdCoreError Код ошибки
 */
LcdCoreError lcd_core_init(LcdCoreConfig* config);

/**
 * @brief Обновление дисплея (должен вызываться периодически)
 * 
 * @param config Конфигурация ядра
 * @return LcdCoreError Код ошибки
 */
LcdCoreError lcd_core_update(LcdCoreConfig* config);

/**
 * @brief Установка маски сегментов для разряда
 * 
 * @param config Конфигурация ядра
 * @param digit Индекс разряда
 * @param segment_mask Маска сегментов (биты 0-6: сегменты, бит 7: точка)
 * @return LcdCoreError Код ошибки
 */
LcdCoreError lcd_core_set_segment_mask(LcdCoreConfig* config, 
                                      uint8_t digit, 
                                      uint8_t segment_mask);

/**
 * @brief Установка состояния точки для разряда
 * 
 * @param config Конфигурация ядра
 * @param digit Индекс разряда
 * @param dot_on Состояние точки (true - включена)
 * @return LcdCoreError Код ошибки
 */
LcdCoreError lcd_core_set_dot(LcdCoreConfig* config, 
                             uint8_t digit, 
                             bool dot_on);

/**
 * @brief Очистка дисплея
 * 
 * @param config Конфигурация ядра
 * @return LcdCoreError Код ошибки
 */
LcdCoreError lcd_core_clear(LcdCoreConfig* config);

/**
 * @brief Установка яркости
 * 
 * @param config Конфигурация ядра
 * @param brightness Яркость (0-100%)
 * @return LcdCoreError Код ошибки
 */
LcdCoreError lcd_core_set_brightness(LcdCoreConfig* config, uint8_t brightness);

/**
 * @brief Получение времени работы ядра (миллисекунды)
 * 
 * @param config Конфигурация ядра
 * @return uint32_t Время в мс
 */
uint32_t lcd_core_get_time(const LcdCoreConfig* config);

#endif // SEGMENT_LCD_CORE_H