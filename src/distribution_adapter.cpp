#include "dispatcher.h"


extern map<int, string> m_servers;
using namespace boost;

int main() {
	m_servers[1] = "localhost";

	map<int, string>::iterator iter;
	for(iter = m_servers.begin(); iter != m_servers.end(); iter++) {
		dispatcher disp;
		disp.start(iter->first, iter->second);
	}

	return 0;
}
