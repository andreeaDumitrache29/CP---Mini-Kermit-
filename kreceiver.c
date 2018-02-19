#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

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

/** functie pentru a seta campurile unui mesaj care contine 
ca data un mesaj de tip S **/
void setminiKermitS(miniKermitS* m, char soh, char len, char seq, char type,
    sendInitMsg data, unsigned short check, char mark){
    (*m).soh = soh;
    (*m).len = len;
    (*m).seq = seq;
    (*m).type = type;
    (*m).data = data;
    (*m).check = check;
    (*m).mark = mark;
}

/** functie pentru a seta campurile unui mesaj care contine ca data orice string **/
void setminiKermit(miniKermit* m, char soh, char len, char seq, char type,
    char* data, unsigned short check, char mark){
    (*m).soh = soh;
    (*m).len = len;
    (*m).seq = seq;
    (*m).type = type;
    (*m).data = data;
    (*m).check = check;
    (*m).mark = mark;
}

/** functie care extrage campul check dintr-un mesaj primit de tip S**/
unsigned short extragereCrc(msg *r){
    /** am extras lungimea datelor ca fiind lungimea toatala a mesajul - 9,
    in loc de -7, deoarece apar 2 biti de 0 suplimentari din cauza 
    paddingului structurii **/
    unsigned short data_size = (*r).len - 9;
    unsigned short r_crc;
    /** pentru a alcatui campul check, care este un unsigned short, deci 
    reprezentat pe 2 bytes, procedam astfel: extragem cei 2 bytes din 
    compozitia lui**/
    unsigned char c1 = r->payload[5 + data_size];
    unsigned char c2 = r->payload[6 + data_size];
    /** datele fiind in format little endian, al doilea byte este cel mai semnificativ, deci il shiftam cu 8 pozitii si adunam la acesta
    primul byte, pentru a obtine numarul dorit **/
    r_crc = (c2<<8) + c1;
    return r_crc;
}

/** functie care extrage campul check dintr-un mesaj primit **/
unsigned short extragereCrcF(msg *r){
    /** procedam asemanator ca mai sus, dar aici nu mai apare padding datorita structurii, deci lungimea datelor este
    lungimea toatala a mesajului - 7 **/
    unsigned short data_size = (*r).len - 7;
    unsigned short r_crc;
    unsigned char c2 = (*r).payload[4 + data_size];
    unsigned char c1 = (*r).payload[5 + data_size];
    r_crc = (c1<<8) + c2;
    return r_crc;
}

/** Functie care initializeaza campurile unui mesaj de trimis **/
void setMsgToSend(msg *t, int *SEQ, char type, unsigned short *crc){
    miniKermit k;
    setminiKermit(&k, 0x01, 5, *SEQ, type, NULL, *crc, 0x0D);
    memcpy(t->payload, &k, sizeof(k));
    t->len = 7;
}

int main(int argc, char** argv) {
    msg *r, t;
    int count_send = 1;
    init(HOST, PORT);
    
    /** asteptam sa primim primul mesaj, de tip S **/
    r = receive_message_timeout(5000);
    while(1){
        if (r == NULL) {
            /** am primit un mesaj null, asa ca incrementam contorul si asteptam in continuare**/
            if(count_send <= 3){
                printf("RECV timeout (S) \n");
                count_send ++;
                r = receive_message_timeout(5000);
            }else{
                /** am ajuns la a patra asteptare, deci inchidem conexiunea **/
                return 0;
            }
        }
        else{
            break;
        }
    }

     /**Am primit un measaj de tip S, deci trimitem inapoi un ACK sau un NACK**/
    int time = 0;
    int SEQ = 1; 
    unsigned short crc = crc16_ccitt(r->payload, r->len - 4);
    unsigned short r_crc =  extragereCrc(r);
     /**  initializam campurile structurii pentru mesajul de confirmare **/
    sendInitMsg s;
    setSstruct(&s);   
    miniKermitS m;
    
    /**daca am primit un mesaj eronat trimitem inapoi un mesaj de tip NACK
    campul data devine null si recalculam crc-ul**/
    if(crc != r_crc){
        setMsgToSend(&t, &SEQ, N, &crc);
        send_message(&t);

        /** astept un nou pachet de tip S **/
        count_send = 1;
        while(1){
            msg *y = receive_message_timeout(5000);
            if (y == NULL) {
                /** am intrat in timeout, asteptam din nou de maxim 3 ori 
                un nou mesaj **/
                if(count_send <= 3){
                    send_message(&t);
                    printf("RECV timeout \n");
                    count_send ++;
                }else{
                    return 0;
                }       

            } else {
                count_send = 1;
                /** Calculam numarul de secventa pentru urmatorul mesaj 
                de trimis si noul crc pentru mesajul primit **/
                SEQ = (SEQ + 2) % 64;
                crc = crc16_ccitt(y->payload, y->len - 4);
                unsigned short y_crc = extragereCrc(y);
                printf("RECV (S) trimite NACK cu seq: %d\n", SEQ);
                if(crc == y_crc){
                    /** am primit un mesaj corect, deci retinem numarul 
                    de secventa si timpul transmis de sender **/
                    SEQ = (y->payload[2] + 1) % 64;
                    time = (int) y->payload[5];
                    break;
                }

                /**construim mesajul de transmis de tip NACK**/
                setMsgToSend(&t, &SEQ, N, &crc);
                send_message(&t);
            }          
        }
    }
    /** Extragem timpul trimis de sender **/
    time = (int) r->payload[5];

    /** Construim mesajul de transmis de tip ACK.
    Acesta trebuie sa aibe ca data o structura de tip mesaj S **/
    setminiKermitS(&m, 0x01, sizeof(s) + 5, SEQ, Y, s, crc, 0x0D);
    printf("RECV trimite ACK (S) cu SEQ: %d\n", m.seq);
    memcpy(t.payload, &m, sizeof(m));
    t.len = sizeof(s) + 7;
    send_message(&t);

    /**Asteptam urmatorul mesaj **/
    while(1){
        r = receive_message_timeout(time * 1000);
        if(r == NULL){
            if(count_send <= 3){
                printf("RECV timeout\n");
                count_send ++;
            }else{
                return 0;
            } 
        }else{
            /** Am primit un mesaj nou. Ii verificam corectitudinea si tipul **/
            count_send = 1;
            unsigned short data_size = r->len - 7;
            crc = crc16_ccitt(r->payload, r->len - 3);
            r_crc =  extragereCrcF(r);
            
            int ok = 0; char* file_name;
            int okData = 0; int fd;
            char* buffer; char type = r->payload[3];
            int seq_primit = r->payload[2];

            if(crc != r_crc){
                /** Mesajul prmit a fost eronat.
                Construim mesajul de transmis de tip NACK**/
                SEQ = (SEQ + 2) % 64;
                printf("RECV trimite NACK cu seq: %d\n", SEQ);
                setMsgToSend(&t, &SEQ, N, &crc);
                send_message(&t);

                /** Asteptam sa primim un mesaj corect **/
                int count_send2 = 1;
                while(1){
                    msg *y = receive_message_timeout(5000);
                    if (y == NULL) {
                        if(count_send2 <= 3){
                            printf("RECV timeout \n");
                            count_send2 ++;
                        }else{
                            return 0;
                        }       

                    } else {
                        count_send2 = 1;
                        /** verificam crc-ul noului mesaj **/
                        crc = crc16_ccitt(y->payload, y->len - 3);
                        unsigned short y_crc = extragereCrcF(y);
                        SEQ = (SEQ + 2) % 64;
                        
                        if(crc == y_crc){
                            /** Am primit un mesja corect, asa ca extagem informatiile corecte necesare **/
                            SEQ = (y->payload[2] + 1)%64;
                            data_size = r->len - 7;
                            type = y->payload[3];
                            seq_primit = r->payload[2];
                            
                            if(type == F){
                                /** Am primit un pachet de tip F, construim numele fisierului asociat **/
                                file_name = malloc(5 + data_size);
                                strcpy(file_name, "recv_");
                                strncat(file_name, &y->payload[4], data_size);
                                ok = 1;
                            }else if(type == D){
                                /** Am primit un pachet de tip D, deci copiem datele primite intr-un buffer pentru a fi scrise ulterior
                                in fisier **/
                                buffer = calloc(data_size, sizeof(char));
                                memcpy(buffer, &y->payload[4], data_size);
                                okData = 1;
                            }

                            break;
                        }
                        /**construim mesajul de transmis de tip NACK**/
                        printf("RECV retrimite NACK seq: %d\n", SEQ);
                        setMsgToSend(&t, &SEQ, N, &crc);
                        send_message(&t);
                    }         
                }
            }
            if(ok == 0 && okData == 0){
                /** Am primit un pachet corect din prima, deci retinem direct numarul
                 nou corect de scventa **/
                SEQ = (r->payload[2] + 1) % 64;
            }
            if(type == Z){
                /** Am primit un pachet de tip Z, deci inchidem fisierul curent **/
                close(fd);
            }
            if(type == B){
                /** Am primit un pachet de tip B, deci inchidem transmisiunea **/
                break;
            }
            if(seq_primit < (SEQ - 1) % 64){
                /** verificam sa nu primim mesaje duplicate (pachete cu numar de secvemta mai mic decat numarul de secventa
                curent - 1 (deoarece acesta a fost actualizat la primirea corecta a mesajului)**/
                continue;
            }
            if(type == F && ok == 0){
                /** Daca pachetul a fost primit corect din prima, construit pe baza informatiilor primite**/
                file_name = malloc(5 + data_size);
                strcpy(file_name, "recv_");
                strncat(file_name, &r->payload[4], data_size);
                SEQ = (r->payload[2] + 1) % 64;
            }if(type == F){
                /** deschidem fisierul cu numele corespunzator**/
                fd = open(file_name, O_WRONLY | O_CREAT, 0777);
            }
            else if(type == D){
                /**Copiem datele in buffer, in cazul in care mesajul a fost primit corect din prima**/
                if(okData == 0){
                    buffer = calloc(data_size, sizeof(char));
                    memcpy(buffer, &r->payload[4], data_size);
                    SEQ = (r->payload[2] + 1) % 64;
                }
                /** Scirem datele din buffer in fisier **/
                write(fd, buffer, data_size);
                memset(buffer, 0, data_size);
            }
            printf("RECV trimite ACK pentru (%c) cu seq: %d\n", type, SEQ);
            /**construim mesajul de transmis de tip ACK**/
            setMsgToSend(&t, &SEQ, Y, &crc);
            send_message(&t);
        }
    }
    /** Trimitem ACK pentru pachetul primit **/
    printf("RECV seq ACK final (B): %d\n", SEQ);
    setMsgToSend(&t, &SEQ, Y, &crc);
    send_message(&t);
	return 0;
}
