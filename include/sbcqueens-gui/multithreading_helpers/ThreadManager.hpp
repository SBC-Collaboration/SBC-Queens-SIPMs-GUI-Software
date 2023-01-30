#ifndef THREADMANAGER_H
#define THREADMANAGER_H
#include <memory>
#pragma once

// C STD includes
// C 3rd party includes
// C++ STD includes
// C++ 3rd party includes
// My includes

namespace SBCQueens {

template<typename Pipes>
class ThreadManager {
 protected:
    Pipes _pipes;

 public:
    explicit ThreadManager(const Pipes& pipes) : _pipes(pipes) {}

    virtual ~ThreadManager() = 0;
    virtual void operator()() = 0;
};

} // namespace SBCQueens

#endif