#ifndef _CLIENT_SET_HPP_
#define _CLIENT_SET_HPP_

#include <memory>
#include <utility>
#include <set>
#include "client.hpp"
#include <iostream>

using namespace std;
class client;
class client_set {
private:
    set<shared_ptr<client>> connections_;
public:
    client_set(const client_set&) = delete;
    client_set& operator=(const client_set&) = delete;

    client_set() {}

    void start(shared_ptr<client> c);
    void stop(shared_ptr<client> c);
    void stop_all();
};

#endif
