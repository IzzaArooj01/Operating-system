#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void xorEncryptDecrypt(char *filePath, const char *key)
{
    FILE *file = fopen(filePath, "rb+");
    if (file == NULL)
    {
        perror("File open error");
        return;
    }

    size_t keyLen = strlen(key);
    unsigned char buffer;

    size_t i = 0;
    while (fread(&buffer, sizeof(unsigned char), 1, file) == 1)
    {
        buffer ^= key[i % keyLen];
        fseek(file, -1, SEEK_CUR);
        fwrite(&buffer, sizeof(unsigned char), 1, file);
        i++;
    }

    fclose(file);
}
