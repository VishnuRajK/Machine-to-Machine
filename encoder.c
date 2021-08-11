#include <time.h>
#include <poll.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdbool.h>

#define PORT 25000  // The port used for socket

pthread_mutex_t lock;
bool direction = true;
struct sockaddr_in ip_address;
int frequency_data = 0, option = 1, sock, first_instance = 0;

//Function used to poll - Interrupts
void *poll_frequency() {
    char clock_wise[1];
    struct pollfd polldata;

    polldata.events = POLLPRI | POLLERR;
    polldata.fd = open("/sys/class/gpio/gpio9/value", O_RDONLY);

    while (1) {
        poll(&polldata, 1, 500);
        lseek(polldata.fd, 0, SEEK_SET);
        read(polldata.fd, clock_wise, 1);

        pthread_mutex_lock(&lock);
        frequency_data += 1;
        pthread_mutex_unlock(&lock);
    }
}

//Creating Socket and connecting to server
void socket_connection() {
    // Create socket file descriptor.
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket Failed!!");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option))) {
        perror("SetSockOpt");
        exit(EXIT_FAILURE);
    }

    ip_address.sin_family = AF_INET;
    ip_address.sin_port = htons(PORT); // putting it in host byte order - big endian, little endian

    if (inet_pton(AF_INET, "192.168.23.2", &ip_address.sin_addr) <= 0) {
        printf("Invalid server ip address.\n");
        return;
    }

    if (connect(sock, (struct sockaddr *)&ip_address, sizeof(ip_address)) < 0) {
        printf("Socket Connection Failed.\n");
        return;
    }
}

//Writing into the server socket
void write_to_socket (int* frequency) {
    printf("Frequency: %d\n", *frequency);
    send(sock, frequency, sizeof(frequency), 0);
}

//Frequency calculation
void *write_frequency() {
    while (1) {
        usleep(20000);
        pthread_mutex_lock(&lock);
        if (first_instance == 0) {
            first_instance = 1;
            if (!direction) {
                frequency_data *= -1;
            }
        }

        printf("Frequency: %d\n", frequency_data);
        write_to_socket(&frequency_data);
        frequency_data = 0; // Reset to Zero.
        pthread_mutex_unlock(&lock);
    }
}

// Direction is clockwise if value is true and anticlockwise if value is false.
void *detect_direction() {
    char clock_wise[1];
    char anticlock_wise[1];

    struct pollfd polldata[2];
    int toToggle = 0, valueA = 0, valueB = 0, loop = 100;

    polldata[0].events = POLLPRI | POLLERR;
    polldata[0].fd = open("/sys/class/gpio/gpio9/value", O_RDONLY);

    polldata[1].events = POLLPRI | POLLERR;
    polldata[1].fd = open("/sys/class/gpio/gpio10/value", O_RDONLY);

    while (loop) {
        lseek(polldata[0].fd, 0, SEEK_SET);
        read(polldata[0].fd, clock_wise, 1);

        lseek(polldata[1].fd, 0, SEEK_SET);
        read(polldata[1].fd, anticlock_wise, 1);

        valueA = atoi(clock_wise);
        valueB = atoi(anticlock_wise);

        if (valueA == valueB && valueA == 0) {
            toToggle = 1;
        } else if (toToggle == 1 && valueA != valueB) {
            toToggle = 0;
            if (valueB == 1) {
                direction = false;
            } else {
                direction = true;
            }
        }
        printf("Direction: %d\n", direction);
        loop -= 1;
    }
}

//Main function - threads created for polling and frequency calculation
void main() {
    socket_connection();
    system("echo 9 > /sys/class/gpio/unexport");
    system("echo 9 > /sys/class/gpio/export");
    system("echo in > /sys/class/gpio/gpio9/direction");
    system("echo rising > /sys/class/gpio/gpio9/edge");

    system("echo 10 > /sys/class/gpio/unexport");
    system("echo 10 > /sys/class/gpio/export");
    system("echo in > /sys/class/gpio/gpio10/direction");
    system("echo none > /sys/class/gpio/gpio10/edge");

    pthread_t pollf_id;
    pthread_t writef_id;
    pthread_t poll_direction_id;

    int poll_direction_err = pthread_create(&poll_direction_id, NULL, detect_direction, NULL);
    poll_direction_err = pthread_join(poll_direction_id, NULL);
    if (poll_direction_err) {
        printf("%d", poll_direction_err);
    }

    usleep(100000);
    int pollf_err = pthread_create(&pollf_id, NULL, poll_frequency, NULL);
    int writef_err = pthread_create(&writef_id, NULL, write_frequency, NULL);

    pollf_err = pthread_join(pollf_id, NULL);
    if (pollf_err) {
        printf("%d", pollf_err);
    }

    writef_err = pthread_join(writef_id, NULL);
    if (writef_err) {
        printf("%d", writef_err);
    }
}