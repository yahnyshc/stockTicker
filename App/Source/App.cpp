#include <iostream>
//App Headers
#include "Core/Api/Session.hpp"

int main(int argc, const char * argv[]) {
    auto s = std::make_shared<Session>();
    s->subscribe();
	s->save_logos();
    s->run_forever();
    
    return 0;
}