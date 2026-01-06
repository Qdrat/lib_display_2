#include "segment_lcd_char.h"
#include <string.h>
#include <ctype.h>

// ============== Единая таблица для общего катода ==============
static const uint8_t SEGMENT_TABLE[SEG_CHAR_COUNT] = {
    // Цифры 0-9 (сегменты a,b,c,d,e,f,g)
    0x3F, // 0: 0b0111111
    0x06, // 1: 0b0000110
    0x5B, // 2: 0b1011011
    0x4F, // 3: 0b1001111
    0x66, // 4: 0b1100110
    0x6D, // 5: 0b1101101
    0x7D, // 6: 0b1111101
    0x07, // 7: 0b0000111
    0x7F, // 8: 0b1111111
    0x6F, // 9: 0b1101111

    // Буквы A-Z
    0x77, // A: 0b1110111
    0x7C, // B: 0b1111100
    0x39, // C: 0b0111001
    0x5E, // D: 0b1011110
    0x79, // E: 0b1111001
    0x71, // F: 0b1110001
    0x6F, // G: 0b1101111 (как 9)
    0x76, // H: 0b1110110
    0x30, // I: 0b0110000
    0x1E, // J: 0b0011110
    0x76, // K: 0b1110110 (как H)
    0x38, // L: 0b0111000
    0x40, // M: 0b1000000 (середина)
    0x54, // N: 0b1010100
    0x3F, // O: 0b0111111 (как 0)
    0x73, // P: 0b1110011
    0x67, // Q: 0b1100111
    0x50, // R: 0b1010000
    0x6D, // S: 0b1101101 (как 5)
    0x78, // T: 0b1111000
    0x3E, // U: 0b0111110
    0x3E, // V: 0b0111110 (как U)
    0x3E, // W: 0b0111110 (как U)
    0x76, // X: 0b1110110 (как H)
    0x6E, // Y: 0b1101110
    0x5B, // Z: 0b1011011 (как 2)

    // Символы
    0x00, // Пробел
    0x40, // Минус (только сегмент g)
    0x48, // Равно (сегменты f,g)
    0x08, // Подчеркивание (только сегмент d)
    0x63, // Градус (сегменты a,b,f,g)
    0x49, // Процент (сегменты c,f,g)
    0x00, // Двоеточие (управляется отдельно)
    0x82, // Восклицание (сегменты b,c)
    0x53, // Вопрос (сегменты a,c,d,f,g)
    0x22, // Кавычки (сегменты b,f)
    0x02, // Апостроф (только сегмент f)
    0x39, // Левая круглая скобка (как C)
    0x0F, // Правая круглая скобка (сегменты e,f,g)
    0x39, // Левая квадратная скобка (как C)
    0x0F, // Правая квадратная скобка (как правая круглая)
    0x48, // Плюс (сегменты f,g)
    0x49, // Звезда (сегменты c,f,g)
    0x01, // Слеш (только сегмент a)
    0x40, // Обратный слеш (только сегмент g)
    0x30, // Вертикальная черта (сегменты b,c)
    0x01, // Тильда (только сегмент a)
    0x00, // Циркумфлекс
    0xFF, // Пусто (все выключено)

    // Специальные символы (пользователь может переопределить)
    0x00, // Стрелка вверх
    0x00, // Стрелка вниз
    0x00, // Стрелка влево
    0x00, // Стрелка вправо
    0x00, // Сердце
    0x00, // Улыбка
    0x00, // Грусть
    0x00, // Колокольчик
};

// ============== Публичные функции ==============

uint8_t segchar_get_code(SegmentChar character, bool is_common_cathode)
{
    // Проверка диапазона
    if ((uint16_t)character >= SEG_CHAR_COUNT)
    {
        // Возвращаем пустой символ
        return is_common_cathode ? 0x00 : 0xFF;
    }

    uint8_t code = SEGMENT_TABLE[character];

    // Для общего анода инвертируем биты
    if (!is_common_cathode)
    {
        code = ~code;
    }

    return code;
}

SegmentChar segchar_from_ascii(char ascii_char)
{
    // Конвертация ASCII в SegmentChar
    if (ascii_char >= '0' && ascii_char <= '9')
    {
        return (SegmentChar)(ascii_char - '0');
    }
    else if (ascii_char >= 'A' && ascii_char <= 'Z')
    {
        return (SegmentChar)(SEG_CHAR_A + (ascii_char - 'A'));
    }
    else if (ascii_char >= 'a' && ascii_char <= 'z')
    {
        return (SegmentChar)(SEG_CHAR_A + (ascii_char - 'a'));
    }

    // Специальные символы
    switch (ascii_char)
    {
    case ' ':
        return SEG_CHAR_SPACE;
    case '-':
        return SEG_CHAR_MINUS;
    case '=':
        return SEG_CHAR_EQUAL;
    case '_':
        return SEG_CHAR_UNDERSCORE;
    case '°':
        return SEG_CHAR_DEGREE;
    case '%':
        return SEG_CHAR_PERCENT;
    case ':':
        return SEG_CHAR_COLON;
    case '!':
        return SEG_CHAR_EXCLAM;
    case '?':
        return SEG_CHAR_QUESTION;
    case '"':
        return SEG_CHAR_QUOTE;
    case '\'':
        return SEG_CHAR_APOSTROPHE;
    case '(':
        return SEG_CHAR_LPAREN;
    case ')':
        return SEG_CHAR_RPAREN;
    case '[':
        return SEG_CHAR_LBRACKET;
    case ']':
        return SEG_CHAR_RBRACKET;
    case '+':
        return SEG_CHAR_PLUS;
    case '*':
        return SEG_CHAR_STAR;
    case '/':
        return SEG_CHAR_SLASH;
    case '\\':
        return SEG_CHAR_BACKSLASH;
    case '|':
        return SEG_CHAR_PIPE;
    case '~':
        return SEG_CHAR_TILDE;
    case '^':
        return SEG_CHAR_CARET;
    default:
        return SEG_CHAR_EMPTY;
    }
}

bool segchar_convert_string(const char *str,
                            SegmentChar *char_buffer,
                            uint8_t *dot_buffer,
                            uint16_t buffer_size,
                            uint16_t *converted_length)
{
    if (!str || !char_buffer || !dot_buffer || buffer_size == 0)
    {
        return false;
    }

    uint16_t length = 0;

    // Инициализируем буферы
    memset(char_buffer, SEG_CHAR_EMPTY, buffer_size);
    memset(dot_buffer, 0, buffer_size);

    while (*str && length < buffer_size)
    {
        if (*str == '.')
        {
            // Точка для предыдущего символа
            if (length > 0)
            {
                dot_buffer[length - 1] = 1;
            }
            // Не увеличиваем length, так как точка не занимает отдельную позицию
        }
        else
        {
            char_buffer[length] = segchar_from_ascii(*str);
            length++;
        }

        str++;
    }

    if (converted_length)
    {
        *converted_length = length;
    }

    return true;
}