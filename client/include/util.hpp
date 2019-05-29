/*
 * Copyright (c) Nuno Alves de Sousa 2019
 *
 * Use, modification and distribution is subject to the Boost Software License,
 * Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef UTILITY_H
#define UTILITY_H

#include <iostream>
#include <string>
#include <limits>
#include <algorithm>
#include <type_traits>
#include <random>

namespace util
{

inline void help()
{
    std::cout <<
              "List of commands\n" <<
              "   ping  - ping adaptiv server\n"
              "   solve - start solution process\n"
              "   help  - show this menu\n"
              "   quit  - close client\n";
}

inline std::string getCommand()
{
    std::cout << "> ";
    std::string command;
    std::cin >> command;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::transform(command.begin(), command.end(), command.begin(), ::tolower);

    return command;
}

template<class T>
T random(T min, T max)
{
    static_assert(std::is_arithmetic_v<T>, "T must be an integral type");

    // Seed with a real random device
    std::random_device rd;

    std::mt19937 gen(rd());
    if constexpr (std::is_integral_v<T>) {
        std::uniform_int_distribution<T> uniformIntDistribution(min, max);
        return uniformIntDistribution(gen);
    } else {
        std::uniform_real_distribution<T> uniformRealDistribution(min, max);
        return uniformRealDistribution(gen);
    }
}

} // namespace util

#endif //UTILITY_H
