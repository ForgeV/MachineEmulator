#ifndef GENERATOR_H
#define GENERATOR_H

#include <stdbool.h>

#define MAX_TEMPLATES 100

typedef struct {
    char* lines[MAX_TEMPLATES];
    int count;
} TemplateList;

bool load_templates(const char* filepath, TemplateList* list);
void free_templates(TemplateList* list);
bool generate_event_string(TemplateList* list, char* output_buffer, size_t max_len);

#endif