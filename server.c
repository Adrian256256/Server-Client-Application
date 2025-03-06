#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/poll.h>
#include <stdbool.h>
#include <netinet/tcp.h>

struct udp_message {
    //50 octeti topic
    char topic[50];
    //unsigned int pe 1 octet
    char type;
    //continutul mesajului 1500 octeti
    char content[1500];
};

struct send_tcp_information {
    char ip[50];
    int port;
    char topic[50];
    char type;
    int length;
    char content[1500];
};

struct client {
    char id[50];
    int socket;
    char ip[50];
    int port;
    bool connected;
    char **subscribed_topics;
    int n_topics;
    int n_topics_max;
};

struct my_protocol {
    int type; /* 0 pentru subscribe, 1 pentru unsubscribe*/
    char topic[50];
};

#define MAX_PFDS 3
#define MAX_CLIENTS 1
#define MAX_TOPICS 3

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    //./server <PORT>
    if (argc != 2) {
        perror("Usage: ./server <PORT>");
        return -1;
    }
    //citesc argumentele
    int port = atoi(argv[1]);

    //citesc de la tastatura intr-un buffer
    char buffer[100];

    //serverul primeste conexiuni de la clienti
    int socket_desc, client_sock, client_size;
    struct sockaddr_in server_addr, client_addr;
    char server_message[2000], client_message[2000];
    
    memset(server_message, 0, sizeof(server_message));
    memset(client_message, 0, sizeof(client_message));
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc < 0) {
        perror("error while creating socket");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    int reuse = 1;
    if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("error while setting socket options");
        close(socket_desc);
        exit(EXIT_FAILURE);
    }

    if (setsockopt(socket_desc, IPPROTO_TCP, TCP_NODELAY, &reuse, sizeof(reuse)) < 0) {
        perror("error while setting socket options");
        close(socket_desc);
        exit(EXIT_FAILURE);
    }

    // Bind the address struct to the socket
    if (bind(socket_desc, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("error while binding");
        return -1;
    }

    // Listen for clients
    if (listen(socket_desc, 1) < 0) {
        perror("error while listening");
        return -1;
    }

    struct pollfd *pfds = (struct pollfd *)malloc(MAX_PFDS * sizeof(struct pollfd));
    memset(pfds, 0, MAX_PFDS * sizeof(struct pollfd));
    int nfds;
    nfds = 0;
    int nfds_max = MAX_PFDS;

    // Add the listening socket to the set
    pfds[nfds].fd = socket_desc;
    pfds[nfds].events = POLLIN;
    nfds++;

    // Add the STDIN to the set
    pfds[nfds].fd = STDIN_FILENO;
    pfds[nfds].events = POLLIN;
    nfds++;

    //fac un socket pentru udp
    int udp_socket;
    struct sockaddr_in udp_server_addr, udp_client_addr;
    char udp_server_message[2000], udp_client_message[2000];
    memset(udp_server_message, 0, sizeof(udp_server_message));
    memset(udp_client_message, 0, sizeof(udp_client_message));
    memset(&udp_server_addr, 0, sizeof(udp_server_addr));
    memset(&udp_client_addr, 0, sizeof(udp_client_addr));

    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        perror("Error while creating udp socket");
        return -1;
    }

    udp_server_addr.sin_family = AF_INET;
    udp_server_addr.sin_addr.s_addr = INADDR_ANY;
    udp_server_addr.sin_port = htons(port);
    reuse = 1;
    if (setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("Eroare la setarea opțiunii SO_REUSEADDR");
        close(udp_socket);
        exit(EXIT_FAILURE);
    }

    if (bind(udp_socket, (struct sockaddr *) &udp_server_addr, sizeof(udp_server_addr)) < 0) {
        perror("Error while binding udp socket");
        return -1;
    }

    //adaug socketul udp in lista de socketuri
    pfds[nfds].fd = udp_socket;
    pfds[nfds].events = POLLIN;
    nfds++;

    struct client *clients = (struct client *)malloc(MAX_CLIENTS * sizeof(struct client));
    int n_clients = 0;
    //aloc memorie pentru topicurile la care este abonat clientul
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].subscribed_topics = (char **)malloc(MAX_TOPICS * sizeof(char *));
        for (int j = 0; j < MAX_TOPICS; j++) {
            clients[i].subscribed_topics[j] = (char *)malloc(51 * sizeof(char));
        }
    }
    int cap = MAX_CLIENTS;

    while(1) {
        poll(pfds, nfds, -1);
        
        //am o conexiune noua
        if (pfds[0].revents & POLLIN) {
            // accept conexiunea
            client_size = sizeof(client_addr);
            client_sock = accept(socket_desc, (struct sockaddr *) &client_addr, (socklen_t*) &client_size);
            if (client_sock < 0) {
                perror("Error while accepting connection");
                return -1;
            }
            int flag = 1;
            int result = setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
            if (result < 0) {
                perror("setsockopt() failed");
                exit(EXIT_FAILURE);
            }
            //verific daca mai e loc in lista de socketuri, daca nu fac realloc
            if (nfds == nfds_max) {
                nfds_max *= 2;
                pfds = (struct pollfd *)realloc(pfds, nfds_max * sizeof(struct pollfd));
            }

            // adaug socketul clientului in lista de socketuri
            pfds[nfds].fd = client_sock;
            pfds[nfds].events = POLLIN;
            nfds++;

            //accept un mesaj de la client cu id-ul lui
            struct my_protocol *msg = (struct my_protocol *)malloc(sizeof(struct my_protocol));
            memset(msg, 0, sizeof(struct my_protocol));
            int len = recv(client_sock, msg, sizeof(struct my_protocol), 0);
            if (len < 0) {
                perror("Error while receiving message from client");
                return -1;
            }
            if (len == 0) {
                printf("Client disconnected\n");
                return 0;
            }
            //verific daca mai exista acest id in lista de clienti
            int ok = 0;
            for (int i = 0; i < n_clients; i++) {
                if (strncmp(clients[i].id, msg->topic, sizeof(clients[i].id)) == 0 && strlen(clients[i].id) == strlen(msg->topic)) {
                    //verific daca clientul este conectat
                    if (clients[i].connected == true) {
                        printf("Client %s already connected.\n", msg->topic);
                        close(client_sock);
                        nfds--;
                        ok = 1;
                        free(msg);
                        break;
                    } else {
                        //clientul a revenit online, ii actualizez socketul
                        clients[i].socket = client_sock;
                        clients[i].connected = true;
                        //New client <ID_CLIENT> connected from IP:PORT.
                        printf("New client %s connected from %s:%d.\n", clients[i].id, clients[i].ip, clients[i].port);
                        ok = 1;
                        free(msg);
                        break;
                    }
                }
            }
            if (ok == 1) {
                continue;
            }
            //daca nu exista clientul in lista, il adaug
            //mai intai verific daca mai e loc in lista de clienti
            if (n_clients == cap) {
                cap *= 2;
                //maresc dimensiunea listei de clienti cu realloc
                clients = (struct client *)realloc(clients, cap * sizeof(struct client));
                //aloc memorie pentru topicurile la care este abonat clientul
                for (int i = n_clients; i < 2 * n_clients; i++) {
                    clients[i].subscribed_topics = (char **)malloc(MAX_TOPICS * sizeof(char *));
                    for (int j = 0; j < MAX_TOPICS; j++) {
                        clients[i].subscribed_topics[j] = (char *)malloc(51 * sizeof(char));
                    }
                }
            }
            memcpy(clients[n_clients].id, msg->topic, sizeof(clients[n_clients].id));
            clients[n_clients].socket = client_sock;
            memcpy(clients[n_clients].ip, inet_ntoa(client_addr.sin_addr), sizeof(clients[n_clients].ip));
            clients[n_clients].port = ntohs(client_addr.sin_port);
            clients[n_clients].connected = true;
            clients[n_clients].n_topics = 0;
            clients[n_clients].n_topics_max = MAX_TOPICS;
            n_clients++;
            printf("New client %s connected from %s:%d.\n", clients[n_clients - 1].id, clients[n_clients - 1].ip, clients[n_clients - 1].port);
            free(msg);
        }
        
        //am primit mesaj de la tastatura
        if (pfds[1].revents & POLLIN) {
            //citesc de la tastatura intr-un buffer
            fgets(buffer, 100, stdin);
            //daca primesc exit, inchid programul
            if (strcmp(buffer, "exit\n") == 0) {
                //inchid toti socketii
                for (int i = 1; i < nfds; i++) {
                    close(pfds[i].fd);
                }
                //free la tot
                for (int i = 0; i < cap; i++) {
                    for (int j = 0; j < clients[i].n_topics_max; j++) {
                        free(clients[i].subscribed_topics[j]);
                    }
                    free(clients[i].subscribed_topics);
                }
                free(clients);
                free(pfds);
                return 0;
            }
        }

        //am primit mesaj de la udp
        if (pfds[2].revents & POLLIN) {
            memset(udp_client_message, 0, sizeof(udp_client_message));
            int len = recvfrom(udp_socket, udp_client_message, sizeof(udp_client_message), 0, (struct sockaddr *) &udp_client_addr, (socklen_t*) &client_size);
            if (len < 0) {
                perror("Error while receiving message from udp client");
                return -1;
            }
            if (len == 0) {
                perror("Client disconnected");
                return 0;
            }
            //trimit mesajul la clientii abonati la topic
            struct udp_message *msg = (struct udp_message *)udp_client_message;
            for (int i = 0; i < n_clients; i++) {
                for (int j = 0; j < clients[i].n_topics; j++) {
                    if (strcmp(clients[i].subscribed_topics[j], msg->topic) == 0) {
                        struct send_tcp_information send_msg;
                        memset(&send_msg, 0, sizeof(send_msg));
                        strcpy(send_msg.ip, inet_ntoa(udp_client_addr.sin_addr));
                        send_msg.port = ntohs(udp_client_addr.sin_port);
                        strcpy(send_msg.topic, msg->topic);
                        send_msg.type = msg->type;
                        memcpy(send_msg.content, msg->content, sizeof(msg->content));
                        send_msg.length = htonl(strlen(msg->content));
                        
                        send(clients[i].socket, &send_msg, sizeof(send_msg), 0);                       
                    }
                }
            }
            
        }
        for (int i = 3; i < nfds; i++) {
            if (pfds[i].revents & POLLIN) {
                //primesc mesaj de la client
                memset(client_message, 0, sizeof(client_message));
                int len = recv(pfds[i].fd, client_message, sizeof(client_message), 0);
                if (len < 0) {
                    perror("Error while receiving message from client");
                    return -1;
                }
                if (len == 0) {
                    //stim ca clientul s-a deconectat
                    char id[50];
                    //caut clientul in lista de clienti si il marchez ca deconectat
                    for (int j = 0; j < n_clients; j++) {
                        if (clients[j].socket == pfds[i].fd) {
                            clients[j].connected = false;
                            //inchid socketul
                            close(pfds[i].fd);
                            strcpy(id, clients[j].id);
                            //scot socketul din lista de socketuri
                            for (int k = i; k < nfds - 1; k++) {
                                pfds[k] = pfds[k + 1];
                            }
                            nfds--;
                            break;
                        }
                    }
                    printf("Client %s disconnected.\n", id);

                    continue;
                }
                struct my_protocol *msg = (struct my_protocol *)client_message;
                if (ntohl(msg->type) == 0) {
                    //msg->topic are newline la final, il sterg
                    msg->topic[strlen(msg->topic) - 1] = '\0';
                    //adaug topicul la lista de topicuri la care este abonat clientul
                    for (int j = 0; j < n_clients; j++) {
                        if (clients[j].socket == pfds[i].fd) {
                            //verific daca mai este abonat la acelasi topic
                            int ok = 0;
                            for (int k = 0; k < clients[j].n_topics; k++) {
                                if (strcmp(clients[j].subscribed_topics[k], msg->topic) == 0) {
                                    ok = 1;
                                    break;
                                }
                            }
                            if (ok == 1) {
                                break;
                            }
                            //verific daca mai am loc, daca nu dau realloc
                            if (clients[j].n_topics == clients[j].n_topics_max) {
                                clients[j].n_topics_max *= 2;
                                clients[j].subscribed_topics = (char **)realloc(clients[j].subscribed_topics, clients[j].n_topics_max * sizeof(char *));
                                for (int k = clients[j].n_topics; k < clients[j].n_topics_max; k++) {
                                    clients[j].subscribed_topics[k] = (char *)malloc(51 * sizeof(char));
                                }
                            }
                            strcpy(clients[j].subscribed_topics[clients[j].n_topics], msg->topic);
                            clients[j].n_topics++;
                            break;
                        }
                    }
                }
                if (ntohl(msg->type) == 1) {
                    //sterg topicul din lista de topicuri la care este abonat clientul
                    for (int j = 0; j < n_clients; j++) {
                        if (clients[j].socket == pfds[i].fd) {
                            for (int k = 0; k < clients[j].n_topics; k++) {
                                if (strcmp(clients[j].subscribed_topics[k], msg->topic) == 0) {
                                    for (int l = k; l < clients[j].n_topics - 1; l++) {
                                        strcpy(clients[j].subscribed_topics[l], clients[j].subscribed_topics[l + 1]);
                                    }
                                    clients[j].n_topics--;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
                if (msg->type == 2) {
                    //am primit id-ul clientului nou abonat
                    for (int j = 0; j < n_clients; j++) {
                        if (clients[j].socket == pfds[i].fd) {
                            strcpy(clients[j].id, msg->topic);
                            break;
                        }
                    }
                }
                
            }
        }

    }
    return 0;
}