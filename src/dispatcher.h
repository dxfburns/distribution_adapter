/*
 * dispatcher.h
 *
 *  Created on: Apr 28, 2014
 *      Author: root
 */

#ifndef DISPATCHER_H_
#define DISPATCHER_H_
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <map>
#include <vector>

#include "model.h"

typedef websocketpp::client<websocketpp::config::asio_client> client;

// pull out the type of messages sent by our config
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

using namespace std;

class dispatcher {
private:
	int machine_id;
	client sip_client;
	void on_open(client*, websocketpp::connection_hdl);
	void on_message(client*, websocketpp::connection_hdl, message_ptr);
	void set_dispatch_package_from_message(const string&, dispatch_package&);
	void set_message_from_dispatch_package(dispatch_package&, string&);
	void set_waiters_map(vector<string>&, map<int, string>&);
public:
	dispatcher();
	void start(int, string);
};




#endif /* DISPATCHER_H_ */
