#include "seg7_display_char.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// ============== Стандартная таблица для общего катода ==============
static const uint8_t DEFAULT_SEGMENT_TABLE[SEG_CHAR_COUNT] = {
    // Цифры 0-9
    0x3F, // 0: ABCDEF
    0x06, // 1: BC
    0x5B, // 2: ABDEG
    0x4F, // 3: ABCDG
    0x66, // 4: BCFG
    0x6D, // 5: ACDFG
    0x7D, // 6: ACDEFG
    0x07, // 7: ABC
    0x7F, // 8: ABCDEFG
    0x6F, // 9: ABCDFG
    
	// Буквы A-Z
	0x77, // A: ABCEFG
	0x7C, // B: CDEFG
	0x39, // C: ADEF
	0x5E, // D: BCDEG
	0x79, // E: ADEFG
	0x71, // F: AEFG
	0x3D, // G: ACDFG
	0x76, // H: BCEFG
	0x30, // I: EF
	0x1E, // J: BCDE
	0x76, // K: BCEFG (такой же как H)
	0x38, // L: DEF
	0x37, // M: ACE (специальный)
	0x54, // N: CEG
	0x3F, // O: ABCDEF (такой же как 0)
	0x73, // P: ABEFG
	0x67, // Q: ABCFG
	0x50, // R: EG
	0x6D, // S: ACDFG (такой же как 5)
	0x78, // T: DEFG
	0x3E, // U: BCDEF
	0x3E, // V: BCDEF (такой же как U)
	0x3E, // W: BCDEF (такой же как U)
	0x76, // X: BCEFG (такой же как H)
	0x6E, // Y: BCDFG
	0x5B, // Z: ABDEG (такой же как 2)
    
	// Символы
	0x00, // Пробел (SEG_CHAR_SPACE) - ВЫКЛ все сегменты
	0x40, // Минус (SEG_CHAR_MINUS)
	0x48, // Равно (SEG_CHAR_EQUAL)
	0x08, // Подчеркивание (SEG_CHAR_UNDERSCORE)
	0x63, // Градус (SEG_CHAR_DEGREE)
	0x49, // Процент (SEG_CHAR_PERCENT)
	0x00, // Двоеточие (SEG_CHAR_COLON) - управляется отдельно
	0x00, // Точка с запятой (SEG_CHAR_SEMICOLON)
	0x22, // Кавычки (SEG_CHAR_QUOTE)
	0x02, // Апостроф (SEG_CHAR_APOSTROPHE)
	0x39, // Квадратная скобка [ (SEG_CHAR_LBRACKET)
	0x0F, // Квадратная скобка ] (SEG_CHAR_RBRACKET)
	0x39, // Круглая скобка ( (SEG_CHAR_LPAREN)
	0x0F, // Круглая скобка ) (SEG_CHAR_RPAREN)
	0x39, // Фигурная скобка { (SEG_CHAR_LBRACE)
	0x0F, // Фигурная скобка } (SEG_CHAR_RBRACE)
	0x00, // Меньше (SEG_CHAR_LT)
	0x00, // Больше (SEG_CHAR_GT)
	0x53, // Вопрос (SEG_CHAR_QUESTION)
	0x82, // Восклицание (SEG_CHAR_EXCLAM)
	0x77, // @ (SEG_CHAR_AT) (как A)
	0x49, // Решетка (SEG_CHAR_HASH)
	0x49, // Доллар (SEG_CHAR_DOLLAR)
	0x7F, // Амперсанд (SEG_CHAR_AMP)
	0x49, // Звезда (SEG_CHAR_STAR)
	0x48, // Плюс (SEG_CHAR_PLUS)
	0x00, // Запятая (SEG_CHAR_COMMA)
	0x00, // Точка (SEG_CHAR_DOT) (без сегментов, только для точки в числе)
	0x01, // Слеш (SEG_CHAR_SLASH)
	0x40, // Обратный слеш (SEG_CHAR_BACKSLASH)
	0x30, // Вертикальная черта (SEG_CHAR_PIPE)
	0x01, // Тильда (SEG_CHAR_TILDE)
	0x00, // Циркумфлекс (SEG_CHAR_CARET)
	0x00, // Гравис (SEG_CHAR_GRAVE)

	// SEG_CHAR_EMPTY  - ПУСТОЙ символ
	0x00, // ВСЕ сегменты ВЫКЛЮЧЕНЫ
};

// ============== Таблица преобразования ASCII -> SegmentChar ==============
// Размер 128 для всех ASCII символов (0-127)
static const uint8_t ASCII_TO_SEGCHAR_TABLE[128] = {
    [0 ... 31] = SEG_CHAR_EMPTY, // Управляющие символы

    // Печатные символы
    [' '] = SEG_CHAR_SPACE,
    ['!'] = SEG_CHAR_EXCLAM,
    ['"'] = SEG_CHAR_QUOTE,
    ['#'] = SEG_CHAR_HASH,
    ['$'] = SEG_CHAR_DOLLAR,
    ['%'] = SEG_CHAR_PERCENT,
    ['&'] = SEG_CHAR_AMP,
    ['\''] = SEG_CHAR_APOSTROPHE,
    ['('] = SEG_CHAR_LPAREN,
    [')'] = SEG_CHAR_RPAREN,
    ['*'] = SEG_CHAR_STAR,
    ['+'] = SEG_CHAR_PLUS,
    [','] = SEG_CHAR_COMMA,
    ['-'] = SEG_CHAR_MINUS,
    ['.'] = SEG_CHAR_DOT,
    ['/'] = SEG_CHAR_SLASH,

    // Цифры 0-9
    ['0'] = SEG_CHAR_0,
    ['1'] = SEG_CHAR_1,
    ['2'] = SEG_CHAR_2,
    ['3'] = SEG_CHAR_3,
    ['4'] = SEG_CHAR_4,
    ['5'] = SEG_CHAR_5,
    ['6'] = SEG_CHAR_6,
    ['7'] = SEG_CHAR_7,
    ['8'] = SEG_CHAR_8,
    ['9'] = SEG_CHAR_9,

    [':'] = SEG_CHAR_COLON,
    [';'] = SEG_CHAR_SEMICOLON,
    ['<'] = SEG_CHAR_LT,
    ['='] = SEG_CHAR_EQUAL,
    ['>'] = SEG_CHAR_GT,
    ['?'] = SEG_CHAR_QUESTION,
    ['@'] = SEG_CHAR_AT,

    // Заглавные буквы A-Z
    ['A'] = SEG_CHAR_A,
    ['B'] = SEG_CHAR_B,
    ['C'] = SEG_CHAR_C,
    ['D'] = SEG_CHAR_D,
    ['E'] = SEG_CHAR_E,
    ['F'] = SEG_CHAR_F,
    ['G'] = SEG_CHAR_G,
    ['H'] = SEG_CHAR_H,
    ['I'] = SEG_CHAR_I,
    ['J'] = SEG_CHAR_J,
    ['K'] = SEG_CHAR_K,
    ['L'] = SEG_CHAR_L,
    ['M'] = SEG_CHAR_M,
    ['N'] = SEG_CHAR_N,
    ['O'] = SEG_CHAR_O,
    ['P'] = SEG_CHAR_P,
    ['Q'] = SEG_CHAR_Q,
    ['R'] = SEG_CHAR_R,
    ['S'] = SEG_CHAR_S,
    ['T'] = SEG_CHAR_T,
    ['U'] = SEG_CHAR_U,
    ['V'] = SEG_CHAR_V,
    ['W'] = SEG_CHAR_W,
    ['X'] = SEG_CHAR_X,
    ['Y'] = SEG_CHAR_Y,
    ['Z'] = SEG_CHAR_Z,

    ['['] = SEG_CHAR_LBRACKET,
    ['\\'] = SEG_CHAR_BACKSLASH,
    [']'] = SEG_CHAR_RBRACKET,
    ['^'] = SEG_CHAR_CARET,
    ['_'] = SEG_CHAR_UNDERSCORE,
    ['`'] = SEG_CHAR_GRAVE,

    // Строчные буквы a-z (маппим на заглавные)
    ['a'] = SEG_CHAR_A,
    ['b'] = SEG_CHAR_B,
    ['c'] = SEG_CHAR_C,
    ['d'] = SEG_CHAR_D,
    ['e'] = SEG_CHAR_E,
    ['f'] = SEG_CHAR_F,
    ['g'] = SEG_CHAR_G,
    ['h'] = SEG_CHAR_H,
    ['i'] = SEG_CHAR_I,
    ['j'] = SEG_CHAR_J,
    ['k'] = SEG_CHAR_K,
    ['l'] = SEG_CHAR_L,
    ['m'] = SEG_CHAR_M,
    ['n'] = SEG_CHAR_N,
    ['o'] = SEG_CHAR_O,
    ['p'] = SEG_CHAR_P,
    ['q'] = SEG_CHAR_Q,
    ['r'] = SEG_CHAR_R,
    ['s'] = SEG_CHAR_S,
    ['t'] = SEG_CHAR_T,
    ['u'] = SEG_CHAR_U,
    ['v'] = SEG_CHAR_V,
    ['w'] = SEG_CHAR_W,
    ['x'] = SEG_CHAR_X,
    ['y'] = SEG_CHAR_Y,
    ['z'] = SEG_CHAR_Z,

    ['{'] = SEG_CHAR_LBRACE,
    ['|'] = SEG_CHAR_PIPE,
    ['}'] = SEG_CHAR_RBRACE,
    ['~'] = SEG_CHAR_TILDE,

    // DEL и выше
    [127] = SEG_CHAR_EMPTY
};

// ============== Структуры таблиц ==============
static const SegmentTable DEFAULT_TABLE_STRUCT = {
    "Default Common Cathode",
    DEFAULT_SEGMENT_TABLE,
    SEG_CHAR_COUNT,
    SEG_CHAR_EMPTY,
	0x80 // Точка - бит 7
};

// ============== Публичные функции ==============

const SegmentTable* segchar_get_default_table(void)
{
    return &DEFAULT_TABLE_STRUCT;
}

uint8_t segchar_get_code(uint8_t character, 
                        const SegmentTable* table, 
                        bool invert_output)
{
    const SegmentTable* used_table = table ? table : &DEFAULT_TABLE_STRUCT;
    
    // Проверка диапазона
    if (character >= used_table->size)
    {
        character = used_table->default_char;
    }
    
    uint8_t code = used_table->codes[character];

    // Если это пустой символ, всегда возвращаем 0x00
    if (character == SEG_CHAR_EMPTY) {
    	return 0x00;
    }

	// Инвертируем если нужно
    if (invert_output)
    {
        code = ~code;
    }
    
    return code;
}

uint8_t segchar_from_ascii(char ascii_char)
{
    // Получаем ASCII код символа
    uint8_t ascii_code = (uint8_t)ascii_char;

    // Проверяем диапазон
    if (ascii_code >= 128) {
        return SEG_CHAR_EMPTY;
    }

    // Прямое преобразование через таблицу
    return ASCII_TO_SEGCHAR_TABLE[ascii_code];
}

bool segchar_convert_string(const char* str, 
                           uint8_t* buffer, 
                           uint16_t buffer_size, 
                           uint16_t* converted_length)
{
    if (!str || !buffer || buffer_size == 0)
    {
        return false;
    }

    uint16_t length = 0;

    while (*str && length < buffer_size)
    {
        buffer[length++] = segchar_from_ascii(*str);
        str++;
    }

    if (converted_length)
    {
        *converted_length = length;
    }

    return true;
}
