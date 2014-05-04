/*
 * dispatcher.cpp
 *
 *  Created on: Apr 28, 2014
 *      Author: root
 */
#include <iostream>
#include <boost/thread/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <redis/redisclient.h>
#include <boost/format.hpp>
#include "dispatcher.h"

using namespace std;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

using namespace boost;
using namespace boost::property_tree;

map<int, string> m_servers;
map<int, client*> m_clients;
map<int, websocketpp::connection_hdl> m_conn;

void dispatcher::run(int machine_id, string host) {
	this->machine_id = machine_id;

	boost::format fmt("ws://%1%:6690");
	fmt % host;
	string uri = fmt.str();

	try {
		// We expect there to be a lot of errors, so suppress them
		sip_client.clear_access_channels(websocketpp::log::alevel::all);
		sip_client.clear_error_channels(websocketpp::log::elevel::all);

		// Initialize ASIO
		sip_client.init_asio();

		// Register our handlers
		sip_client.set_open_handler(bind(&dispatcher::on_open, this, ::_1));
		sip_client.set_message_handler(bind(&dispatcher::on_message, this, ::_1, ::_2));

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

void dispatcher::on_open(websocketpp::connection_hdl hdl) {
	cout << "connection ready in dispatcher" << endl;
	m_conn[this->machine_id] = hdl;
	m_clients[this->machine_id] = &sip_client;

	websocketpp::frame::opcode::value ev = websocketpp::frame::opcode::text;
	const string msg =
			"{\"to\":\"dispatcher@1\",\"msg\":{\"from\":\"dispatcher@1\",\"to\":\"\",\"site_id\":\"\",\"session_id\":\"\",\"message_type\":\"5\",\"messages\":[]}}";
	cout << "send on open message : " << endl;
	cout << msg << endl;
	sip_client.send(hdl, msg, ev);
}

void dispatcher::on_message(websocketpp::connection_hdl hdl, message_ptr msg) {
	client::connection_ptr con = sip_client.get_con_from_hdl(hdl);
	cout << "Received a message:" << endl;
	cout << "    " << msg->get_payload() << endl;

	websocketpp::frame::opcode::value ev = websocketpp::frame::opcode::text;

	dispatch_package dis_pack;
	set_dispatch_package_from_message(msg->get_payload(), dis_pack);
	map<int, string> m_waiters;
	set_waiters_map(dis_pack.to, m_waiters);

	map<int, string>::iterator iter;
	for (iter = m_waiters.begin(); iter != m_waiters.end(); iter++) {
		string str_to = iter->second;
		vector<string> v_to;
		boost::split(v_to, str_to, boost::is_any_of(";"));
		dis_pack.to = v_to;

		string str_msg;
		set_message_from_dispatch_package(dis_pack, str_msg);

		if (m_clients.count(iter->first) > 0 && m_conn.count(iter->first) > 0) {
			m_clients[iter->first]->send(m_conn[iter->first], str_msg, ev);
		}
	}
}

void dispatcher::set_dispatch_package_from_message(const string& msg, dispatch_package& dis_pack) {
	ptree pt;
	stringstream stream;

	stream << msg;
	read_json(stream, pt);

	string str_to = pt.get<string>("to");
	vector<string> v_to;
	boost::split(v_to, str_to, boost::is_any_of(";"));

	package pack;
	pack.from = pt.get<string>("msg.from");
	pack.to = pt.get<string>("msg.to");
	pack.site_id = atoi(pt.get<string>("msg.site_id").c_str());
	pack.session_id = atoi(pt.get<string>("msg.session_id").c_str());
	pack.message_type = (msg_t) (atoi(pt.get<string>("msg.message_type").c_str()));
	ptree pt1 = pt.get_child("msg.messages");
	queue<message> q;
	for (ptree::iterator iter = pt1.begin(); iter != pt1.end(); iter++) {
		ptree pt2 = iter->second;
		message m;
		m.text = pt2.get<string>("text");
		m.time = atol(pt2.get<string>("time").c_str());

		q.push(m);
	}
	pack.messages = q;

	dis_pack.to = v_to;
	dis_pack.msg = pack;
}

void dispatcher::set_message_from_dispatch_package(dispatch_package& dis_pack, string& msg) {
	ptree pt, pt1, pt2;
	string str_to;
	for (vector<string>::iterator iter = dis_pack.to.begin(); iter != dis_pack.to.end(); iter++) {
		str_to += *iter;
		str_to += ";";
	}

	if (str_to.size() == 0) {
		return;
	}
	str_to = str_to.substr(0, str_to.size() - 1);

	package& pack = dis_pack.msg;
	pt1.put("from", pack.from);
	pt1.put("to", pack.to);
	pt1.put<int>("site_id", pack.site_id);
	pt1.put("session_id", pack.session_id);
	pt1.put<short>("message_type", (short) pack.message_type);
	while (!pack.messages.empty()) {
		message msg = pack.messages.front();
		ptree pt3;
		pt3.put("text", msg.text);
		pt3.put<long>("time", msg.time);

		pt2.push_back(std::make_pair("", pt3));

		pack.messages.pop();
	}
	pt1.put_child("messages", pt2);

	pt.put("to", str_to);
	pt.put_child("msg", pt1);

	ostringstream os;
	write_json(os, pt);

	msg = os.str();
}

void dispatcher::set_waiters_map(vector<string>& v_to, map<int, string>& m_machine) {
	auto_ptr<redis::client> rc(new redis::client("localhost"));
	rc->select(1);

	redis::client::string_vector values;
	vector<string>::iterator iter;
	for (iter = v_to.begin(); iter != v_to.end(); iter++) {
		string key = *iter;
		if (rc->exists(key)) {
			rc->hvals(key, values);
			for (size_t i = 0; i < values.size(); i++) {
				int id = atoi(values[i].c_str());
				if (m_machine.count(id) > 0) {
					key = (m_machine[id] + ";" + key);
				}
				m_machine[id] = key;
			}
		}
	}
}

