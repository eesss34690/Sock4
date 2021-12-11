#ifndef _SINGLE_CONN_HPP_
#define _SINGLE_CONN_HPP_
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <map>
#include <string>
using namespace std;
using boost::asio::ip::tcp;

boost::asio::io_context io_context;

struct sock4{
    unsigned char VN;
    unsigned char CD;
    string D_PORT;
    unsigned char D_PORT_CHAR[2];
    string D_IP;
    unsigned char D_IP_CHAR[4];
};

struct SocksReq{
    int VN;
    int CD;
    int S_PORT;
    string S_IP;
    int D_PORT;
    char DSTPORT[2];
    string D_IP;
    char DSTIP[4];
    string DOMAIN_NAME;
};

class single_conn
 : public enable_shared_from_this<single_conn> {
private:
    tcp::socket socket_;
    tcp::socket rsock;
    enum { max_length = 15000 };
    array<uint8_t, max_length> data_;
    sock4 header;
    bool fourA;
    void do_read();
    bool firewall();

public:
    single_conn(tcp::socket socket)
      : socket_(move(socket)), rsock(io_context){};

    void start() { do_read(); };
};

class conn
 : public enable_shared_from_this<conn> {
private:
    shared_ptr<tcp::socket> cli_sock;
    tcp::socket ser_sock;
    sock4 header;
    //SocksReq header;
    enum { max_length = 15000 };
    array<unsigned char, max_length> data_;

    void sock_reply(bool granted);
    void sock_redirect(bool enableser_socket, bool enablecli_socket);
    void do_connect();
    void do_accept();
public:
    conn(tcp::socket sock, sock4 req):
    //conn(tcp::socket sock, SocksReq req):
        cli_sock(make_shared<tcp::socket>(move(sock))),
        ser_sock(io_context),
        header(req){};
    void start(int mode);
};
#endif
