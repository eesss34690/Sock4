
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/system/error_code.hpp>
#define BOOST_FILESYSTEM_VERSION 3
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/interprocess/streams/bufferstream.hpp>
#include <set>
#include <map>
#include "server.hpp"
#include "single_conn.hpp"
#include <sys/types.h>
#include <sys/wait.h>
#include <fstream>

using boost::asio::ip::tcp;
using namespace std;

void sock_conn::start(int mode){
    data_.fill('\0');
    if(mode == 1)
        do_connect();
    else if(op == 2)
        do_bind();
    else
        reply_(req, false);
}

void sock_conn::reply_(sock4 sock_, bool ok){
    auto self(shared_from_this());
    string msg;
    msg += '\0';
    if (ok)
        msg += 0x5A;
    else
        msg += 0x5B;
    for(int i=0; i<2; i++)
        msg += req.D_PORT_CHAR[i];
    for(int i=0; i<4; i++)
        msg += req.D_IP_CHAR[i];
    cli_sock->async_send(boost::asio::buffer(msg), [](boost::system::error_code ec, size_t){});
}

void sock_conn::sock_commute(bool ser, bool cli){
    auto self(shared_from_this());
    if(ser){
        ser_sock.async_read_some(boost::asio::buffer(data_, MAXLEN), [this, self](boost::system::error_code ec, size_t length){
            if(!ec){
                cli_sock->async_send(boost::asio::buffer(data_, length), [](boost::system::error_code ec, size_t){});
                data_.fill('\0');
                sock_commute(1, 0);
            }
            if(ec == boost::asio::error::eof){
                cli_sock->close();
            }
        });
    }
    if(cli){
        cli_sock->async_read_some(boost::asio::buffer(data_, MAXLEN), [this, self](boost::system::error_code ec, size_t length) {
            if(!ec){
                ser_sock.async_send(boost::asio::buffer(data_, length), [](boost::system::error_code ec, size_t){});
                data_.fill('\0');
                sock_commute(0, 1);
            }
        });
    }
}

void sock_conn::do_connect(){
    auto self(shared_from_this());
    tcp::endpoint ep(boost::asio::ip::address::from_string(req.D_IP), stoi(req.D_PORT));
    ser_sock.async_connect(ep, [this, self](const boost::system::error_code &ec){
        if(!ec){
            reply_(req, true);
            sock_commute(1, 1);
        }
    });
}

void sock_conn::do_bind(){
    auto self(shared_from_this());
    tcp::acceptor acceptor_(io_context, tcp::endpoint(tcp::v4(), 0));
    acceptor_.listen();
    int b_port = acceptor_.local_endpoint().port();
    req.D_PORT = b_port;
    req.D_PORT_CHAR[0] = b_port/256;
    req.D_PORT_CHAR[1] = b_port%256;
    for(int i=0; i<4; i++)
        req.D_IP_CHAR[i] = 0;
    reply_(req, true);
    acceptor_.accept(ser_sock);
    reply_(req, true);
    sock_commute(1, 1);
}

bool single_conn::firewall()
{
    char send_mode = header.CD == 0x01 ? 'c' : 'b';
    ifstream infile("socks.conf");

    string go, ip;
    char mode;
    while(infile >> go >> mode >> ip){
        if(mode == send_mode){
            stringstream ssip(ip);
            string s;
            auto i = 4, pos = 0, pos2 = 0;
            while(getline(ssip, s, '.'))
            {
                pos2 = (header.D_IP).find(".", pos);
                if ((header.D_IP).substr(pos, pos2 - pos)!= s && s != "*")
                    break;
                pos = pos2 + 1;
                i--;
            }
            if (!i)
                return true;
        }
    }
    return false;
}

void single_conn::do_read()
{
    auto self(shared_from_this());
    socket_.async_read_some(
    boost::asio::buffer(data_, MAXLEN),
        [this, self](boost::system::error_code ec, size_t length) {
            if (!ec) {
                header.VN = data_[0];
                header.CD = data_[1];
                unsigned int port_ = (data_[2] - '\0') * 256 + (data_[3] - '\0');
                for (auto &j: to_string(port_))
                    header.D_PORT += j;
                header.D_PORT_CHAR[0] = data_[2], header.D_PORT_CHAR[1] = data_[3];
                for (int i= 0; i< 4; i++)
                    header.D_IP_CHAR[i] = data_[4 + i];
                
                fourA = true;
                header.D_IP = "";
                for (int i = 4; i< 7; i++)
                {
                    stringstream ss;
                    ss << setfill('0');
                    ss << static_cast<unsigned>(data_[i]);
                    for (auto &j: ss.str())
                        header.D_IP += j;
                    header.D_IP += ".";
                    if (ss.str() != "0")
                    {
                        fourA = false;
                    }
                }
                stringstream ss;
                ss << setfill('0');
                ss << static_cast<unsigned>(data_[7]);
                for (auto &j: ss.str())
                    header.D_IP += j;
                
                if (fourA && (ss.str() == "0"))
                    fourA = false;
                if (fourA)
                {
                    string domain_name = "";
                    auto pos = 8;
                    while(data_[pos++] != 0);
                    while(pos != MAXLEN - 1) domain_name += data_[pos++];
                    tcp::resolver resolv(io_context);
                    auto results = resolv.resolve(domain_name, header.D_PORT);
                    for(auto entry : results) {
                        if(entry.endpoint().address().is_v4()) {
                            header.D_IP = entry.endpoint().address().to_string();
                            stringstream ss(header.D_IP);
                            string s;

                            for(int i=0; i<4; i++){
                                getline(ss, s, '.');
                                stringstream sss;
                                sss << hex << setfill('0');
                                for(int i=0; i< s.length(); ++i)
                                    sss << setw(2) << static_cast<unsigned>(s[i]);
                                header.D_IP_CHAR[i] = sss.str()[0];
                            }
                            break;
                        }
                    }
                }

                data_.fill('\0');
                tcp::endpoint remote_ep = socket_.remote_endpoint();

                cout << "<S_IP>: " << remote_ep.address().to_string() << endl;
                cout << "<S_PORT>: " << remote_ep.port() << endl;
                cout << "<D_IP>: " << header.D_IP << endl;
                cout << "<D_PORT>: " << header.D_PORT << endl;
                cout << "<Command>: ";
                if(header.CD == 0x01)
                    cout << "CONNECT" << endl;
                else if(header.CD == 0x02)
                    cout << "BIND" << endl;
                if (firewall())
                {
                    cout << "<Reply> Accept" << endl << endl;
                    make_shared<sock_conn>(move(socket_), header)->start(int(header.CD));
                }
                else
                {
                    cout << "<Reply> Reject" << endl << endl;
                    make_shared<sock_conn>(move(socket_), header)->start(0);
                }
                do_read();
            }
        });
}

void server::do_accept()
{
    boost::asio::socket_base::reuse_address option(true);
    acceptor_.set_option(option);
    acceptor_.async_accept(
        sock_, [this](boost::system::error_code ec)
        {
            if (!ec)
            {
                io_context.notify_fork(boost::asio::io_context::fork_prepare);
                if(fork() == 0){
                    io_context.notify_fork(boost::asio::io_context::fork_child);
                    make_shared<single_conn>(move(sock_))->start();
                }
                else{
                    io_context.notify_fork(boost::asio::io_context::fork_parent);
                    sock_.close();
                    do_accept();
                }
            }
            do_accept();
    });
}

int main(int argc, char* argv[])
{
  signal(SIGCHLD, SIG_IGN);
  try
  {
    if (argc != 2)
    {
      cerr << "Usage: async_tcp_echo_server <port>\n";
      return 1;
    }
    server s(atoi(argv[1]));
    io_context.run();
  }
  catch (exception& e)
  {
    cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
