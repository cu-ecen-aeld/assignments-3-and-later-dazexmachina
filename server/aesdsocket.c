#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

int terminate = 0;
int run_daemon = 0;

void handle_sigint_sigterm(int sig) {
    terminate = 1;
}

int recv_line(int sock_fd, char** line) {
    char c;
    ssize_t n_read;
    size_t line_size = 1024;
    int i = 0;

    *line = (char*)malloc(line_size * sizeof(char));

    if (*line == NULL) {
        perror("malloc");

        return -1;
    }
    while (1) {
        if (terminate == 1) {
            return i;
        }
        if (i == line_size) {
            line_size *= 2;
            *line = realloc(*line, line_size * sizeof(char));

            if (*line == NULL) {
                perror("realloc");

                return -1;
            }   
        }
        n_read = recv(sock_fd, &c, 1, 0);

        if (n_read < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                usleep(100000);
                continue;
            }
            else {
                perror("recv");

                return i;
            }
        }
        else if (n_read == 0 || c == '\n') {
            break;
        }
        else {
            (*line)[i] = c;
            i++;
        }
    }
    
    return i;
}

int write_data_to_file(const char* file_name, char* data, int len) {
    FILE* file = fopen(file_name, "a");
    int n_written = 0;

    if (file == NULL) {
        perror("fopen");

        return -1;
    }
    for (int i = 0; i < len; i++) {
        if (fputc(data[i], file) == EOF) {
            perror("fputc");
            fclose(file);

            return -1;
        }
        n_written++;
    }
    fputc('\n', file);
    fclose(file);
    
    return n_written;
}

int send_file(int sock_fd, const char* file_name) {
    size_t line_size = 1024;
    FILE* file = fopen(file_name, "r");
    char* line = (char*)malloc(line_size * sizeof(char));
    int i = 0;
    int c;
    ssize_t n_total_sent = 0;

    if (file == NULL) {
        perror("fopen");

        return -1;
    }
    c = fgetc(file);
    
    while (c != EOF) {
        while (c != EOF && (char)c != '\n') {
            if (i == line_size) {
                line_size *= 2;
                line = (char*)realloc(line, line_size * sizeof(char));
            }
            line[i] = (char)c;
            i++;
            c = fgetc(file);
        }
        c = fgetc(file);

        if (i == line_size) {
            line_size *= 2;
            line = (char*)realloc(line, line_size * sizeof(char));
        }
        line[i] = '\n';
        i++;
        ssize_t n_sent = send(sock_fd, (void*)line, i, 0);

        if (n_sent < 0) {
            perror("send");
            fclose(file);
            free(line);

            return n_total_sent;
        }
        n_total_sent += n_sent;
        i = 0;
    }
    fclose(file);
    free(line);

    return n_total_sent;
}

int main(int argc, char** argv) {
    if (signal(SIGINT, handle_sigint_sigterm) == SIG_ERR) {
        perror("signal");
    }
    if (signal(SIGTERM, handle_sigint_sigterm) == SIG_ERR) {
        perror("signal");
    }
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        run_daemon = 1;
    }
    const char* file_name = "/var/tmp/aesdsocketdata";

    while (terminate == 0) {
        int listen_sock_fd;
        int optval = 1;
        struct addrinfo hints;
        struct addrinfo* server_info;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        if (getaddrinfo(NULL, "9000", &hints, &server_info) != 0) {
            perror("getaddrinfo");

            return -1;
        }   
        listen_sock_fd = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

        if (listen_sock_fd == -1) {
            perror("socket");
            freeaddrinfo(server_info);

            return -1;
        }
        int listen_flags = fcntl(listen_sock_fd, F_GETFL, 0);
        if (fcntl(listen_sock_fd, F_SETFL, listen_flags | O_NONBLOCK) == -1) {
            perror("fcntl");
            
            return -1;
        }
        if (setsockopt(listen_sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) != 0) {
            perror("setsockopt");
            close(listen_sock_fd);
            freeaddrinfo(server_info);

            return -1;
        }
        if (bind(listen_sock_fd, server_info->ai_addr, server_info->ai_addrlen) == -1) {
            perror("bind");
            close(listen_sock_fd);
            freeaddrinfo(server_info);

            return -1;
        }
        freeaddrinfo(server_info);

        if (run_daemon == 1) {
            run_daemon = 0;
            pid_t pid = fork();

            if (pid > 0) {
                return 0;
            }
            else if (pid == 0) {
                close(STDIN_FILENO);
                close(STDOUT_FILENO);
                close(STDERR_FILENO);
                setsid();
            }
            else {
                perror("fork");
                close(listen_sock_fd);

                return -1;
            }
        }
        if (listen(listen_sock_fd, 1) == -1) {
            perror("listen");
            close(listen_sock_fd);

            return -1;
        }
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int data_sock_fd = -1;

        while (data_sock_fd < 0) {
            if (terminate == 1) {
                shutdown(listen_sock_fd, SHUT_RDWR);
                close(listen_sock_fd);
                syslog(LOG_INFO, "Caught signal, exiting");

                if (remove(file_name) != 0) {
                    perror("remove");

                    return -1;
                }
   
                return 0;
            }
            data_sock_fd = accept(listen_sock_fd, (struct sockaddr*)&client_addr, &client_addr_len);
   
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                usleep(100000);
                continue;
            }
            else {
                perror("accept");
                close(listen_sock_fd);

                return -1;
            }
        }

        int data_flags = fcntl(data_sock_fd, F_GETFL, 0);
        if (fcntl(data_sock_fd, F_SETFL, data_flags | O_NONBLOCK) == -1) {
            perror("fcntl");
            
            return -1;
        }
        syslog(LOG_INFO, "Accepted connection from %s\n", inet_ntoa(client_addr.sin_addr));

        char** buff = (char**)malloc(sizeof(char*));
        int line_len = recv_line(data_sock_fd, buff);
        
        if (line_len > 1) {
            write_data_to_file(file_name, *buff, line_len);
            send_file(data_sock_fd, file_name);
        }
        if (buff != NULL && *buff != NULL) {
            free(*buff);
        }
        if (buff != NULL) {
            free(buff);
        }

        shutdown(listen_sock_fd, SHUT_RDWR);
        close(listen_sock_fd);
        shutdown(data_sock_fd, SHUT_RDWR);
        close(data_sock_fd);

        syslog(LOG_INFO, "Closed connection from %s\n", inet_ntoa(client_addr.sin_addr));
    }
    if (remove(file_name) != 0) {
        perror("remove");
    }
    syslog(LOG_INFO, "Caught signal, exiting");

    return 0;
}
