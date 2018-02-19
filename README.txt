DUMITRACHE Daniela Andreea
321 CB

TEMA 1 Protocoale de Comunicatii

		==== lib.h ====

	Am definit urmatoarele structuri ajutatoare:

	- miniKermit: contine campurile necesare unui mesaj: soh, len, seq, type, 
data, check si mark; Campul data este reprezentat sub forma de char*, check
sub forma de unsigned short (pentru a avea 2 bytes), iar restul sunt de 
tip char.

	- sendInitMsg: contine campurile necesare unui mesaj care are campul data de
tip S: maxl, time, npad, padc, eol, qctl, qbin, chkt, rept, capa, r; Toate
campurile sunt de tip char.

	- miniKermitS: este identica structurii miniKermit, cu exceptia faptului 
ca data este de tip sendInitMsg, pentru a usura transmiterea pachetului
de tip S.

		==== ksender.c ====

	Pentru trimiterea mesajelor se folosesc urmatoarele functii:
	
	- void setSstruct(sendInitMsg* s) : functie pentru a seta campurile 
unei structuri sendInitMsg, care contine campurile necesare unui pachet de tip S.
	
	- setminiKermitS(miniKermitS* m, char soh, char len, char seq, char type,
sendInitMsg data, char mark): primeste o structura de tip miniKermitS si 
datele necesare si seteaza corespunzator campurile structurii, 
conform datelor primite. Aceasta corespunde unui pachet de tip S,  deoarece
campul data contine o structura de tip sendInitMsg.

	- void setminiKermit(miniKermit* m, char soh, char len, char seq, char
type, char* data, char mark): primeste o structura de tip miniKermit si 
datele necesare si seteaza corespunzator campurile structurii primite, 
conform datelor.

	- void setMessage(msg *t, miniKermit k, int data_len): functie care 
primeste un mesaj si caracteristicile lui si construieste primele campuri 
pentru a putea calcula crc-ul. Campurile check si mark vor fi copiate ulterior
in payload, dupa calculul crc-ului.
	
	- void setMessageCont(msg *t, miniKermit k, int data_len) : functie care
copiaza si restul datelor din structura de tipul miniKermit in payload-ul
mesajului de trimis (crc si mark).

	OBS: am facut copierea membru cu membru folosind memcopy si adresele
corespunzatoare fiecareul camp in cadrul payload-ului, deoarece copierea
cu totul a structurii miniKermit in payoad ducea la transmisia 
necorespunzatoare a datelor.

	- createMsgS(msg *t, int *SEQ, int* crc): functie care construieste un 
mesaj de tip S apeland functiile de mai sus si setand lungimea mesajului.
Tot in cadrul acestei functii se calculeaza si crc-ul mesajului transmis.
Pentru calculul acestuia am luat in considerare si cei 2 biti de padding care
apar la crearea structurii de tip miniKermitS, datorita alinierii datelor.

	- void reply(msg *t, int *SEQ, int* crc, char type, char* data, int len):
asemanatoare functiei createMsgS, dar aceasta functie creaza un mesaj pentru
celelalte pachete, unde campul data este reprezentat print-un char*. Am trata
separat cazurile cand campul data este NULL, pentru a evita erorile.

	- void replyData(msg *t, int *SEQ, int* crc, char type, char* data, int 
len, int n): asemanator functiei reply, aceasta functie creaza mesajul de
transmis pentru pachetele de tip D, deoarece in cadrul acestora trebuie sa
se tina cont de numarul de bytes cititi pentru a putea seta corespunzator
campurile din cadrul structurilor.

	- int waitForACK(msg *t, int time, int *SEQ, int* crc, int len, char type, 
char* data, int n): functie care asteapta primirea unui ACK pentru un mesaj. 
Intoarce 0 in caz de timeout si 1 atunci cand se primeste ACK pentru mesajul
dat. Incepem prin a astepta un mesaj. Daca functia de asteptare intoarce null, 
incrementam un contor, pana ajungem la 3, si retrimitem mesajul. Daca s-a 
trecut de a 3-a trimitere consecutiva fara a primi un raspuns, atunci 
executia programului se incheie(intoarce 0). Altfel, verificam tipul mesajului primit. 
Daca am primit un NACK, atunci recalculam numarul de secventa si crc-ul si trimitem
mesajul din nou, proces care se repeta pana la primirea unui ACK. Cand mesajul
primit este de tip Y, atunci extragem secventa din mesajul primit si adunam 1,
pentru a obtine noul numar de secventa.

	OBS: atat in sender, cat si in receiver, am calculat numerele de secventa
pe baza valorilor anterioare, pana la primirea unui ACK/ mesaj corect, cand
updatatez valorile pe baza celor primite. Am facut acest lucru pentru
a evita ca numarul de secventa sa fie corupt de o transmise necorespunzatoare.

	Functia main incepe prin a construi un mesaj de tip S, care contine datele
ce urmeaza sa fie trimise receptorului, din care acesta sa extraga informatiile
cu privire la emitator. Se trimite acest prim pachet si se asteapta primirea 
unei confirmari. In caz ca se ajunge la timeout, se incrementeaza un contor si 
se retrimite mesajul. Cand se depasesc 3 retransmisii fara vreun raspuns, se
incheie executia programului.
	Dupa ce senderul primeste un raspuns pentru pachtul de tip S, verifica
tipul acestuia: in caz de NACK recalculam numarul de secventa si crc-ul si 
trimitem mesajul din nou, proces care se repeta pana la primirea unui ACK,
moment cand extragem secventa din mesajul primit si adunam 1, pentru a obtine 
noul numar de secventa. Extragem de asemenea informatiile trimise de receptor,
anume dimensiunea maxima a datelor acceptate de acesta si timpul pe care
il asteapta pana la timeout.
	In continuare, pentru fiecare fisier primit ca argument in
linia de comanda: incepem  prin trimiterea pachetului de tip F, construind 
mesajul, avand in campul data numele fisierului, apoi apelam functia
waitForACK si verificam rezultatul: data intoarce 0, inseamna ca s-a ajuns
la timeout si se incheie conexiunea. Altfel, inseamna ca mesajul trimis
a fost receptionat cu succes si continuam transmisia.
	Pentru pachetele de tip D: se deschide fisierul si se citesc date, cel
mult maxl o data. Construim mesajul pe baza datelor citite si apelam din nou
functia waitForACK. La finalul transmiterii unui mesaj, apelam functia 
memset pentru a nu avea caractere ramase din mesajele anterioare in 
viitorul mesaj.
	Pachetul Z se transmite cand se termina de citit datele dintr-un fisier.
Construim un mesaj de tip Z, cu campul data NULL si il trasmitem, apoi 
asteptam rapsunsul folosind functia waitForACK.
	La finalul transmisiei tuturor fisierelor se va trimite pachetul de tip
B, care are campul data NULL si tipul B. Ateptam rapsunsul folosind functia
waitForACK. La primirea confirmarii, transmisia se incheie cu succes.

		==== kreceiver.c ====

	Am folosit urmatoarele functii:

	- void setminiKermitS(miniKermitS* m, char soh, char len, char seq, char 
type, sendInitMsg data, unsigned short check, char mark): la fel ca la sender;

	- void setminiKermit(miniKermit* m, char soh, char len, char seq, char 
type, char* data, unsigned short check, char mark): la fel ca la sender;

	- unsigned short extragereCrc(msg *r): returneaza campul check dintr-un
mesaj de tip S primit. Am extras lungimea datelor ca fiind lungimea toatala a 
mesajul - 9, in loc de -7, deoarece apar 2 biti de 0 suplimentari din cauza 
paddingului structurii miniKermitS. Pentru a alcatui campul check, care este 
un unsigned short, deci reprezentat pe 2 bytes, procedam astfel:
extragem cei 2 bytes din compozitia lui, aflati la un offset de 5 / 6 + 
lungimea datelor de inceputul mesajului, Acest offset este cu 1 mai mare decat
in mod normal, datorita paddingului structurii. Datele fiind in format 
little endian, al doilea byte este cel mai semnificativ, deci il shiftam cu
8 pozitii si adunam la acesta primul byte, pentru a obtine numarul dorit.

	- unsigned short extragereCrcF(msg *r): se procedeaza identic ca mai sus,
cu precizarea ca acum nu mai avem biti in plus, deci campul data
are lungime de len - 7 (suma toatala a campurilor mark ,len etc), iar cei
2 bytes care intra in compozitia crc-ului sunt la un offset de 4, respectiv
5 + dimensiunea datelor de inceputul structurii.

	Functia main: Incepem prin a astepta sosirea unui pachet de tip S. Se
asteapta de maxim 3 ori, fara a trimite nimic inapoi in caz de timeout.
Daca se ajunge la a 4-a asteptare fara a primi nimic, se inchide conexiunea.
	La primirea unui mesaj de tip S, se calculeaza numarul de secventa pentru
mesajul ce urmeaza sa fie transmis si crc-ul pentru mesajul primit. De
asemena, se extrage crc-ul mesajului primit folosind functia extragereCrc.
Daca cele doua valori difera, se trimite un mesaj de tip NACK si se asteapta
din nou primirea unui mesaj corect, fara retransmitere in caz de timeout.
La fiecare mesaj primit se actualizeaza numaru de secventa si se recalculeaza
crc-ul. Dupa primirea unui mesaj corect, se extrag informatiile primite de la
emitator si se trimite un mesaj de tip ACK, care contine in campul data
datele receptorului: maxl si timpul de asteptat pana la timeout.
	In continuare, receptorul asteapta venirea unui nou mesaj. In caz de
timeout, se modifica contorul pana se ajunge la 3 timeouturi consecutive.
Am tinut cont de precizarea ca pe ruta receptor - emitator nu se pierd date,
asa ca nu am retrimis ultimul mesaj in caz de timeout, deoarece acesta sigur
va ajunge oricum la emitator.
	Dupa primirea unui mesaj este verificata corectitudinea si este updatat
numarul de secventa. In caz ca trebuie sa trimitem un NACK, numarul de secventa
se calculeaza pe baza celui anterior, pentru a fi siguri ca nu folosim un
numar de secventa corupt. Trimitem mesajul de NACK si asteptam sosirea unui
nou mesaj, pentru care se repeta procedeul de verificare de timeout si de
verificare a corectitudinii prin comparatia crc-ului calculat cu crc-ul
extras din pachetul primit.
	Atunci cand primim un mesaj corect (fie din prima, fie dupa transmiterea
unui NACK) verificam tipul acestuia si in functie de acesta se efectueaza
diferite operatii:
	- pachet F: extragem numele fisierului din zona de date a mesajului
primit si creem fisierul corespunzator, adaugand prefixul "recv_";
	- pachet D: extragem datele trimise din pachetul primit, le copiem
intr-un bufffer si le scriem in fisierul creat anterior;
	- pachet Z: inchidem fisierul deschis;
	- pachet B: inchidem conexiunea.
	
	Recptorul verifica duplicatele prin comparatia numarului de secventa primit
cu numarul de secventa anterior (inainte sa fie updatat pentru transmiterea
urmatorului mesaj), respingand pachetele cu numar de secventa mai mic
decat cel anterior.
	In urma primirii corecte a oricarui tip de pachet se va trimite un mesaj
de tip ACK, folosind numarul de secventa extras din pachetul primit, care
acum avem siguranta ca este corect.
