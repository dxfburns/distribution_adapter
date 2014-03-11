/*
 * dispatcher.cpp
 *
 *  Created on: Apr 28, 2014
 *      Author: root
 */
#include <iostream>
#include <boost/thread/thread.hpp>
#include "dispatcher.h"

using namespace std;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

dispatcher::dispatcher() {
	std::string uri = "ws://localhost:6688";

	try {
		// We expect there to be a lot of errors, so suppress them
		sip_client.clear_access_channels(websocketpp::log::alevel::all);
		sip_client.clear_error_channels(websocketpp::log::elevel::all);

		// Initialize ASIO
		sip_client.init_asio();

		// Register our handlers
		sip_client.set_open_handler(bind(&dispatcher::on_open, this, &sip_client, ::_1));
		sip_client.set_message_handler(bind(&dispatcher::on_message, this, &sip_client, ::_1, ::_2));

		websocketpp::lib::error_code ec;
		client::connection_ptr con = sip_client.get_connection(uri, ec);

		// Specify the SIP subprotocol:
		con->add_subprotocol("sip");

		sip_client.connect(con);

		// Start the ASIO io_service run loop
		sip_client.run();

		while (1) {
			boost::this_thread::sleep(boost::posix_time::milliseconds(100));
		}

		std::cout << "done" << std::endl;

	} catch (const std::exception & e) {
		std::cout << e.what() << std::endl;
	} catch (websocketpp::lib::error_code & e) {
		std::cout << e.message() << std::endl;
	} catch (...) {
		std::cout << "other exception" << std::endl;
	}
}

void dispatcher::on_open(client* c, websocketpp::connection_hdl hdl) {
	cout << "connection ready in dispatcher" << endl;
	std::string SIP_msg =
			"OPTIONS sip:carol@chicago.com SIP/2.0\r\nVia: SIP/2.0/WS df7jal23ls0d.invalid;rport;branch=z9hG4bKhjhs8ass877\r\nMax-Forwards: 70\r\nTo: <sip:carol@chicago.com>\r\nFrom: Alice <sip:alice@atlanta.com>;tag=1928301774\r\nCall-ID: a84b4c76e66710\r\nCSeq: 63104 OPTIONS\r\nContact: <sip:alice@pc33.atlanta.com>\r\nAccept: application/sdp\r\nContent-Length: 0\r\n\r\n";
	sip_client.send(hdl, SIP_msg.c_str(), websocketpp::frame::opcode::text);
}

void dispatcher::on_message(client* c, websocketpp::connection_hdl hdl, message_ptr msg) {
	client::connection_ptr con = sip_client.get_con_from_hdl(hdl);
	cout << "Received a message:" << endl;
	cout << "    " << msg->get_payload() << endl;
}

