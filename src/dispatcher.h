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

typedef websocketpp::client<websocketpp::config::asio_client> client;

// pull out the type of messages sent by our config
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

class dispatcher {
private:
	client sip_client;
public:
	dispatcher();
	void on_open(client*, websocketpp::connection_hdl);
	void on_message(client*, websocketpp::connection_hdl, message_ptr);
};




#endif /* DISPATCHER_H_ */
