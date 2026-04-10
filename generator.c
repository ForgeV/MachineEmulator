#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_TEMPLATES 100
#define MAX_LINE_LENGTH 512

#include "generator.h"

// Загружаем шаблоны из текстового файла в нашу структуру.
bool load_templates(const char* filepath, TemplateList* list) {
    list->count = 0;
    // Пытаемся открыть файл на чтение.
    FILE* file = fopen(filepath, "r");

    // Если указатель NULL, значит файла нет.
    if (!file) {
        printf("[ERROR] Не удалось открыть файл с шаблонами: %s\n", filepath);
        return false;
    }

    char buffer[MAX_LINE_LENGTH];
    // Построчно читаем файл.
    while (fgets(buffer, sizeof(buffer), file) && list->count < MAX_TEMPLATES) {
        size_t len = strlen(buffer);
        while (len > 0 && (buffer[len-1] == '\n' || buffer[len-1] == '\r')) {
            buffer[len-1] = '\0';
            len--;
        }

        if (len > 0) {
            // _strdup выделяет память динамически и копирует туда строку из буфера.
            // Если просто присвоить адрес, при следующем шаге цикла данные в буфере не сохранятся.
            list->lines[list->count] = _strdup(buffer);
            list->count++;
        }
    }

    fclose(file); // Закрываем файл.
    return list->count > 0;
}

// Возвращаем память системе
void free_templates(TemplateList* list) {
    for (int i = 0; i < list->count; i++) {
        free(list->lines[i]);
    }
    list->count = 0;
}

// Внутренняя функция: превращает теги в реальные данные.
static void process_tag(const char* tag, char* out_val) {
    if (strncmp(tag, "{HEX4}", 6) == 0) {
        snprintf(out_val, 16, "%04x", rand() % 0x10000);
    }
    else if (strncmp(tag, "{HEX5}", 6) == 0) {
        snprintf(out_val, 16, "%05x", (rand() << 4) | (rand() % 0x10));
    }
    else if (strncmp(tag, "{HEX8}", 6) == 0) {
        unsigned int r32 = (rand() << 16) | rand();
        snprintf(out_val, 16, "%08x", r32);
    }
    else if (strncmp(tag, "{INT}", 5) == 0) {
        snprintf(out_val, 16, "%d", rand() % 1000);
    }
    else if (strncmp(tag, "{STATE}", 7) == 0) {
        snprintf(out_val, 16, "%d", rand() % 10);
    }
    else if (strncmp(tag, "{STATE_OLD}", 11) == 0) {
        snprintf(out_val, 16, "%d", (rand() % 5) + 1);
    }
    else if (strncmp(tag, "{STATE_NEW}", 11) == 0) {
        snprintf(out_val, 16, "%d", (rand() % 5) + 6);
    }
    else {
        // Если тег неизвестен, просто оставляем его как есть.
        strcpy(out_val, tag);
    }
}

// Берем шаблон и собираем из него готовую строку лога.
bool generate_event_string(TemplateList* list, char* output_buffer, size_t max_len) {
    if (list->count == 0 || max_len == 0) return false;

    // Выбираем рандомный индекс из списка загруженных шаблонов.
    int template_idx = rand() % list->count;
    const char* template_str = list->lines[template_idx];

    char temp_val[32];
    size_t out_pos = 0; // Наша текущая позиция в итоговой строке.
    size_t temp_len = strlen(template_str);

    // Идем по шаблону посимвольно.
    for (size_t i = 0; i < temp_len && out_pos < max_len - 1; ) {
        if (template_str[i] == '{') {
            const char* end_bracket = strchr(template_str + i, '}');

            if (end_bracket) {
                size_t tag_len = end_bracket - (template_str + i) + 1;
                char tag[32] = {0};

                strncpy(tag, template_str + i, tag_len < 31 ? tag_len : 31);
                process_tag(tag, temp_val);
                size_t val_len = strlen(temp_val);

                // Если результат влезает в наш итоговый буфер, то копируем его туда.
                if (out_pos + val_len < max_len - 1) {
                    strcpy(output_buffer + out_pos, temp_val);
                    out_pos += val_len;
                }

                // Перепрыгиваем через обработанный тег в шаблоне.
                i += tag_len;
                continue;
            }
        }
        // Если это не тег, а просто буква — копируем её как есть.
        output_buffer[out_pos++] = template_str[i++];
    }
    output_buffer[out_pos] = '\0';
    return true;
}