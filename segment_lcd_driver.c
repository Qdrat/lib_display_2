#include "segment_lcd_driver.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

// ============== Конфигурация для встраиваемых систем ==============
// Раскомментировать для систем без стандартной библиотеки
// #define DISPLAY_NO_STDLIB
// #define DISPLAY_STATIC_BUFFERS

// Размер статических буферов (если используется)
#ifdef DISPLAY_STATIC_BUFFERS
#define MAX_DISPLAY_DIGITS 8
#endif

// ============== Таблицы кодов (оптимизированные) ==============
static const uint8_t SEGMENT_CODES_CC[] = {
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
    0x38, // I (исправлено)
    0x1E, // J
    0x38, // K (такой же как I)
    0x38, // L (исправлено на правильный)
    0x40, // M (приблизительно)
    0x54, // n
    0x3F, // O
    0x73, // P
    0x67, // q
    0x50, // r
    0x6D, // S
    0x78, // t
    0x3E, // U
    0x3E, // V (такой же как U)
    0x3E, // W (такой же как U)
    0x76, // X (такой же как H)
    0x6E, // Y
    0x5B, // Z
    0x40, // '-' (сегмент g)
    0x08, // '_' (сегмент dp)
    0x63, // '°'
};

static const uint8_t SEGMENT_CODES_CA[] = {
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
    0xC7, // I 
    0xF1, // J
    0xC7, // K
    0xC7, // L
    0xBF, // M
    0xAB, // n
    0xC0, // O
    0x8C, // P
    0x98, // q
    0xAF, // r
    0x92, // S
    0x87, // t
    0xE3, // U
    0xE3, // V
    0xE3, // W
    0x89, // X
    0x91, // Y
    0xA4, // Z
    0xBF, // '-'
    0xF7, // '_'
    0x9C, // '°'
};

// ============== Глобальные переменные ==============
static DisplayConfig *current_config = NULL;
static uint8_t current_digit = 0;
static bool display_initialized = false;
static DisplayTime display_time = {0};

#ifdef DISPLAY_STATIC_BUFFERS
static uint8_t display_buffer_static[MAX_DISPLAY_DIGITS];
static uint8_t dot_flags_static[MAX_DISPLAY_DIGITS];
static uint8_t *display_buffer = display_buffer_static;
static uint8_t *dot_flags = dot_flags_static;
#else
static uint8_t *display_buffer = NULL;
static uint8_t *dot_flags = NULL;
#endif

// ============== Внутренние функции ==============
static DisplayError InitializeBuffers(uint8_t digits_count)
{
#ifdef DISPLAY_STATIC_BUFFERS
    if (digits_count > MAX_DISPLAY_DIGITS)
    {
        return DISPLAY_ERROR_BUFFER_OVERFLOW;
    }
#else
    // Освобождаем старые буферы, если есть
    if (display_buffer)
    {
        free(display_buffer);
        display_buffer = NULL;
    }
    if (dot_flags)
    {
        free(dot_flags);
        dot_flags = NULL;
    }

    // Выделяем новую память
    display_buffer = (uint8_t *)malloc(digits_count * sizeof(uint8_t));
    dot_flags = (uint8_t *)malloc(digits_count * sizeof(uint8_t));

    if (!display_buffer || !dot_flags)
    {
        if (display_buffer)
            free(display_buffer);
        if (dot_flags)
            free(dot_flags);
        display_buffer = NULL;
        dot_flags = NULL;
        return DISPLAY_ERROR_MEMORY;
    }
#endif

    // Инициализируем буферы
    memset(display_buffer, (uint8_t)SEG_CHAR_EMPTY, digits_count);
    memset(dot_flags, 0, digits_count);

    return DISPLAY_OK;
}

static uint8_t GetSegmentCode(SegmentChar character, DisplayType type)
{
    if (character == SEG_CHAR_EMPTY)
    {
        return (type == DISPLAY_TYPE_COMMON_CATHODE) ? 0x00 : 0xFF;
    }

    if ((uint8_t)character < sizeof(SEGMENT_CODES_CC))
    {
        return (type == DISPLAY_TYPE_COMMON_CATHODE) ? SEGMENT_CODES_CC[(uint8_t)character] : SEGMENT_CODES_CA[(uint8_t)character];
    }

    return (type == DISPLAY_TYPE_COMMON_CATHODE) ? 0x00 : 0xFF;
}

// ============== API функции ==============
DisplayError Display_Init(DisplayConfig *config)
{
    DISPLAY_CHECK_PTR(config);

    if (!config->set_segments || !config->set_digit)
    {
        return DISPLAY_ERROR_INVALID_CONFIG;
    }

    current_config = config;

    DisplayError err = InitializeBuffers(config->digits_count);
    if (err != DISPLAY_OK)
    {
        return err;
    }

    // Устанавливаем яркость по умолчанию
    if (config->set_brightness)
    {
        config->set_brightness(config->brightness);
    }

    // Очищаем дисплей
    err = Display_Clear();
    if (err != DISPLAY_OK)
    {
        return err;
    }

    display_initialized = true;
    return DISPLAY_OK;
}

DisplayError Display_Update(void)
{
    DISPLAY_CHECK_INIT();
    DISPLAY_CHECK_PTR(current_config);

    // Выключаем текущий разряд
    bool digit_off_state = (current_config->type == DISPLAY_TYPE_COMMON_CATHODE);
    current_config->set_digit(current_digit, digit_off_state);

    // Получаем код сегментов для текущей цифры
    uint8_t segments = GetSegmentCode(
        (SegmentChar)display_buffer[current_digit],
        current_config->type);

    // Обрабатываем точку
    if (dot_flags[current_digit])
    {
        if (current_config->type == DISPLAY_TYPE_COMMON_CATHODE)
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
        if (current_config->type == DISPLAY_TYPE_COMMON_CATHODE)
        {
            segments &= ~0x80; // Выключаем точку
        }
        else
        {
            segments |= 0x80; // Выключаем точку
        }
    }

    // Устанавливаем сегменты
    current_config->set_segments(segments);

    // Включаем текущий разряд
    bool digit_on_state = !digit_off_state;
    current_config->set_digit(current_digit, digit_on_state);

    // Переходим к следующему разряду
    current_digit = (current_digit + 1) % current_config->digits_count;

    return DISPLAY_OK;
}

DisplayError Display_SetNumber(int32_t number)
{
    DISPLAY_CHECK_INIT();
    DISPLAY_CHECK_PTR(current_config);

    uint32_t temp = (number < 0) ? (uint32_t)(-number) : (uint32_t)number;
    uint8_t digits = current_config->digits_count;

    // Заполняем справа налево
    for (int8_t i = digits - 1; i >= 0; i--)
    {
        if (temp > 0 || i == digits - 1)
        {
            display_buffer[i] = (uint8_t)(temp % 10);
            temp /= 10;
        }
        else
        {
            display_buffer[i] = (uint8_t)SEG_CHAR_EMPTY;
        }
    }

    // Добавляем знак минус для отрицательных чисел
    if (number < 0)
    {
        // Ищем первую непустую позицию слева
        for (uint8_t i = 0; i < digits; i++)
        {
            if (display_buffer[i] != (uint8_t)SEG_CHAR_EMPTY)
            {
                if (i > 0)
                {
                    display_buffer[i - 1] = (uint8_t)SEG_CHAR_MINUS;
                }
                break;
            }
        }
    }

    return DISPLAY_OK;
}

DisplayError Display_SetFloat(float number, uint8_t decimal_places)
{
    DISPLAY_CHECK_INIT();
    DISPLAY_CHECK_PTR(current_config);

    uint8_t digits = current_config->digits_count;

    // Проверяем валидность параметров
    if (decimal_places >= digits)
    {
        decimal_places = digits - 1;
    }

    // Обрабатываем отрицательные числа
    bool is_negative = (number < 0);
    if (is_negative)
    {
        number = -number;
        digits--; // Один разряд на минус
    }

    // Проверяем, помещается ли число
    float max_value = 1.0f;
    for (uint8_t i = 0; i < (digits - decimal_places); i++)
    {
        max_value *= 10.0f;
    }
    max_value -= 1.0f / powf(10.0f, (float)decimal_places);

    if (number > max_value)
    {
        number = max_value;
    }

    // Масштабируем и округляем
    int32_t scaled = (int32_t)(number * powf(10.0f, (float)decimal_places) + 0.5f);

    // Устанавливаем цифры
    for (int8_t i = current_config->digits_count - 1; i >= 0; i--)
    {
        // Устанавливаем точку
        if (i == (int8_t)(current_config->digits_count - 1 - decimal_places))
        {
            Display_SetDot((uint8_t)i, DOT_ON);
        }
        else
        {
            Display_SetDot((uint8_t)i, DOT_OFF);
        }

        // Устанавливаем цифру
        if (scaled > 0 || i >= (int8_t)(current_config->digits_count - 1 - decimal_places))
        {
            display_buffer[i] = (uint8_t)(scaled % 10);
            scaled /= 10;
        }
        else
        {
            display_buffer[i] = (uint8_t)SEG_CHAR_EMPTY;
        }
    }

    // Устанавливаем минус для отрицательных чисел
    if (is_negative)
    {
        // Ищем первую непустую позицию слева
        for (uint8_t i = 0; i < current_config->digits_count; i++)
        {
            if (display_buffer[i] != (uint8_t)SEG_CHAR_EMPTY)
            {
                if (i > 0)
                {
                    display_buffer[i - 1] = (uint8_t)SEG_CHAR_MINUS;
                }
                break;
            }
        }
    }

    return DISPLAY_OK;
}

DisplayError Display_SetCharacters(const SegmentChar *characters)
{
    DISPLAY_CHECK_INIT();
    DISPLAY_CHECK_PTR(current_config);
    DISPLAY_CHECK_PTR(characters);

    uint8_t digits = current_config->digits_count;
    for (uint8_t i = 0; i < digits; i++)
    {
        if (characters[i] > SEG_CHAR_DEGREE && characters[i] != SEG_CHAR_EMPTY)
        {
            return DISPLAY_ERROR_INVALID_CHAR;
        }
        display_buffer[i] = (uint8_t)characters[i];
    }

    return DISPLAY_OK;
}

DisplayError Display_SetCharacter(uint8_t digit, SegmentChar character)
{
    DISPLAY_CHECK_INIT();
    DISPLAY_CHECK_PTR(current_config);
    DISPLAY_CHECK_DIGIT(digit);

    if (character > SEG_CHAR_DEGREE && character != SEG_CHAR_EMPTY)
    {
        return DISPLAY_ERROR_INVALID_CHAR;
    }

    display_buffer[digit] = (uint8_t)character;
    return DISPLAY_OK;
}

DisplayError Display_SetDot(uint8_t digit, DotState state)
{
    DISPLAY_CHECK_INIT();
    DISPLAY_CHECK_PTR(current_config);
    DISPLAY_CHECK_DIGIT(digit);

    dot_flags[digit] = (uint8_t)state;
    return DISPLAY_OK;
}

DisplayError Display_Clear(void)
{
    DISPLAY_CHECK_INIT();
    DISPLAY_CHECK_PTR(current_config);

    uint8_t digits = current_config->digits_count;

    // Очищаем буферы
    memset(display_buffer, (uint8_t)SEG_CHAR_EMPTY, digits);
    memset(dot_flags, 0, digits);

    // Выключаем все сегменты
    if (current_config->type == DISPLAY_TYPE_COMMON_CATHODE)
    {
        current_config->set_segments(0x00);
    }
    else
    {
        current_config->set_segments(0xFF);
    }

    // Выключаем все разряды
    bool digit_off_state = (current_config->type == DISPLAY_TYPE_COMMON_CATHODE);
    for (uint8_t i = 0; i < digits; i++)
    {
        current_config->set_digit(i, digit_off_state);
    }

    return DISPLAY_OK;
}

DisplayError Display_SetBrightness(BrightnessLevel brightness)
{
    DISPLAY_CHECK_INIT();
    DISPLAY_CHECK_PTR(current_config);

    // Проверяем диапазон
    if (brightness > 100)
    {
        brightness = 100;
    }

    // Сохраняем значение
    current_config->brightness = brightness;

    // Если есть callback для яркости, вызываем его
    if (current_config->set_brightness)
    {
        current_config->set_brightness(brightness);
    }

    return DISPLAY_OK;
}

// ============== Функции конфигурации ==============
DisplayError Display_CreateConfig(DisplayConfig *config,
                                  uint8_t digits_count,
                                  DisplayType type,
                                  SegmentCallback seg_cb,
                                  DigitCallback dig_cb,
                                  BrightnessCallback bright_cb)
{
    DISPLAY_CHECK_PTR(config);

    if (!seg_cb || !dig_cb || digits_count == 0)
    {
        return DISPLAY_ERROR_INVALID_CONFIG;
    }

    config->set_segments = seg_cb;
    config->set_digit = dig_cb;
    config->set_brightness = bright_cb;
    config->digits_count = digits_count;
    config->type = type;
    config->brightness = 100; // Яркость по умолчанию

    return DISPLAY_OK;
}

// ============== Функции времени ==============
void Display_Tick(void)
{
    display_time.internal_counter++;
}

DisplayError Display_DelayMs(uint32_t ms)
{
    DISPLAY_CHECK_INIT();

    uint32_t start_time = Display_GetInternalTime();
    while (Display_GetInternalTime() - start_time < ms)
    {
        // Обновляем дисплей во время задержки
        DisplayError err = Display_Update();
        if (err != DISPLAY_OK)
        {
            return err;
        }
    }

    return DISPLAY_OK;
}

void Display_SetTimeCallback(uint32_t (*time_callback)(void))
{
    display_time.get_time_ms = time_callback;
}

uint32_t Display_GetInternalTime(void)
{
    if (display_time.get_time_ms)
    {
        return display_time.get_time_ms();
    }
    return display_time.internal_counter;
}

void Display_ResetInternalTime(void)
{
    display_time.internal_counter = 0;
}

// ============== Функции бегущей строки ==============
DisplayError Scroll_Init(ScrollingText *scroll,
                         const SegmentChar *text,
                         uint16_t length,
                         uint8_t display_len,
                         ScrollDirection direction,
                         uint16_t delay_ms,
                         bool loop)
{
    DISPLAY_CHECK_PTR(scroll);
    DISPLAY_CHECK_PTR(text);

    if (length == 0 || display_len == 0)
    {
        return DISPLAY_ERROR_INVALID_CONFIG;
    }

    scroll->text = text;
    scroll->text_length = length;
    scroll->display_length = display_len;
    scroll->direction = direction;
    scroll->scroll_delay = delay_ms;
    scroll->loop = loop;
    scroll->enabled = false;
    scroll->current_position = (direction == SCROLL_RIGHT) ? 0 : (int16_t)(length - 1);
    scroll->last_scroll_time = 0;

    return DISPLAY_OK;
}

DisplayError Scroll_Update(ScrollingText *scroll)
{
    DISPLAY_CHECK_PTR(scroll);
    DISPLAY_CHECK_INIT();

    if (!scroll->enabled)
    {
        return DISPLAY_OK;
    }

    uint32_t current_time = Display_GetInternalTime();

    // Проверяем время для сдвига
    if (current_time - scroll->last_scroll_time < scroll->scroll_delay)
    {
        return DISPLAY_OK;
    }

    scroll->last_scroll_time = current_time;

    // Проверяем завершение
    if (!scroll->loop && Scroll_IsFinished(scroll))
    {
        scroll->enabled = false;
        return DISPLAY_OK;
    }

    // Создаем буфер для отображения
    SegmentChar display_buf[16]; // Максимум 16 разрядов
    if (scroll->display_length > 16)
    {
        return DISPLAY_ERROR_BUFFER_OVERFLOW;
    }

    memset(display_buf, SEG_CHAR_EMPTY, sizeof(SegmentChar) * scroll->display_length);

    // Заполняем буфер
    if (scroll->direction == SCROLL_RIGHT)
    {
        for (uint8_t i = 0; i < scroll->display_length; i++)
        {
            int16_t idx = scroll->current_position + i - scroll->display_length;
            if (idx >= 0 && idx < (int16_t)scroll->text_length)
            {
                display_buf[i] = scroll->text[idx];
            }
        }
        scroll->current_position++;

        if (scroll->loop && scroll->current_position >= (int16_t)(scroll->text_length + scroll->display_length))
        {
            scroll->current_position = 0;
        }
    }
    else
    {
        for (uint8_t i = 0; i < scroll->display_length; i++)
        {
            int16_t idx = scroll->current_position + i;
            if (idx >= 0 && idx < (int16_t)scroll->text_length)
            {
                display_buf[i] = scroll->text[idx];
            }
        }
        scroll->current_position--;

        if (scroll->loop && scroll->current_position < -(int16_t)scroll->display_length)
        {
            scroll->current_position = (int16_t)(scroll->text_length - 1);
        }
    }

    // Отображаем
    return Display_SetCharacters(display_buf);
}

DisplayError Scroll_Start(ScrollingText *scroll)
{
    DISPLAY_CHECK_PTR(scroll);

    scroll->enabled = true;
    return DISPLAY_OK;
}

DisplayError Scroll_Stop(ScrollingText *scroll)
{
    DISPLAY_CHECK_PTR(scroll);

    scroll->enabled = false;
    return DISPLAY_OK;
}

DisplayError Scroll_Reset(ScrollingText *scroll)
{
    DISPLAY_CHECK_PTR(scroll);

    scroll->current_position = (scroll->direction == SCROLL_RIGHT) ? 0 : (int16_t)(scroll->text_length - 1);
    scroll->last_scroll_time = 0;

    return DISPLAY_OK;
}

bool Scroll_IsFinished(const ScrollingText *scroll)
{
    if (!scroll || scroll->loop)
    {
        return false;
    }

    if (scroll->direction == SCROLL_RIGHT)
    {
        return (scroll->current_position >= (int16_t)(scroll->text_length + scroll->display_length));
    }
    else
    {
        return (scroll->current_position < -(int16_t)scroll->display_length);
    }
}

// ============== Вспомогательные функции ==============
DisplayError String_ToSegmentCodes(const char *str,
                                   SegmentChar *buffer,
                                   uint16_t buffer_size,
                                   uint16_t *converted_length)
{
    DISPLAY_CHECK_PTR(str);
    DISPLAY_CHECK_PTR(buffer);

    uint16_t length = 0;
    bool prev_dot = false;

    while (*str && length < buffer_size)
    {
        char c = *str;

        switch (c)
        {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            buffer[length++] = (SegmentChar)(c - '0');
            prev_dot = false;
            break;

        case 'A':
        case 'a':
            buffer[length++] = SEG_CHAR_A;
            break;
        case 'B':
        case 'b':
            buffer[length++] = SEG_CHAR_B;
            break;
        case 'C':
        case 'c':
            buffer[length++] = SEG_CHAR_C;
            break;
        case 'D':
        case 'd':
            buffer[length++] = SEG_CHAR_D;
            break;
        case 'E':
        case 'e':
            buffer[length++] = SEG_CHAR_E;
            break;
        case 'F':
        case 'f':
            buffer[length++] = SEG_CHAR_F;
            break;
        case 'G':
        case 'g':
            buffer[length++] = SEG_CHAR_G;
            break;
        case 'H':
        case 'h':
            buffer[length++] = SEG_CHAR_H;
            break;
        case 'I':
        case 'i':
            buffer[length++] = SEG_CHAR_I;
            break;
        case 'J':
        case 'j':
            buffer[length++] = SEG_CHAR_J;
            break;
        case 'K':
        case 'k':
            buffer[length++] = SEG_CHAR_K;
            break;
        case 'L':
        case 'l':
            buffer[length++] = SEG_CHAR_L;
            break;
        case 'M':
        case 'm':
            buffer[length++] = SEG_CHAR_M;
            break;
        case 'N':
        case 'n':
            buffer[length++] = SEG_CHAR_N;
            break;
        case 'O':
        case 'o':
            buffer[length++] = SEG_CHAR_O;
            break;
        case 'P':
        case 'p':
            buffer[length++] = SEG_CHAR_P;
            break;
        case 'Q':
        case 'q':
            buffer[length++] = SEG_CHAR_Q;
            break;
        case 'R':
        case 'r':
            buffer[length++] = SEG_CHAR_R;
            break;
        case 'S':
        case 's':
            buffer[length++] = SEG_CHAR_S;
            break;
        case 'T':
        case 't':
            buffer[length++] = SEG_CHAR_T;
            break;
        case 'U':
        case 'u':
            buffer[length++] = SEG_CHAR_U;
            break;
        case 'V':
        case 'v':
            buffer[length++] = SEG_CHAR_V;
            break;
        case 'W':
        case 'w':
            buffer[length++] = SEG_CHAR_W;
            break;
        case 'X':
        case 'x':
            buffer[length++] = SEG_CHAR_X;
            break;
        case 'Y':
        case 'y':
            buffer[length++] = SEG_CHAR_Y;
            break;
        case 'Z':
        case 'z':
            buffer[length++] = SEG_CHAR_Z;
            break;

        case '-':
            buffer[length++] = SEG_CHAR_MINUS;
            prev_dot = false;
            break;

        case '_':
            buffer[length++] = SEG_CHAR_UNDERSCORE;
            prev_dot = false;
            break;

        case '.':
            if (length > 0 && !prev_dot)
            {
                // Устанавливаем точку для предыдущего символа
                Display_SetDot((uint8_t)(length - 1), DOT_ON);
            }
            prev_dot = true;
            break;

        case ' ':
            buffer[length++] = SEG_CHAR_EMPTY;
            prev_dot = false;
            break;

        default:
            buffer[length++] = SEG_CHAR_EMPTY;
            prev_dot = false;
            break;
        }

        str++;
    }

    if (converted_length)
    {
        *converted_length = length;
    }

    return DISPLAY_OK;
}