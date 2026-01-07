#include "segment_lcd_driver.h"
#include <cstddef>
#include <stdlib.h>
#include <cstring>
#include <cmath>

// Таблицы кодов
const uint8_t SEGMENT_CODES_CC[] = {
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x6F, // 9
    0x77, // A
    0x7C, // b
    0x39, // C
    0x5E, // d
    0x79, // E
    0x71, // F
    0x3D, // G
    0x76, // H
    0x06, // I
    0x1E, // J
    0x38, // L
    0x54, // n
    0x3F, // O
    0x73, // P
    0x67, // q
    0x50, // r
    0x6D, // S
    0x78, // t
    0x3E, // U
    0x6E, // Y
    0x5B  // Z
};
const uint8_t SEGMENT_CODES_CA[] = {
    0xC0, // 0
    0xF9, // 1
    0xA4, // 2
    0xB0, // 3
    0x99, // 4
    0x92, // 5
    0x82, // 6
    0xF8, // 7
    0x80, // 8
    0x90, // 9
    0x88, // A
    0x83, // b
    0xC6, // C
    0xA1, // d
    0x86, // E
    0x8E, // F
    0xC2, // G
    0x89, // H
    0xF9, // I
    0xF1, // J
    0xC7, // L
    0xAB, // n
    0xC0, // O
    0x8C, // P
    0x98, // q
    0xAF, // r
    0x92, // S
    0x87, // t
    0xE3, // U
    0x91, // Y
    0xA4  // Z
};

// Специальные символы
const uint8_t SPECIAL_SYMBOLS_CC[] = {
    0x00, // CHAR_EMPTY
};

const uint8_t SPECIAL_SYMBOLS_CA[] = {
    0xFF, // CHAR_EMPTY
};

// Глобальные переменные
static volatile uint8_t current_digit = 0;
static volatile uint8_t *display_buffer = NULL;
static volatile uint8_t *dot_flags = NULL;
static DisplayConfig *current_config = NULL;
static uint8_t buffer_initialized = 0;

static DisplayTime display_time = {0};

// Внутренние функции
static uint8_t GetSegmentCode(uint8_t character, DisplayType type)
{
    if (character == CHAR_EMPTY)
        return (type == COMMON_CATHODE) ? 0x00 : 0xFF;
    if (character < 36)
    { // Цифры и буквы
        return (type == COMMON_CATHODE) ? SEGMENT_CODES_CC[character] : SEGMENT_CODES_CA[character];
    }

    return (type == COMMON_CATHODE) ? 0x00 : 0xFF;
}

static void InitializeBuffers(DisplayConfig *config)
{
    if (buffer_initialized)
    {
        free((void *)display_buffer);
        free((void *)dot_flags);
    }

    display_buffer = (uint8_t *)malloc(config->digits_count * sizeof(uint8_t));
    dot_flags = (uint8_t *)malloc(config->digits_count * sizeof(uint8_t));

    if (display_buffer && dot_flags)
    {
        memset((void *)display_buffer, 0, config->digits_count);
        memset((void *)dot_flags, 0, config->digits_count);
        buffer_initialized = 1;
    }
}

// API функции
void Display_Init(DisplayConfig *config)
{
    if (!config || !config->set_segments || !config->set_digit)
        return;

    current_config = config;
    InitializeBuffers(config);
    Display_Clear(config);
}

void Display_Update(DisplayConfig *config)
{
    if (!config || !buffer_initialized)
        return;

    // Выключаем все разряды
    if (config->type == COMMON_CATHODE)
    {
        config->set_digit(current_digit, 0);
    }
    else
    {
        config->set_digit(current_digit, 1);
    }

    // Устанавливаем сегменты для текущего разряда
    uint8_t segments = GetSegmentCode(display_buffer[current_digit], config->type);

    // Добавляем точку если нужно
    if (dot_flags[current_digit])
    {
        if (config->type == COMMON_CATHODE)
        {
            segments |= 0x80; // Включаем точку
        }
        else
        {
            segments &= ~0x80; // Включаем точку (инвертированно)
        }
    }
    else
    {
        if (config->type == COMMON_CATHODE)
        {
            segments &= ~0x80; // Выключаем точку
        }
        else
        {
            segments |= 0x80; // Выключаем точку
        }
    }

    // Включаем текущий разряд
    if (config->type == COMMON_CATHODE)
    {
        config->set_digit(current_digit, 1);
    }
    else
    {
        config->set_digit(current_digit, 0);
    }

    // Переходим к следующему разряду
    current_digit = (current_digit + 1) % config->digits_count;
}

void Display_SetNumber(uint32_t number)
{
    if (!buffer_initialized || !current_config)
        return;

    uint32_t temp = number;
    uint8_t digits = current_config->digits_count;

    // Заполняем справа налево
    for (int8_t i = digits - 1; i >= 0; i--)
    {
        if (temp > 0 || i == digits - 1)
        {
            display_buffer[i] = temp % 10;
            temp /= 10;
        }
        else
        {
            display_buffer[i] = CHAR_EMPTY; // Пусто вместо нулей
        }
    }
}

void Display_SetFloat(float number, uint8_t decimal_places)
{
    if (!buffer_initialized || !current_config)
        return;

    uint8_t digits = current_config->digits_count;

    // Ограничиваем количество знаков после запятой
    if (decimal_places >= digits)
        decimal_places = digits - 1;

    // Проверяем диапазон
    float max_value = pow(10, digits - decimal_places) - 1;
    float min_value = -max_value;

    if (number > max_value)
        number = max_value;
    if (number < min_value)
        number = min_value;

    // Обрабатываем отрицательные числа
    uint8_t is_negative = (number < 0);
    if (is_negative)
    {
        number = -number;
    }

    // Масштабируем число
    float scaled_float = number * pow(10, decimal_places);

    // Ограничиваем максимальное значение
    int32_t max_scaled = pow(10, digits) - 1;
    if (scaled_float > max_scaled)
    {
        scaled_float = max_scaled;
    }

    int32_t scaled = (int32_t)(scaled_float + 0.5); // Округление

    // Устанавливаем цифры
    uint8_t start_digit = is_negative ? 1 : 0;

    for (int8_t i = digits - 1; i >= 0; i--)
    {
        if (is_negative && i == 0)
        {
            // Первый символ - минус
            display_buffer[i] = 31; // Индекс для '-'
            continue;
        }

        if (i == (digits - 1 - decimal_places))
        {
            // Устанавливаем точку для этого разряда
            Display_SetDot(i, 1);
        }

        if (scaled > 0 || i >= (digits - 1 - decimal_places))
        {
            display_buffer[i] = scaled % 10;
            scaled /= 10;
        }
        else
        {
            display_buffer[i] = CHAR_EMPTY;
        }
    }
}

void Display_SetCharacters(const uint8_t *characters)
{
    if (!buffer_initialized || !current_config || !characters)
        return;

    uint8_t digits = current_config->digits_count;
    for (uint8_t i = 0; i < digits; i++)
    {
        display_buffer[i] = characters[i];
    }
}

void Display_SetCharacter(uint8_t digit, uint8_t character)
{
    if (!buffer_initialized || !current_config || digit >= current_config->digits_count)
        return;

    display_buffer[digit] = character;
}

void Display_SetDot(uint8_t digit, uint8_t state)
{
    if (!buffer_initialized || !current_config || digit >= current_config->digits_count)
        return;

    dot_flags[digit] = state;
}

void Display_Clear(DisplayConfig *config)
{
    if (!buffer_initialized || !config)
        return;

    uint8_t digits = config->digits_count;
    for (uint8_t i = 0; i < digits; i++)
    {
        display_buffer[i] = CHAR_EMPTY;
        dot_flags[i] = 0;
    }

    // Выключаем все сегменты и разряды
    if (config->type == COMMON_CATHODE)
    {
        config->set_segments(0x00);
        for (uint8_t i = 0; i < digits; i++)
        {
            config->set_digit(i, 0);
        }
    }
    else
    {
        config->set_segments(0xFF);
        for (uint8_t i = 0; i < digits; i++)
        {
            config->set_digit(i, 1);
        }
    }
}

// Создание базовой конфигурации
DisplayConfig *Display_CreateConfig(uint8_t digits_count, DisplayType type,
                                    SegmentCallback seg_cb, DigitCallback dig_cb)
{
    DisplayConfig *config = (DisplayConfig *)malloc(sizeof(DisplayConfig));
    if (!config)
        return NULL;

    config->digits_count = digits_count;
    config->type = type;
    config->set_segments = seg_cb;
    config->set_digit = dig_cb;

    return config;
}

// Очистка ресурсов
void Display_DestroyConfig(DisplayConfig *config)
{
    if (!config)
        return;

    // Освобождаем буферы только если это тот же конфиг
    if (buffer_initialized && current_config == config)
    {
        free((void *)display_buffer);
        free((void *)dot_flags);
        display_buffer = NULL;
        dot_flags = NULL;
        current_config = NULL;
        buffer_initialized = 0;
    }

    free(config);
}

// Функция для обновления времени (вызывать каждую 1мс в прерывании таймера)
void Display_Tick(DisplayConfig *config)
{
    (void)config; // Не используем, но оставляем для совместимости
    display_time.internal_counter++;
}

// Функция получения текущего времени
static uint32_t GetCurrentTime(void)
{
    // Если пользователь предоставил свою функцию времени - используем ее
    if (display_time.get_time_ms)
    {
        return display_time.get_time_ms();
    }
    // Иначе используем внутренний счетчик
    return display_time.internal_counter;
}

// Функция задержки (блокирующая)
void Display_DelayMs(DisplayConfig *config, uint32_t ms)
{
    uint32_t start_time = GetCurrentTime();
    while (GetCurrentTime() - start_time < ms)
    {
        // Можно добавить вызов Display_Update для поддержания индикации
        if (config)
        {
            Display_Update(config);
        }
    }
}

// Установка пользовательской функции времени
void Display_SetTimeCallback(uint32_t (*time_callback)(void))
{
    display_time.get_time_ms = time_callback;
}

// Получение внутреннего счетчика времени
uint32_t Display_GetInternalTime(void)
{
    return display_time.internal_counter;
}

// Сброс внутреннего счетчика времени
void Display_ResetInternalTime(void)
{
    display_time.internal_counter = 0;
}

// Функция преобразования строки в коды сегментов
uint16_t String_ToSegmentCodes(const char *str, uint8_t *buffer, uint16_t buffer_size)
{
    uint16_t length = 0;

    while (*str && length < buffer_size)
    {
        char c = *str;

        switch (c)
        {
        case '0' ... '9':
            buffer[length++] = c - '0';
            break;
        case 'A':
        case 'a':
            buffer[length++] = CHAR_A;
            break;
        case 'B':
        case 'b':
            buffer[length++] = CHAR_B;
            break;
        case 'C':
        case 'c':
            buffer[length++] = CHAR_C;
            break;
        case 'D':
        case 'd':
            buffer[length++] = CHAR_D;
            break;
        case 'E':
        case 'e':
            buffer[length++] = CHAR_E;
            break;
        case 'F':
        case 'f':
            buffer[length++] = CHAR_F;
            break;
        case 'G':
        case 'g':
            buffer[length++] = CHAR_G;
            break;
        case 'H':
        case 'h':
            buffer[length++] = CHAR_H;
            break;
        case 'I':
        case 'i':
            buffer[length++] = CHAR_I;
            break;
        case 'J':
        case 'j':
            buffer[length++] = CHAR_J;
            break;
        case 'L':
        case 'l':
            buffer[length++] = CHAR_L;
            break;
        case 'N':
        case 'n':
            buffer[length++] = CHAR_N;
            break;
        case 'O':
        case 'o':
            buffer[length++] = CHAR_O;
            break;
        case 'P':
        case 'p':
            buffer[length++] = CHAR_P;
            break;
        case 'Q':
        case 'q':
            buffer[length++] = CHAR_Q;
            break;
        case 'R':
        case 'r':
            buffer[length++] = CHAR_R;
            break;
        case 'S':
        case 's':
            buffer[length++] = CHAR_S;
            break;
        case 'T':
        case 't':
            buffer[length++] = CHAR_T;
            break;
        case 'U':
        case 'u':
            buffer[length++] = CHAR_U;
            break;
        case 'Y':
        case 'y':
            buffer[length++] = CHAR_Y;
            break;
        case 'Z':
        case 'z':
            buffer[length++] = CHAR_Z;
            break;
        // case '-':
        //     buffer[length++] = CHAR_DASH;
        //     break;
        // case '_':
        //     buffer[length++] = CHAR_UNDERSCORE;
        //     break;
        case ' ':
            buffer[length++] = CHAR_EMPTY;
            break;
        case '.':
            // Устанавливаем точку для предыдущего символа
            if (length > 0)
            {
                Display_SetDot(length - 1, 1);
            }
            // Не увеличиваем length, так как точка не добавляет новый символ
            break;
        default:
            buffer[length++] = CHAR_EMPTY;
            break;
        }

        str++;
    }

    return length;
}

// Инициализация бегущей строки
void Scroll_Init(ScrollingText *scroll, uint8_t *text, uint16_t length,
                 uint16_t display_len, uint8_t direction, uint16_t delay_ms, uint8_t loop)
{
    scroll->text = text;
    scroll->text_length = length;
    scroll->display_length = display_len;
    scroll->direction = direction;
    scroll->scroll_delay = delay_ms;
    scroll->loop = loop;
    scroll->enabled = 0; // По умолчанию выключена
    scroll->current_position = 0;
    scroll->last_scroll_time = 0;
}

// Запуск бегущей строки
void Scroll_Start(ScrollingText *scroll)
{
    scroll->enabled = 1;
    scroll->current_position = (scroll->direction == 0) ? 0 : scroll->text_length - 1;
}

// Остановка бегущей строки
void Scroll_Stop(ScrollingText *scroll)
{
    scroll->enabled = 0;
}

// Сброс бегущей строки в начальное положение
void Scroll_Reset(ScrollingText *scroll)
{
    scroll->current_position = (scroll->direction == 0) ? 0 : scroll->text_length - 1;
}

// Проверка завершения бегущей строки (только если не зациклена)
uint8_t Scroll_IsFinished(ScrollingText *scroll)
{
    if (scroll->loop)
        return 0;

    if (scroll->direction == 0)
    {
        // Движение вправо - закончили, когда весь текст прошел
        return (scroll->current_position >= scroll->text_length + scroll->display_length);
    }
    else
    {
        // Движение влево - закончили, когда позиция стала отрицательной
        return (scroll->current_position < 0);
    }
}

// Функции бегущей строки (используется GetCurrentTime)
void Scroll_Update(ScrollingText *scroll, DisplayConfig *display)
{
    if (!scroll || !scroll->enabled || !display)
        return;

    uint32_t current_time = GetCurrentTime();

    // Проверяем, не пришло ли время для сдвига
    if (current_time - scroll->last_scroll_time < scroll->scroll_delay)
    {
        return;
    }

    scroll->last_scroll_time = current_time;

    // Проверяем завершение (если не зациклено)
    if (!scroll->loop && Scroll_IsFinished(scroll))
    {
        scroll->enabled = 0;
        return;
    }

    // Создаем буфер для отображаемой части текста
    uint8_t display_buffer[scroll->display_length];
    memset(display_buffer, CHAR_EMPTY, scroll->display_length);

    // Заполняем буфер в зависимости от направления
    if (scroll->direction == 0)
    {
        // Движение вправо
        for (uint8_t i = 0; i < scroll->display_length; i++)
        {
            int16_t text_index = scroll->current_position + i - scroll->display_length;

            if (text_index >= 0 && text_index < scroll->text_length)
            {
                display_buffer[i] = scroll->text[text_index];
            }
        }

        // Увеличиваем позицию для следующего шага
        scroll->current_position++;

        // Проверяем зацикливание
        if (scroll->loop && scroll->current_position >= scroll->text_length + scroll->display_length)
        {
            scroll->current_position = 0;
        }
    }
    else
    {
        // Движение влево
        for (uint8_t i = 0; i < scroll->display_length; i++)
        {
            int16_t text_index = scroll->current_position + i;

            if (text_index >= 0 && text_index < scroll->text_length)
            {
                display_buffer[i] = scroll->text[text_index];
            }
        }

        // Уменьшаем позицию для следующего шага
        scroll->current_position--;

        // Проверяем зацикливание
        if (scroll->loop && scroll->current_position < -(int16_t)scroll->display_length)
        {
            scroll->current_position = scroll->text_length - 1;
        }
    }

    // Устанавливаем символы на дисплей
    Display_SetCharacters(display_buffer);
}