#include <iostream>
#include <string>
#include <cstdlib>

#include "util.hpp"
#include "solver.hpp"


int main()
{
    std::cout << "adaptiv client (type help for more info)\n";
    while (true) {
        auto command = util::getCommand();

        if (command == "ping") {

        } else if (command == "solve") {
            solver::solve();
        } else if (command == "help") {
            util::help();
        } else if (command == "quit") {
            break;
        } else {
            std::cerr << "error: invalid command '" << command << "'\n";
        }
    }

    return EXIT_SUCCESS;
}