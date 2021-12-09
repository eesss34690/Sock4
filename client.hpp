#ifndef _CLIENT_HPP
#define _CLIENT_HPP
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/thread/mutex.hpp>
#include <memory>
#include <string>
#include <vector>
#include "client_set.hpp"

using namespace std;
using namespace boost::asio;

class client_set;
class client
 : public enable_shared_from_this<client> {
private:
    string host_;
    string port_;
    string file_;
    string session;
    client_set& cs_;

    vector<string> cmd_list;
    int idx;

    enum { max_length = 15000 };
    array<char, max_length> data_;

    boost::asio::ip::tcp::socket socket;
    boost::asio::ip::tcp::resolver resolver;
    boost::asio::io_context& io_context_;

    void do_read();
    void do_write();
public:
    client(boost::asio::io_context& io_context, client_set& cs, string s, string h, string p, string f);
    void start();
    void stop(){boost::asio::post(io_context_, [this]() { socket.close(); });}
};
#endif
