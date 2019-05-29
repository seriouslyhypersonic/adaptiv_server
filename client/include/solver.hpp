/*
 * Copyright (c) Nuno Alves de Sousa 2019
 *
 * Use, modification and distribution is subject to the Boost Software License,
 * Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef SOLVE_H
#define SOLVE_H

#include <iostream>
#include <cstddef>
#include <thread>
#include <chrono>

#include "util.hpp"
#include "json.hpp"

namespace solver
{

using miliseconds_t = std::chrono::milliseconds;
using namespace std::chrono_literals;

class RANS
{

private:
    std::size_t iteration_;
    struct Residuals
    {
        static double constexpr min = 1e-8;
        static double constexpr max = 1;

        struct Momentum
        {
            double x_;
            double y_;
            double z_;
        } momentum_;

        double energy_;
        double tke_;
        double tdr_;
    } residuals_;

    double maxIterations_;
    miliseconds_t iterationTime_;
    bool hasFinished_ = false;

public:
    RANS(
        std::size_t iterations = 32,
        miliseconds_t iterationTime = 500ms)
    : iteration_(0)
    , residuals_{{1, 1, 1}, 1, 1, 1}
    , maxIterations_(iterations)
    , iterationTime_(iterationTime)
    { }

    bool hasFinished() const { return hasFinished_; }

    void update()
    {
        if (hasFinished_) {
            return;
        }

        ++iteration_;
        hasFinished_ = iteration_ == maxIterations_;

        residuals_.momentum_.x_ = util::random(Residuals::min, Residuals::max);
        residuals_.momentum_.y_ = util::random(Residuals::min, Residuals::max);
        residuals_.momentum_.z_ = util::random(Residuals::min, Residuals::max);
        residuals_.energy_ = util::random(Residuals::min, Residuals::max);
        residuals_.tke_ = util::random(Residuals::min, Residuals::max);
        residuals_.tdr_ = util::random(Residuals::min, Residuals::max);

    }

    void reset()
    {
        iteration_ = 0;
        residuals_ = {{1, 1, 1}, 1, 1, 1};
    }

    void toJson(std::ostream& out, bool pretty = true)
    {
        json::ptree results;
        results.put("finished", hasFinished_);
        results.put("iteration", iteration_);
        results.put("residuals.momentum.x", residuals_.momentum_.x_);
        results.put("residuals.momentum.y", residuals_.momentum_.y_);
        results.put("residuals.momentum.z", residuals_.momentum_.z_);
        results.put("residuals.energy", residuals_.energy_);
        results.put("residuals.tke", residuals_.tke_);
        results.put("residuals.tdr", residuals_.tdr_);

        // Simulate solver runtime
        std::this_thread::sleep_for(iterationTime_);

        json::write_json(out, results, pretty);
    }
};

inline void solve()
{
    RANS solver(50, 250ms);
    solver.toJson(std::cout, false);
    while (!solver.hasFinished()) {
        solver.update();
        solver.toJson(std::cout, false);
    }
}

} // namespace solve

#endif //SOLVE_H
