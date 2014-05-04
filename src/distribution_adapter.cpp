#include "dispatcher.h"
#include <boost/bind.hpp>
#include <boost/thread.hpp>

using namespace boost;

extern map<int, string> m_servers;

int main() {
	m_servers[1] = "localhost";

	map<int, string>::iterator iter;
	for(iter = m_servers.begin(); iter != m_servers.end(); iter++) {
		dispatcher disp;
		thread t(bind(&dispatcher::run, &disp, iter->first, iter->second));
	}

	return 0;
}
