#include <iostream>
//App Headers
#include "Core/Api/Session.hpp"
#include "Core/Api/Config.hpp"

int main(int argc, const char * argv[]) {
    Session s;
    s.subscribe();
	s.save_logos();
    s.run_forever();
    
    return 0;
}