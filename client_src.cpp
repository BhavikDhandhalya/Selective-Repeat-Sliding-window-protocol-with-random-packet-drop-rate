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
#define THREE_SECOND 3.0
#define LOCAL_HOST "127.0.0.1"
#define REP(i, N) for(int i = 0; i < N; i++)

/*CREATING GLOBAL VARIABLES*/
struct sockaddr_in client;
int sock, foo = 1;
bool thread_running;
unsigned int WINDOW_SIZE = 5;
unsigned int NUMBER_OF_PACKETS;
unsigned int packet_number, BASE;
unsigned int expected_packet;
bool sent_packets[12345];
bool received_ack[12345];
bool all_acks_received;
bool all_received;
map < int, vector < char > > HashMap;

socklen_t slen;
typedef struct packet1 {
	int seq_no;
} ACK_PKT;

typedef struct packet2 {
	int seq_no;
	char data[BUFFER_SIZE];
	int packet_size;
} DATA_PKT;

/*GLOBAL STRUCTURE VARIABLES*/
ACK_PKT rcv_ack;
DATA_PKT send_pkt[1000];
queue < packet2 > packet_Q;

void error(char *msg);
void print(char *msg);

void* sending_fun(void* foo) {
	thread_running = true;
	BASE = 0;
	packet_number = 0;
	REP(i, WINDOW_SIZE) {
		DATA_PKT current_packet = send_pkt[packet_number];
		packet_Q.push(current_packet);
		if (sendto(sock, &current_packet, sizeof(current_packet), 0, (struct sockaddr *) &client, slen) == -1) error("ERROR WHILE SENDING PACKET !!");
		sent_packets[packet_number] = true;
		printf("SEND PACKET %d \t BASE %d\n", packet_number, BASE);
	 	packet_number++;
	}

	expected_packet = 0;
	clock_t begin_time = clock();
	while (!all_acks_received) {
		all_acks_received = true;

		if (received_ack[expected_packet] == true) {
			while (received_ack[expected_packet] == true) {
				packet_Q.pop();
				BASE++;
				expected_packet++;
			}

			while (packet_Q.size() < WINDOW_SIZE) {
				DATA_PKT current_packet = send_pkt[packet_number];
				packet_Q.push(current_packet);
				if (sendto(sock, &current_packet, sizeof(current_packet), 0, (struct sockaddr *) &client, slen) == -1) error("ERROR WHILE SENDING PACKET !!");
				sent_packets[packet_number] = true;
				printf("SEND PACKET %d \t BASE %d\n", packet_number, BASE);
				packet_number++;
			}
			begin_time = clock();
		}

		float current_time = float( clock () - begin_time ) / CLOCKS_PER_SEC;
		if (current_time > THREE_SECOND) {
			begin_time = clock();
			DATA_PKT new_packet = send_pkt[expected_packet];
			if (sendto(sock, &new_packet, sizeof(new_packet), 0, (struct sockaddr *) &client, slen) == -1) error("ERROR WHILE SENDING PACKET !!");
			sent_packets[expected_packet] = true;
			printf("TIMEOUT %d\n", expected_packet);
			printf("RE-SEND PACKET %d \t BASE %d\n", expected_packet, BASE);
		}
		REP(i, NUMBER_OF_PACKETS) {
			if (received_ack[i] == false)
				all_acks_received = false;
		}
	}
	printf("ALL PACKETS ARE SENT !!\n");
}

void* receiving_fun(void* foo) {
	while (all_received == false) {
		all_received = true;
		if (recvfrom(sock, &rcv_ack, sizeof(rcv_ack), 0, (struct sockaddr *) &client, &slen) == -1) error("ERROR WHILE RECEIVING DATA !!");	
		received_ack[rcv_ack.seq_no] = true;
		printf("RECEIVED ACK %d\n", rcv_ack.seq_no);
		REP(i, NUMBER_OF_PACKETS) {
			if (received_ack[i] == false)
				all_received = false;
		}
	}
	printf("ALL ACKS ARE RECEIVED !!\n");
}

int main() {
	/*CREATING SOCKET*/
	slen = sizeof(client);

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) error("ERROR WHILE CREATING SOCKET !!");
	print("SOCKET CREATED !!");

	memset((char *) &client, 0, slen);
	client.sin_family = AF_INET;
	client.sin_port = htons(PORT);
	client.sin_addr.s_addr = inet_addr(LOCAL_HOST);

	/*---------------------------------------READING FILE HERE---------------------------------*/
	FILE *file = fopen("in.txt", "r");

	unsigned long long int total_characters = 0;

	if (file == NULL) {
		printf("ERROR WHILE OPENING FILE\n");
		exit(0);
	}

	vector < char > v;

	int c;
	int sz = 0;
	int Q = 0;
	while ((c = fgetc(file)) != EOF) {
		//putchar((char)c);
		total_characters++;
		sz++;
		v.push_back((char)c);
		/*BE CAREFUL*/
		if (sz == BUFFER_SIZE-4) {
			HashMap[Q++] = v;
			sz = 0;
			v.clear();
		}
	}
	if (sz > 0) HashMap[Q++] = v;
	NUMBER_OF_PACKETS = Q;
	printf("TOTAL NUMBER OF PACKETS %d\n", NUMBER_OF_PACKETS);
	/*---------------------------------------CREATING DATA PACKETS--------------------------------*/	

	for (int i = 0; i < Q; i++) {
		send_pkt[i].seq_no = i;
		vector < char > temp = HashMap[i];
		REP(k, temp.size()) send_pkt[i].data[k] = temp[k];
		send_pkt[i].packet_size = temp.size();
	}

	/*---------------------------------------EDIT HERE-----------------------------------------*/

	/*SENDING WINDOW SIZE*/
	char wsize[1];
	wsize[0] = (char)WINDOW_SIZE;
	if (sendto(sock, &wsize, sizeof(WINDOW_SIZE), 0, (struct sockaddr *) &client, slen) == -1) error("ERROR WHILE SENDING WINDOW SIZE !!");
	printf("Window size = %d sent to server !!\n", WINDOW_SIZE);

	/*SENDING TOTAL NUMBER OF PACKETS*/
	string str_int = to_string(NUMBER_OF_PACKETS);
	int NN = str_int.length();
	char nop[NN];
	strcpy(nop, str_int.c_str());
	if (sendto(sock, &nop, NN, 0, (struct sockaddr *) &client, slen) == -1) error("ERROR WHILE SENDING NUMBER OF PACKETS !!");
	printf("TOTAL NUMBER OF PACKETS ARE %d\n", NUMBER_OF_PACKETS);

	/*MULTI-THREADING*/
	pthread_t thread_one, thread_two;

	pthread_create(&thread_one, NULL, &sending_fun, (void *)foo);
	pthread_create(&thread_two, NULL, &receiving_fun, (void *)foo);

	pthread_join(thread_one, NULL);
	pthread_join(thread_two, NULL);

	/*---------------------------------------------------------------------------------------------*/
	
	return 0;
}

void print(char *msg) {
	printf("%s\n", msg);
}

void error(char *msg) {
	printf("%s\n", msg);
	exit(0);
}