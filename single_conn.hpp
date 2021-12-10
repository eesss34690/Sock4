#ifndef _SINGLE_CONN_HPP_
#define _SINGLE_CONN_HPP_
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <map>
#include <string>
using namespace std;
using boost::asio::ip::tcp;

struct sock4{
    unsigned char VN;
    unsigned char CD;
    unsigned char D_PORT;
    unsigned char D_PORT_CHAR[2];
    string D_IP;
    unsigned char D_IP_CHAR[4];
};

class single_conn
 : public enable_shared_from_this<single_conn> {
private:
    tcp::socket socket_;
    enum { max_length = 15000 };
    array<char, max_length> data_;
    sock4 header;
    bool fourA;
    boost::asio::io_context& io_context_;
    void do_read();
    bool firewall();

public:
    single_conn(tcp::socket socket,
             boost::asio::io_context& io_context)
      : socket_(move(socket)),
        io_context_(io_context){};

    void start() { do_read(); };
};

class conn
 : public enable_shared_from_this<conn> {
private:
    shared_ptr<tcp::socket> cli_sock;
    tcp::socket ser_sock;
    boost::asio::io_context& io_context_;
    sock4 header;
    enum { max_length = 15000 };
    array<char, max_length> data_;

    void sock_reply(bool granted);
    void sock_redirect(bool enableser_socket, bool enablecli_socket);
    void do_connect();
    void do_accept();
public:
    conn(boost::asio::io_context& io_context, tcp::socket sock, sock4 req):
        cli_sock(make_shared<tcp::socket>(move(sock))),
        ser_sock(io_context),
        io_context_(io_context),
        header(req){};
    void start(int mode);
};
#endif