**Harea Teodor-Adrian**
**323CA**
**Tema 2**
*Aplicatie client-server TCP si UDP pentru gestionarea mesajelor*

In cadrul acestei teme, am implementat 2 entitati si anume _clientul_ si _serverul_.
Codul este scris in limbajul C, si se pot remarca 2 fisiere specifice fiecarei entitati:

->client.c
->server.c

Clientul si serverul functioneaza independent si comunica prin intermediul socketilor.
Se utilizeaza biblioteca de socketi si se realizeaza multiplexare I/O. Atat clientul,
cat si serverul, se bazeaza pe un while in care se realizeaza multiplexarea
descriptorilor pentru socketi si pentru datele primite de la tastatura. Se adauga
socketii in poll si la fiecare rulare a while-ului se verifica daca s-a primit
printr-un socket ceva.

Mesajele sunt procesate cu ajutorul urmatoarelor structuri:

->my_protocol: aceasta structura faciliteaza transmiterea de informatii de catre client
spre server. Prin intermediul acesteia se realizeaza apelul de subscribe/unsubscribe
si trimiterea id-ului subscriberului nou conectat. Campul _type_ este cel care ii
spune serverului despre ce caz din cele 3 este vorba. In _topic_ se incadreaza id-ul
sau topicul la care se face subscribe/unsubscribe.

->udp_message: in aceasta structura se incadreaza mesajele primite de la clientii
udp de catre server. Conform enuntului, campurile sunt cele prestabilite: topic,
type si content.

->send_tcp_information: cu ajutorul acestei structuri serverul transmite mesajele
primite de la clientii udp catre clientii tcp. Aceasta este o structura ce contine
campurile din *udp_message*, dar are si campuri pentru ip-ul si port-ul clientului
udp care a transmis mesajul, pentru a transmite si aceste informatii clientului TCP,
pentru afisare.

->client: structura este folosita in cadrul serverului. Aceasta retine informatiile
despre clientii care se conecteaza la server(id-ul, socket-ul prin care comunica,
ip-ul, port-ul, daca este conectat sau nu, topicurile la care este abonat, numarul
acestora si numarul maxim alocat pentru topicuri)

Atat pentru structurile de clienti cat si pentru topicuri, se aloca initial un numar
mic de octeti, iar atunci cand este nevoie, se mareste spatiul alocat
pentru a face loc altor entitati. Prin acest procedeu, nu se limiteaza numarul spaiului
alocat.

Mesajele afisate sunt numai cele cerute in enunt. In cazul unei erori, se apeleaza
perror.

Sunt tratate cazurile de eroare, sunt verificate valorile intoarse de apelurile de
sistem si se gestioneaza cazurile in care lucrurile nu merg bine.

In cazul in care input-ul primit este invalid, programul stie acest lucru si nu se
ajunge in stari nedefinite sau la comportament impredictibil.

De asemenea, este dezactivat algoritmul lui Nagle, asa cum este cerut in enunt.

In cazul in care serverul se inchide, clientii primesc un mesaj cu len egal cu 0 si
se elibereaza restursele, urmand ca clientul sa se inchida.

Pentru trimiterea si primirea mesajelor, se realizeaza conversiile intre host si network
order.

