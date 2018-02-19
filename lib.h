#ifndef LIB
#define LIB

#define S 'S'
#define F 'F'
#define D 'D'
#define B 'B'
#define Z 'Z'
#define Y 'Y'
#define N 'N'
#define Z 'Z'
#define E 'E'

typedef struct {
    int len;
    char payload[1400];
} msg;


typedef struct
{	//marcheaza inceputul de pachet -> valoare de 0x01
	char soh;
	//numarul de bytes care urmeaza acestui camp (lungime_pachet - 2) - dimensiune folosita
	char len;
	//numar de secventa % 64
	char seq;
	//tipul pachetului
	char type;
	//data
	char* data;
	//suma de control
	unsigned short check;
	//sfarsit de pachet
	char mark;
}miniKermit;

typedef struct 
{	//lungimea maxima a datelor receptionate
	char maxl;
	//durata de timeout pt un pachet (s)
	char time;
	//numar de caractere de padding. Implicit initializat cu 0x00
	char npad;
	//caracterul de padding. NUL si ignorat daca npad e 0x00
	char padc;
	//caracterul folosit in campul mark.Implicit este 0x0D
	char eol;
	//restul campurilor
	char qctl;
	char qbin;
	char chkt;
	char rept;
	char capa;
	char r;
}sendInitMsg;

typedef struct
{	//marcheaza inceputul de pachet -> valoare de 0x01
	char soh;
	//numarul de bytes care urmeaza acestui camp (lungime_pachet - 2) - dimensiune folosita
	char len;
	//numar de secventa % 64
	char seq;
	//tipul pachetului
	char type;
	//data
	sendInitMsg data;
	//suma de control
	unsigned short check;
	//sfarsit de pachet
	char mark;
}miniKermitS;

void init(char* remote, int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);
msg* receive_message_timeout(int timeout); //timeout in milliseconds
unsigned short crc16_ccitt(const void *buf, int len);

#endif
