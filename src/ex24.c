#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUFFER_SIZE 256

void close_ctrl_errs(int fd, const char *m_error);

int main(int argc, char **argv) {
    if(argc != 3) {
        fprintf(stderr, "Usage: %s <source> <destination>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char* src = argv[1];
    char* dst = argv[2];

    struct stat src_sb;
    if(stat(src, &src_sb) == -1) {
        perror("lstat src");
        return EXIT_FAILURE;
    }

    if((src_sb.st_mode & S_IFMT) != S_IFREG) {
        fprintf(stderr, "Error, %s is not a regular file.\n", src);
        return EXIT_FAILURE;
    }

    struct stat dst_sb;
    errno = 0; // reset errno.
    int dst_stat_output = stat(dst, &dst_sb);
    int dst_exist = (errno == ENOENT) ? 0 : 1;

    if(dst_exist) {
        if(dst_stat_output == -1) {
            perror("lstat dst");
            return EXIT_FAILURE;
        }

        if((dst_sb.st_mode & S_IFMT) != S_IFREG) {
            fprintf(stderr, "Error, %s is not a regular file.\n", src);
            return EXIT_FAILURE;
        }

        if(src_sb.st_ino == dst_sb.st_ino && src_sb.st_dev == dst_sb.st_dev) {
            fprintf(stderr, "The two files %s and %s are the same.\n", src, dst);
            return EXIT_FAILURE;
        }
    }

    int tubes[2];
    pipe(tubes);

    pid_t read_pid = fork();
    if(read_pid == -1) { perror("fork"); return EXIT_FAILURE; }
    if(read_pid == 0) {
        close_ctrl_errs(tubes[0], "close read end pipe");

        FILE *src_file = fopen(src, "r");
        if(src_file == NULL) { perror("fopen src"); return EXIT_FAILURE; }

        char* buffer = calloc(BUFFER_SIZE, sizeof(char));
        if(buffer == NULL) { perror("calloc"); return EXIT_FAILURE; }
        size_t nb;
        while(!feof(src_file)) {
            nb = fread(buffer, sizeof(char), BUFFER_SIZE, src_file);

            if(write(tubes[1], buffer, nb) == -1) {
                perror("write read_process");
                return EXIT_FAILURE;
            }

        }

        free(buffer);
        fclose(src_file);
        close_ctrl_errs(tubes[1], "close write end pipe");
        return EXIT_SUCCESS;
    }

    close_ctrl_errs(tubes[1], "close write end pipe");

    pid_t write_pid = fork();
    if(write_pid == -1) { perror("fork"); return EXIT_FAILURE; }
    if(write_pid == 0) {
        FILE *dst_file = fopen(dst, "w");
        if(dst_file == NULL) { perror("fopen dst"); return EXIT_FAILURE; }

        char *buffer = calloc(BUFFER_SIZE, sizeof(char));
        if(buffer == NULL) { perror("calloc"); return EXIT_FAILURE; }

        ssize_t bytesRead;
        while((bytesRead = read(tubes[0], buffer, BUFFER_SIZE)) > 0) {
            fwrite(buffer, sizeof(char), bytesRead, dst_file);
            if(ferror(dst_file)) {
                perror("fwrite");
                return EXIT_FAILURE;
            }
        }

        free(buffer);
        close_ctrl_errs(tubes[0], "close read end pipe");
        if(fclose(dst_file) == EOF) {
            perror("fclose dst_file");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    close_ctrl_errs(tubes[0], "close read end pipe");
    wait(NULL); wait(NULL);
    return EXIT_SUCCESS;
}

void close_ctrl_errs(int fd, const char *m_error){
    if(close(fd) != 0) {
        perror(m_error);
        exit(EXIT_FAILURE);
    }
}

