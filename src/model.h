/*
 * model.h
 *
 *  Created on: May 3, 2014
 *      Author: root
 */

#ifndef MODEL_H_
#define MODEL_H_

#include <vector>
#include <queue>
using namespace std;

struct message {
	string text;
	long time;
};

typedef enum message_type {
	waiter_to_client = 1,
	client_to_waiter = 2,
	waiter_connect = 3,
	client_connect = 4,
	dispatcher_connect = 5,
	client_disconnect = 6,
	message_from_waiter = 7,
	message_from_client = 8,
	heartbeat = 9
} msg_t;

struct package {
	string from;
	string to;
	int site_id;
	string session_id;
	msg_t message_type;
	queue<message> messages;
};

struct dispatch_package {
	vector<string> to;
	package msg;
};

typedef struct connection {
	int conn_address;
	int site_id;
	string client_id;
	short machine_id;
	connection() {
		conn_address = 0;
		site_id = 0;
		client_id = "";
		machine_id = 0;
	}
} conn_m;
#endif /* MODEL_H_ */
