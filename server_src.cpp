/*
B H A V I K
Bhavik Dhandhalya - 2018H1030118
ME 1st year Computer Science at
Birla Institute of Technology & Science, Pilani
bhavik.bitspilani@gmail.com
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <pthread.h>
#include <queue>
#include <unistd.h>
#include <signal.h>
#include <set>
#include <math.h>
using namespace std;

#define BUFFER_SIZE 512
#define PORT 8882
#define LOCAL_HOST "127.0.0.1"
#define REP(i, N) for(int i = 0; i < N; i++)

/*CREATING GLOBAL VARIABLES*/
struct sockaddr_in server, from;
int sock, foo = 1;
int drop_rate;
unsigned int WINDOW_SIZE;
int BASE;
bool received_packets[12345];
bool sent_ack[12345];
bool all_received;
bool all_sent_ack;
bool should_drop_this[12345];
bool without_drop;
int NUMBER_OF_PACKETS;
int expected_packet;
int CURRENT_RECEIVED_PACKET;
set < int > S;
queue < int > ack_array;

socklen_t slen, recv_len;
typedef struct packet1 {
	int seq_no;
} ACK_PKT;

typedef struct packet2 {
	int seq_no;
	char data[BUFFER_SIZE];
	int packet_size;
} DATA_PKT;

/*GLOBAL STRUCTURE VARIABLES*/
DATA_PKT rcv_pkt;
ACK_PKT ack_pkt;
DATA_PKT received_pkt[1000];
queue < packet2 > packet_Q;
map < int, packet2 > ALL_DATA;

void error(char *msg);
void print(char *msg);
void dont_drop();
void generate_drop_rate();

void* receiving_fun(void* foo) {
	int NN = 0;
	expected_packet = 0;
	BASE = 0;
	while (!all_received) {
		all_received = true;
		fflush(stdout);

		recv_len = recvfrom(sock, &rcv_pkt, BUFFER_SIZE, 0, (struct sockaddr *) &from, &slen);

		while (received_packets[expected_packet] == true && (int)packet_Q.size() <= WINDOW_SIZE) {
				packet_Q.pop();
				BASE++;
				expected_packet++;
		}

		if (received_packets[rcv_pkt.seq_no] == false) {
			if (recv_len < 0) error("ERROR WHILE RECEIVING !!");
			if (should_drop_this[rcv_pkt.seq_no] == false) {
				printf("RECEIVED PACKET %d :ACCEPT :BASE %d\n", rcv_pkt.seq_no, BASE);
				ack_array.push(rcv_pkt.seq_no);
				received_packets[rcv_pkt.seq_no] = true;
				ALL_DATA[rcv_pkt.seq_no] = rcv_pkt;
				packet_Q.push(rcv_pkt);
			} else {
				should_drop_this[rcv_pkt.seq_no] = false;
				printf("RECEIVED PACKET %d :DROP :BASE %d\n", rcv_pkt.seq_no, BASE);
				received_packets[rcv_pkt.seq_no] = false;
			}
		}

		for (int i = 0; i < NUMBER_OF_PACKETS; i++)
			if (received_packets[i] == false)
				all_received = false;
	}
}

void* sending_fun(void* foo) {
	while (!all_sent_ack) {
		all_sent_ack = true;
		while (ack_array.empty() == false) {
			ack_pkt.seq_no = ack_array.front();
			ack_array.pop();
			sent_ack[ack_pkt.seq_no] = true;
			if (sendto(sock, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr *) &from, slen) < 0) error("ERROR WHILE SENDING ACK !!");
			printf("SEND ACK %d\n", ack_pkt.seq_no);
		}
		for (int i = 0; i < NUMBER_OF_PACKETS; i++)
			if (sent_ack[i] == false)
				all_sent_ack = false;
	}
}

int main(int argc, char *argv[]) {
	if (argc != 2) error("ENTER COMMAND-LINE ARGUMENTS !!");
	string s = argv[1];
	without_drop = (s == "0")? true : false;

	/*CREATING SOCKET*/
	slen = sizeof(server);

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) error("ERROR WHILE CREATING SOCKET !!");
	print("SOCKET CREATED !!");

	memset((char *) &server, 0, slen);
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sock, (struct sockaddr *) &server, sizeof(server)) < 0) error("ERROR WHILE BINDING !!");
	print("BINDING SUCCESSFULLY !!");

	/*---------------------------------------EDIT HERE-----------------------------------------*/

	/*RECEIVING WINDOW SIZE*/
	char x[1];
	recv_len = recvfrom(sock, &x, sizeof(int), 0, (struct sockaddr *) &from, &slen);
	if (recv_len < 0) error("ERROR WHILE RECEIVING WINDOW SIZE !!");
	WINDOW_SIZE = (int)x[0];
	printf("received WINDOW_SIZE = %d\n", WINDOW_SIZE);

	/*RECEIVING PACKET SIZE*/
	char nop[BUFFER_SIZE];
	recv_len = recvfrom(sock, &nop, BUFFER_SIZE, 0, (struct sockaddr *) &from, &slen);
	if (recv_len < 0) error("ERROR WHILE RECEIVING NUMBER OF PACKETS !!");
	string tempp = nop;
	NUMBER_OF_PACKETS = stoi(tempp);
	printf("NUMBER OF PACKETS %d\n", NUMBER_OF_PACKETS);

	/*CHECK IF USER WANT TO DROP SOME PACKETS OR NOT*/
	generate_drop_rate();
	if (without_drop == true) dont_drop();

	/*MULTI-THREADING*/
	pthread_t thread_one, thread_two, thread_three;

	pthread_create(&thread_one, NULL, &sending_fun, (void *)foo);
	pthread_create(&thread_two, NULL, &receiving_fun, (void *)foo);

	pthread_join(thread_one, NULL);
	pthread_join(thread_two, NULL);

	/*WRITE ON FILE*/
	FILE *file_write = fopen("out.txt", "w");
	if (file_write == NULL) error("ERROR WHILE OPENING FILE !!");

	int bsize = ALL_DATA.size();
	REP(i, bsize) {
		char *char_data = ALL_DATA[i].data;
		int L = strlen(char_data);
		REP(j, L) {
			fputc(char_data[j], file_write);
		}
	}
	printf("CHECK OUTPUT FILE NOW !!\n\n");
	/*-----------------------------------------------------------------------------------------*/
	
	return 0;
}

void print(char *msg) {
	printf("%s\n", msg);
}

void error(char *msg) {
	printf("%s\n", msg);
	exit(0);
}

void dont_drop() {
	REP(i, NUMBER_OF_PACKETS+5) {
		should_drop_this[i] = false;
	}
}

void generate_drop_rate() {
	double x = drand48();
	while (x < 10) {
	    x *= 10;
	}
	drop_rate = (int)x;
	drop_rate = ceil(( (NUMBER_OF_PACKETS*1.0) * (drop_rate*1.0) )/100.0);
	
	while (S.size() != drop_rate) {
	    int find_x = (rand() * rand()) % (NUMBER_OF_PACKETS);
	    if (find_x < 0) find_x *= -1;
	    S.insert(find_x);
	}

	for (int NNN : S) {
		should_drop_this[NNN] = true;
	}
}