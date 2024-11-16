#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024

void print_hash(unsigned char *digest, size_t length)
{
    printf("%s: ", "SHA-256");
    for (size_t i = 0; i < length; i++)
    {
        printf("%02X", digest[i]);
    }
    printf("%s", "\n");
}
void print_binary(unsigned char *data, size_t length)
{
    for (size_t i = 0; i < length; ++i)
    {
        for (int bit = 7; bit >= 0; --bit)
        {
            printf("%d", (data[i] >> bit) & 1); // Print each bit
        }
        printf(" "); // Space between bytes for readability
    }
    printf("\n");
}

unsigned char *Sha256Hash(const char *Name)
{
    SHA256_CTX sha256_ctx;
    SHA256_Init(&sha256_ctx);
    SHA256_Update(&sha256_ctx, Name, strlen(Name));
    unsigned char *sha256_digest = malloc(SHA256_DIGEST_LENGTH);
    if (sha256_digest == NULL)
    {
        perror("Failed to allocate memory for SHA256 digest");
        exit(EXIT_FAILURE);
    }
    SHA256_Final(sha256_digest, &sha256_ctx);
    // print_binary(sha256_digest, SHA256_DIGEST_LENGTH);
    print_hash(sha256_digest, SHA256_DIGEST_LENGTH);
    return sha256_digest;
}

void binaryToHex(const unsigned char *data, size_t length, char *hexStr)
{
    for (size_t i = 0; i < length; ++i)
    {
        sprintf(hexStr + i * 2, "%02X", data[i]);
    }
    hexStr[length * 2] = '\0'; 
}

void takingUserNameOrPassword(int client_fd, const char *msg)
{
    printf("Enter %s", msg);
    fflush(stdout);

    char str[BUFFER_SIZE];
    if (fgets(str, sizeof(str), stdin) != NULL)
    {
        size_t len = strlen(str);
        if (len > 0 && str[len - 1] == '\n')
        {
            str[len - 1] = '\0'; // Remove newline character
        }

        unsigned char *hashedPassword = Sha256Hash(str);
        char hexStrPassword[SHA256_DIGEST_LENGTH * 2 + 1]; // Allocate buffer for hex string

        binaryToHex(hashedPassword, SHA256_DIGEST_LENGTH, hexStrPassword);
        free(hashedPassword); // Free the memory after use

        printf("\nGot to print: %s\n\n", hexStrPassword);
        fflush(stdout);

        // Send hexStrPassword to server
        // send(client_fd, hexStrPassword, strlen(hexStrPassword), 0);
    }
}

int main()
{
    takingUserNameOrPassword(0, "Password:\n");
    return 0;
}
