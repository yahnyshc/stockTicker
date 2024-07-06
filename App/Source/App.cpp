#include <iostream>
//App Headers
#include "Core/Session.hpp"

int main(int argc, const char * argv[]) {
    Session s("ws.cfg");
    s.subscribe();
	s.get_icons();
    s.run_forever();
    
    return 0;
}