# Server-Client Application

**Author:** Harea Teodor-Adrian  
**Class:** 323CA

---

## Overview

This project implements two independent entities, a client and a server, written in C, communicating via TCP and UDP sockets. The application uses I/O multiplexing with `poll` to handle simultaneous input from both sockets and the keyboard.

---

## Files

- **client.c**  
  Implements the client-side functionality.

- **server.c**  
  Implements the server-side functionality.

---

## Key Structures

- **my_protocol**  
  Facilitates communication from client to server, enabling subscribe/unsubscribe operations and transmitting subscriber IDs. The `type` field indicates the operation type, and `topic` holds the relevant topic or subscriber ID.

- **udp_message**  
  Represents messages received by the server from UDP clients, containing predefined fields: topic, type, and content.

- **send_tcp_information**  
  Used by the server to forward UDP client messages to TCP clients. Extends `udp_message` by including the UDP client's IP and port for display purposes.

- **client**  
  Used on the server side to store client information such as ID, socket descriptor, IP, port, connection status, subscribed topics, and topic counts. Initial allocations for clients and topics are small but dynamically expanded as needed.

---

## Features

- I/O multiplexing with `poll` to manage socket and keyboard input concurrently.  
- Dynamic allocation and resizing of client and topic lists to avoid fixed limits.  
- Proper error handling with checks on system call returns and use of `perror` on failure.  
- Detection and handling of invalid inputs to prevent undefined or unpredictable behavior.  
- Disabling Nagle’s algorithm as required.  
- Graceful server shutdown notification to clients, freeing resources properly.  
- Network-to-host and host-to-network byte order conversions for message transmission.

---
