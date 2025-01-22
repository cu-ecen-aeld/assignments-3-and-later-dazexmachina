#include <stdio.h>
#include <syslog.h>

void print_usage() {
    printf("Usage: ./writer <writefile> <writestr>\n");
    printf(" <writefile> Path to the file to write\n");
    printf(" <writestr> String to write to the file\n");
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Error: invalid arguments\n");
        print_usage();
        
        return 1;
    }
    openlog("writer", 0, 0);
    FILE* file = fopen(argv[1], "w");
    
    if (!file) {
        perror("fopen");
        syslog(LOG_ERR, "Failed to open file %s", argv[1]);
        
        return 1;
    }
    syslog(LOG_DEBUG, "Writing %s to %s", argv[2], argv[1]);

    if (fputs(argv[2], file) == EOF) {
        perror("fputs");
        syslog(LOG_ERR, "Failed to write string %s to file %s", argv[2], argv[1]);
        
        if (fclose(file) == EOF) {
            perror("fclose");
            syslog(LOG_ERR, "Failed to close file %s", argv[1]);
        }

        return 1;
    }
    if (fclose(file) == EOF) {
        perror("fclose");
        syslog(LOG_ERR, "Failed to close file %s", argv[1]);
    }

    return 0;
}
