#ifndef _SERVER_HPP_
#define _SERVER_HPP_

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <map>
#include <string>
#include <algorithm>
#include "single_conn.hpp"

using namespace std;
using boost::asio::ip::tcp;
class server
{
public:
    server(short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
        sock_(io_context)
    {
        do_accept();
    }

private:
    void do_accept();

    tcp::acceptor acceptor_;
    tcp::socket sock_;
};
#endif
