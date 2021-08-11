// Adding a comment to setup the second commit
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BASE_SPEED 737
#define TIME_SAMPLE 20
#define INPUT_FREQUENCY 2
#define PORT 25000

// Co-efficients
#define Kp 8
#define Ki 1.3
#define Kd 2.0
#define Km 0.008

float Eold = 0, integral = 0, differential = 0;
int sock, input_freq = 0, packet_no = 0, err_value = 0, pid = 0, output = 737;

void calculate_speed_differential (int freq) {
    // Capture value from Encoder program and calculate error/difference in speed.
    // Formula ---> E = Finp - Fenc.
    char output_command[100] = {0};

    int quotient = packet_no/1000;
    int odd_even_flag = quotient%2;

    if (odd_even_flag == 1) {
        input_freq = 0;
    } else {
        input_freq = INPUT_FREQUENCY;
    }

    if (output < 737) {
        freq *= -1;
    }

    err_value = (input_freq * 10) - freq;
    // printf("Buffer contents: %d\n", freq);
    // printf("Error Value: %d\n", err_value);

    // Calculate the integral error. Formula ---> I = I + E * Tsample.
    integral = integral + (err_value * TIME_SAMPLE);
    // printf("Integral Value: %f\n", integral);

    // Calculate differential error. Formula ---> D = (E-Eold) / Tsample.
    differential = (err_value - Eold) / TIME_SAMPLE;
    // printf("Differential Value: %f\n", differential);
    Eold = err_value;

    // Calculate PID value. Formula ---> PID = Kp * E + Ki * I + Kd * D
    pid = (err_value * Kp) + (integral * Ki) + (differential * Kd);
    // printf("PID Value: %d\n", pid);

    output = BASE_SPEED + (pid * Km);

    int graph_plot = packet_no * 20;
    printf ("%d %d\n", graph_plot, output);

    snprintf (output_command, sizeof(output_command), "echo %d 1\\\\ > /dev/serial0", output);
    // printf("Output Command: %s\n", output_command);
    system(output_command);
}

void *child_socket_handler (void *sock_desc) {
    // Get the socket descriptor
    int sock = *(int*)sock_desc;
    int data_size, frequency;

    // Receive a message from client
    while ((data_size = recv(sock, &frequency, 4, 0)) > 0) {
        packet_no += 1;
        calculate_speed_differential(frequency);
    }

    if (data_size == 0) {
        printf("\nEncoder client disconnected.");
    } else if (data_size == -1) {
        printf("\nData not sent from encoder.");
    }

    // Free the socket pointer
    free(sock_desc);
}

void socket_connection () {
    int server_fd, value_read, *child_sock, option = 1;
    struct sockaddr_in ip_address;
    int address_len = sizeof(ip_address);

    // Create socket file descriptor.
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("\nSocket Failed!!");
        exit(EXIT_FAILURE);
    }

    if (setsockopt (server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option))) {
        perror("\nSetSockOpt");
        exit(EXIT_FAILURE);
    }

    ip_address.sin_family = AF_INET;
    ip_address.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "192.168.23.2", &ip_address.sin_addr) <= 0) {
        printf("Invalid server ip address.\n");
        return;
    }

    // Binding socket to the port 25000
    if (bind(server_fd, (struct sockaddr *) &ip_address, sizeof(ip_address)) < 0) {
        perror("Bind Failed");
        exit(EXIT_FAILURE);
    }

    // Listen to the connection.
    if (listen(server_fd, 3) < 0) {
        perror("Listen");
        exit(EXIT_FAILURE);
    }

    while ((sock = accept(server_fd, (struct sockaddr *) &ip_address, (socklen_t*) &address_len))) {
        pthread_t child_thread;
        child_sock = (int*) malloc(1);
        *child_sock = sock;

        if (pthread_create(&child_thread, NULL, child_socket_handler, (void*) child_sock) < 0) {
            printf("\nError!! Creating Controller listener thread.");
        }
    }

    if (sock < 0) {
        perror("Socket Connection Closed");
        exit(EXIT_FAILURE);
    }
}

void main () {
    socket_connection ();
}
