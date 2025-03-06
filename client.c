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
#include <math.h>
#include <netinet/tcp.h>

struct my_protocol {
    int type; /* 0 pentru subscribe, 1 pentru unsubscribe, 2 pentru trimitere id*/
    char topic[50];
};

struct send_tcp_information {
    char ip[50];
    int port;
    char topic[50];
    char type;
    int length;
    char content[1500];
};

struct udp_message {
    //50 octeti topic
    char topic[50];
    //unsigned int pe 1 octet
    char type;
    //continutul mesajului 1500 octeti
    char content[1500];
};

#define MAX_PFDS 32

int get_message_from_server(int sockfd) {
    //type 0 -> int, type 1 -> short_real, type 2 -> float, type 3 -> string
    struct send_tcp_information *msg = (struct send_tcp_information *)malloc(sizeof(struct send_tcp_information));
    memset(msg, 0, sizeof(struct send_tcp_information));
    int size_to_recv = sizeof(struct send_tcp_information);
    char *buffer = (char *)msg;
    int bytes_recv = 0;
    while (bytes_recv < size_to_recv) {
        int ret = recv(sockfd, buffer + bytes_recv, size_to_recv - bytes_recv, 0);
        if (ret < 0) {
            perror("recv() failed");
            return -1;
        }
        if (ret == 0) {
            return -1;
        }
        bytes_recv += ret;
    }


    //salvez mesajul primit in variabile
    char *ip = msg->ip;
    int port = msg->port;
    char *topic = msg->topic;
    char type_received = msg->type;
    char *content = msg->content;
    //prelucrez content - ul in functie de type
    if (type_received == 0) {
        uint32_t value = ntohl(*(uint32_t *)(content + 1));
        // primul octet este pentru semn
        if (msg->content[0] == 1) {
            value = -value;
        }
        //1.2.3.4:4573 - UPB/precis/1/temperature - SHORT_REAL - 23.5
        char type[5] = "INT";
        printf("%s:%d - %s - %s - %d\n", ip, port, topic, type, value);
        free(msg);
        return 0;
    }
    if (type_received == 1) {
        //avem modulul numarului inmultit cu 100, fac pasii pentru a reveni la numar
        double value = ntohs(*(uint16_t *)content) / 100.0;
        
        printf("%s:%d - %s - SHORT_REAL - %lf\n", ip, port, topic, value);
        free(msg);
        return 0;
    }
    if (type_received == 2) {
        //octet de semn + uint32_t in network order + uint8_t putere negativa a lui 10
        double value = ntohl(*(uint32_t *)(content + 1));
        //primul octet este pentru semn
        if (content[0] == 1) {
            value = -(value / pow(10, msg->content[5]));
        } else {
            value = value / pow(10, msg->content[5]);
        }
        char type[6] = "FLOAT";
        printf("%s:%d - %s - %s - %lf\n", ip, port, topic, type, value);
        free(msg);
        return 0;
    }
    if (type_received == 3) {
        char type[7] = "STRING";
        printf("%s:%d - %s - %s - %s\n", ip, port, topic, type, content);
        free(msg);
        return 0;
    }
    return 0;
}

int read_from_stdin(int sockfd) {
    char buffer[100];
    memset(buffer, 0, sizeof(buffer));
    fgets(buffer, 100, stdin);
    //daca am primit comanda quit, inchid socketul si ies
    if (strncmp(buffer, "exit\n", 4) == 0) {
        close(sockfd);
        return -1;
    }
    //am primit subscribe de la tastatura
    if (strncmp(buffer, "subscribe", 9) == 0) {
        //trimit mesaj la server cu topicul la care vreau sa ma abonez
        struct my_protocol *msg = (struct my_protocol *)malloc(sizeof(struct my_protocol));
        msg->type = htonl(0);
        //intr-un char *topic iau ce este dupa subscribe
        strtok(buffer, " ");
        memcpy(msg->topic, strtok(NULL, " "), 51);
        send(sockfd, msg, sizeof(struct my_protocol), 0);
        //Subscribed to topic X
        printf("Subscribed to topic %s", msg->topic);
        free(msg);
        return 0;
    }
    //am primit unsubscribe de la tastatura
    if (strncmp(buffer, "unsubscribe", 11) == 0) {
        //trimit mesaj la server cu topicul de la care vreau sa ma dezabonez
        struct my_protocol *msg = (struct my_protocol *)malloc(sizeof(struct my_protocol));
        msg->type = htonl(1);
        strtok(buffer, " ");
        memcpy(msg->topic, strtok(NULL, " "), 50);
        //Subscribed to topic X
        printf("Unsubscribed from topic %s\n", msg->topic);
        send(sockfd, msg, sizeof(struct my_protocol), 0);
        free(msg);
        return 0;
    }
    //daca ajung aici, eroare, comanda necunoscuta
    perror("Unknown command");
    return 0;
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    //./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>
    if (argc != 4) {
        perror("Usage: ./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>");
        return -1;
    }
    //citesc argumentele
    char *id_client = argv[1];
    char *ip_server = argv[2];
    int port_server = atoi(argv[3]);

    //conectare la server
    int sockfd;
    struct sockaddr_in serv_addr;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_server);
    serv_addr.sin_addr.s_addr = inet_addr(ip_server);

    if (inet_pton(AF_INET, ip_server, &serv_addr.sin_addr) <= 0) {
        perror("invalid address / address not supported");
        return -1;
    }
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connection failed");
        return -1;
    }
    struct my_protocol *msg = (struct my_protocol *)malloc(sizeof(struct my_protocol));
    memset(msg, 0, sizeof(struct my_protocol));
    msg->type = htonl(2);
    memcpy(msg->topic, id_client, 50);
    send(sockfd, msg, sizeof(struct my_protocol), 0);
    free(msg);
    int flag = 1;
    int result = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
    if (result < 0) {
        perror("setsockopt() failed");
        exit(EXIT_FAILURE);
    }
    struct pollfd pfds[MAX_PFDS];
    int nfds;
    nfds = 0;
    //adaug socketul de la server in lista de socketuri
    pfds[nfds].fd = sockfd;
    pfds[nfds].events = POLLIN;
    nfds++;
    //adaug stdin in lista de socketuri
    pfds[nfds].fd = 0;
    pfds[nfds].events = POLLIN;
    nfds++;

    while(1) {
        int ret = poll(pfds, nfds, -1);
        if (ret < 0) {
            perror("poll() failed");
            return -1;
        }
        if (ret == 0) {
            perror("poll() timeout");
            return -1;
        }
        //mesaj de la server
        if (pfds[0].revents & POLLIN) {
            if(get_message_from_server(sockfd) == -1) {
                return 0;
            }
        }
        //mesaj de la tastatura
        if (pfds[1].revents & POLLIN) {
            if(read_from_stdin(sockfd) == -1) {
                return 0;
            }
        }
    }
    return 0;
}