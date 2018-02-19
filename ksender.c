#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

/** functie pentru a seta campurile unei structuri pentru un mesaj de tip S **/
void setSstruct(sendInitMsg* s){
    (*s).maxl = 250;
    (*s).time = 5;
    (*s).npad = 0;
    (*s).padc = 0;
    (*s).eol = 0x0D;
    (*s).qctl = 0;
    (*s).qbin = 0;
    (*s).chkt = 0;
    (*s).rept = 0;
    (*s).capa = 0;
    (*s).r = 0;
}

/** functie pentru a seta campurile unui mesaj care contine ca data un mesaj de tip S **/
void setminiKermitS(miniKermitS* m, char soh, char len, char seq, char type,
    sendInitMsg data, char mark){
    (*m).soh = soh;
    (*m).len = len;
    (*m).seq = seq;
    (*m).type = type;
    (*m).data = data;
    (*m).mark = mark;
}

/** functie pentru a seta campurile unui mesaj care contine ca data orice string **/
void setminiKermit(miniKermit* m, char soh, char len, char seq, char type,
    char* data, char mark){
    (*m).soh = soh;
    (*m).len = len;
    (*m).seq = seq;
    (*m).type = type;
    (*m).data = data;
    (*m).mark = mark;
}

/** functie care primeste un mesaj si caracteristicile lui si construieste primele campuri pentru a putea calcula crc-ul **/
void setMessage(msg *t, miniKermit k, int data_len){
  memcpy(&(*t).payload[0], &k.soh, 1);
  memcpy(&(*t).payload[1], &k.len, 1);
  memcpy(&(*t).payload[2], &k.seq, 1);
  memcpy(&(*t).payload[3], &k.type, 1);
  memcpy(&(*t).payload[4], k.data, data_len);
}

/** functie care seteaza campul check si mark pentru un mesaj dat **/
void setMessageCont(msg *t, miniKermit k, int data_len){
    memcpy(&(*t).payload[4 + data_len], &k.check, 2);
    memcpy(&(*t).payload[4 + data_len + 2], &k.mark, 1);
}

/** functie care construieste un mesaj de tip S, setand lugnimea si payload-ul **/
void createMsgS(msg *t, int *SEQ, int* crc){
    sendInitMsg s;
    miniKermitS m;
    /** initializam campurie structurii pentru mesajul de tip S **/
    setSstruct(&s);
    setminiKermitS(&m, 0x01, sizeof(s) + 5, *SEQ, S, s, 0x0D);
    /** calcula mcrc pentru noul mesaj **/
    *crc = crc16_ccitt(&m, sizeof(m) - 4);
    m.check = *crc;
    /** copiem datele de la adresa de memorie a structurii ce contine mesajul la adresa payload-ului mesajului de trimis **/
    memcpy(t->payload, &m, sizeof(m));
    t->len = sizeof(m);
}

/** functie care construieste mesaj se de scventa, crc, tip, date si lungime date **/
void reply(msg *t, int *SEQ, int* crc, char type, char* data, int len){
    miniKermit k;
    setminiKermit(&k, 0x01, len, *SEQ, type, data, 0x0D);
    /** construim un nou mesaj cu datele primite si calculam crc-ul corespunzator **/
    if(data != NULL){
        setMessage(t, k, strlen(data));
        *crc = crc16_ccitt(t->payload, strlen(data) + 4);
        k.check = *crc;
        setMessageCont(t, k, strlen(data));
        t->len = strlen(data) + 7;
    }
    else{
        setMessage(t, k, 0);
        *crc = crc16_ccitt(t->payload, 4);
        k.check = *crc;
        setMessageCont(t, k, 0);
        t->len = 7;
    }
}

/** functie care construieste mesaj de secventa, crc, tip, date si lungime date pentru pachete de tip D, pe baza numarului de bytes cititi, n **/
void replyData(msg *t, int *SEQ, int* crc, char type, char* data, int len, int n){
    miniKermit k;
    setminiKermit(&k, 0x01, len, *SEQ, type, data, 0x0D);
    /** construim un nou mesaj cu datele primite si calculam crc-ul corespunzator **/
    if(data != NULL){
        setMessage(t, k, n);
        *crc = crc16_ccitt(t->payload, n + 4);
        k.check = *crc;
        setMessageCont(t, k, n);
        t->len = n + 7;
    }
    else{
        setMessage(t, k, 0);
        *crc = crc16_ccitt(t->payload, 4);
        k.check = *crc;
        setMessageCont(t, k, 0);
        t->len = 7;
    }
}

/** functie care asteapta primirea unui ACK pentru un mesaj. 
    intoarce 0 in caz de timeout si 1 atunci cand se primeste ACK pentru mesajul dat **/
int waitForACK(msg *t, int time, int *SEQ, int* crc, int len, char type, char* data, int n){
    msg *y;
    int count_send = 1;
    char typeRecv;
    while(1){
        /** Daca primim un mesaj null, incrementam un contor pana ajungem la 3
        si retrimitem mesajul. Daca s-a trecut de a 3-a trimitere consecutiva fara a primi un raspuns,
        atunci executia programului se incheie **/
        y = receive_message_timeout(time * 1000);
        if (y == NULL) {
            if(count_send <= 3){
                printf("SENDER Timeout(%c)\n", type);
                send_message(t);
                count_send ++;
            }else{
                return 0;
            }       

        } else { 
            /** Verificam tipul mesajului primit **/
            typeRecv = y->payload[3];
            count_send = 1;
            if(typeRecv == N){
                /** Am primit un NACK, deci recalculam numarul de secventa si crc-ul, folosind functia reply sa udeplyData, si pregatim mesajul pentru a fi retrimis **/
                *SEQ = (*SEQ + 2) % 64;
                if(type != D){
                    reply(t, SEQ, crc, type, data, len);
                }else{
                    replyData(t, SEQ, crc, type, data, len, n);
                }   
                printf("SENDER retrimite (%c) cu SEQ: %d\n", type, *SEQ);
                send_message(t);
            }else if(typeRecv == Y){
                /** am primit un ACK, deci updatam numaul de secventa si functia se incheie cu succes**/
                *SEQ = (y->payload[2] + 1) % 64;
                printf("SENDER primeste ACK (%c) si SEQ devine: %d: \n",type, *SEQ);
                break;
            }
        }
    }

    return 1;
}

int main(int argc, char** argv) {
    msg t;   
    init(HOST, PORT);
    int time; 
    unsigned char maxl;
    char type;
    int crc;
    int SEQ = 0;
    /** construim si trimitem primul mesaj, de tip  S **/
    createMsgS(&t, &SEQ, &crc);
    send_message(&t);
    printf("SENDER trimite (S) cu SEQ: %d\n", SEQ);
    int count_send = 1;
    while(1){
        /** astpetam sa primim ACL sau NACK pentru mesajul tirmis **/
        msg *y = receive_message_timeout(5000);
        if (y == NULL) {
            /** am primit un measj null, deci verificam daca trebuie inchisa conexiunea (daca am retransmis mesajul de 3 ori consecutiv) **/
            if(count_send <= 3){
                send_message(&t);
                count_send ++;
                printf("SENDER timeout (S)\n");
            }else{
                return 0;
            }       

        } else {
            /** verificam tipul mesajului primit **/
            type = y->payload[3]; 
            count_send = 1;
            if(type == N){
                /** am primit un NACK, deci calculam noul numar de secventa si retrimitem mesajul**/
                SEQ = (SEQ + 2) % 64;
                printf("SENDER retrimite (S) cu seq:  %d\n", SEQ);
                createMsgS(&t, &SEQ, &crc);
                send_message(&t);
            }else if(type == Y){
                /** am primit un mesaj de tip ACK, deci extragem din el datele trimise de receiver si recalculam numarul de secventa
                pentru urmatoarele transmisii **/
                    maxl = (unsigned char) y->payload[4];
                    time = (int) y->payload[5];
                    SEQ = (y->payload[2] + 1)%64;
                    printf("Sender primeste ack(S)\n");
                    break;
                }
            }
        } 

    /** pentru fiecare fisier primit ca argument in linia de comanda **/
    for(int i = 1; i < argc; i++){
        /** incepem  prin trimiterea pachetului de tip F.
        construind mesajul avand in campul data numele fisierului**/
      int len = strlen(argv[i]) + 5;
      reply(&t, &SEQ, &crc, F, argv[i], len);
      printf("SENDER File Header (%s) cu SEQ: %d\n", argv[i], SEQ);
      send_message(&t);
      count_send = 1;
      /** apelam functia waitForAck care intoarce 0 in caz de timeout, si 1 cand s-a primit ACK pentru mesajul transmis **/
      int rez = waitForACK(&t, time, &SEQ, &crc, len, F, argv[i], 0);
      if(rez == 0){
        return 0;
      }  

    /** deschidem fisierul pentru citirea datelor **/
    int fd = open(argv[i], O_RDONLY, 0777);
    char buf[maxl];
    int n;
    /** cat timp mai putem citi din fisier **/
    while((n = read(fd, buf, maxl)) > 0){
        len = 5 + n;
        /** construim mesajul pe baza datelor citite si a numarului de bytes cititi, intors de functia read**/
        replyData(&t, &SEQ, &crc, D, buf, len, n);
        send_message(&t);
        printf("SENDER trimite data cu SEQ: %d\n", SEQ);
        /** apelam functia waitForACK pentru a astepta confirmare si a retransmite mesajul in caz de NACK**/
        int rez = waitForACK(&t, time, &SEQ, &crc, len, D, buf, n);
        if(rez == 0){
            return 0;
        }
        /** apelam functia memset pentru a nu avea caractere ramase din mesajele anterioare in viitorul mesaj**/
        memset(buf, 0, maxl);
        memset(t.payload, 0, t.len);
    }

        /** s-a incheiat transmiterea datelor din fisier, asa ca trimitem un mesaj cu campul data gol si tipul Z, pentru a anunta
        receptorul ca urmeaza un nou fisier **/
        len = 7;
        reply(&t, &SEQ, &crc, Z, NULL, len);
        send_message(&t);
        count_send = 1;
        /** apelam functia waitForACK pentru a astepta confirmare si a retransmite mesajul in caz de NACK**/
        rez = waitForACK(&t, time, &SEQ, &crc, len, Z, NULL, 0);
        if(rez == 0){
            return 0;
        }
        /** inchidem fisierul pe care l-am terminat de transmis **/
        close(fd);
    }

     /** s-a incheiat transmiterea datelor din  toate fisierele, asa ca trimitem un mesaj cu campul data gol si tipul B, pentru a anunta
    receptorul ca transmisia se va incheia **/
    int len = 7;
    reply(&t, &SEQ, &crc, B, NULL, len);
    send_message(&t);
    count_send = 1;
    /** apelam functia waitForACK pentru a astepta confirmare si a retransmite mesajul in caz de NACK**/
    int rez = waitForACK(&t, time, &SEQ, &crc, len, B, NULL, 0);
    if(rez == 0){
        return 0;
    }

    printf("=== Transmision ended ===\n");

    return 0;
}
