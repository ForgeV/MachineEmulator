#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#define MAX_LINE_LENGTH 2048
#define MAX_TEMPLATES 1024
#define MAX_RENDERED 4096

#define ERROR_CHANCE 20
#define LOOP_DELAY_MS 1000

#define MAX_LOG_SIZE 5242880

typedef struct{
    char lines[MAX_TEMPLATES][MAX_LINE_LENGTH];
    int count;
} TemplateStorage;

TemplateStorage errorTemplates;
TemplateStorage operateTemplates;

FILE* errorLog;
FILE* operateLog;

int machineState = 0;

void generateHex(char* out, int length){
    static const char* hex = "0123456789ABCDEF";

    for (int i = 0; i < length; i++){
        out[i] = hex[rand() % 16];
    }

    out[length] = '\0';
}

void trimNewline(char* str){
    size_t len = strlen(str);

    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')){
        str[len - 1] = '\0';
        len--;
    }
}

void loadTemplates(const char* filename, TemplateStorage* storage){
    FILE* file = fopen(filename, "r");

    if (!file){
        printf("Failed to open template file: %s\n", filename);
        exit(1);
    }

    storage->count = 0;

    while (fgets(storage->lines[storage->count], MAX_LINE_LENGTH, file)){
        trimNewline(storage->lines[storage->count]);

        if (strlen(storage->lines[storage->count]) > 0){
            storage->count++;
        }
        if (storage->count >= MAX_TEMPLATES){
            break;
        }
    }

    fclose(file);
}

const char* randomTemplate(TemplateStorage* storage){
    if (storage->count == 0){
        return "";
    }
    int index = rand() % storage->count;

    return storage->lines[index];
}

void appendText(char* dest, int* pos, const char* src){
    while (*src){
        dest[*pos] = *src;
        (*pos)++;
        src++;
    }
    dest[*pos] = '\0';
}

void appendChar(char* dest, int* pos, char c){
    dest[*pos] = c;
    (*pos)++;
    dest[*pos] = '\0';
}

void renderTemplate(const char* input, char* output){
    int inputPos = 0;
    int outputPos = 0;

    int oldState = machineState;
    int newState = rand() % 10;
    machineState = newState;

    while (input[inputPos] != '\0'){
        if (input[inputPos] == '{'){
            char token[128];
            int tokenPos = 0;

            inputPos++;

            while (input[inputPos] != '}' && input[inputPos] != '\0'){
                token[tokenPos++] = input[inputPos++];
            }

            token[tokenPos] = '\0';

            if (input[inputPos] == '}'){
                inputPos++;
            }

            if (strcmp(token, "INT") == 0){
                char temp[64];
                sprintf(temp, "%d", rand() % 10000);
                appendText(output, &outputPos, temp);
            }
            else if (strcmp(token, "STATE_OLD") == 0){
                char temp[64];
                sprintf(temp, "%d", oldState);
                appendText(output, &outputPos, temp);
            }
            else if (strcmp(token, "STATE_NEW") == 0){
                char temp[64];
                sprintf(temp, "%d", newState);
                appendText(output, &outputPos, temp);
            }
            else if (strncmp(token, "HEX", 3) == 0){
                int hex_length = atoi(token + 3);
                char temp[256];
                generateHex(temp, hex_length);
                appendText(output, &outputPos, temp);
            }
            else{
                appendChar(output, &outputPos, '{');
                appendText(output, &outputPos, token);
                appendChar(output, &outputPos, '}');
            }
        }
        else{
            appendChar(output, &outputPos, input[inputPos]);
            inputPos++;
        }
    }
    output[outputPos] = '\0';
}

long getFileSize(FILE* file){
    long current = ftell(file);

    fseek(file, 0, SEEK_END);

    long size = ftell(file);

    fseek(file, current, SEEK_SET);

    return size;
}

void rotateLog(const char* filename){
    char backup[256];

    sprintf(backup, "%s.old", filename);
    remove(backup);
    rename(filename, backup);
}

void checkRotation(FILE** file, const char* filename){
    long size = getFileSize(*file);

    if (size >= MAX_LOG_SIZE){
        fclose(*file);
        rotateLog(filename);
        *file = fopen(filename, "w");
    }
}

void write_operate_log(){
    const char* tpl = randomTemplate(&operateTemplates);
    char rendered[MAX_RENDERED];

    renderTemplate(tpl, rendered);
    fprintf(operateLog, "%s\n", rendered);
    fflush(operateLog);
}

void writeErrorLog(){
    const char* tpl = randomTemplate(&errorTemplates);
    char rendered[MAX_RENDERED];

    renderTemplate(tpl, rendered);
    fprintf(errorLog, "%s\n", rendered);
    fflush(errorLog);
}

int main(){
    srand((unsigned int)time(NULL));

    loadTemplates("source_error.txt", &errorTemplates);
    loadTemplates("source_operate.txt", &operateTemplates);

    operateLog = fopen("operate.txt", "a");
    errorLog = fopen("error.txt", "a");

    if (!operateLog || !errorLog){
        printf("Failed to open log files\n");
        return 1;
    }

    printf("Machine Emulator Started\n");

    while (1){
        write_operate_log();

        if ((rand() % 100) < ERROR_CHANCE){
            writeErrorLog();
        }

        checkRotation(&operateLog,"operate.txt");
        checkRotation(&errorLog,"error.txt");
        Sleep(LOOP_DELAY_MS);
    }
    fclose(operateLog);
    fclose(errorLog);

    return 0;
}