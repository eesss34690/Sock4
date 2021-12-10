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

void conn::start(int mode)
{
    data_.fill('\0');
    if(mode == 1)         //Connect
        do_connect();
    else if(mode == 2)    //Bind
        do_accept();
    else
        sock_reply(false);
}

void conn::sock_reply(bool granted)
{
    auto self(shared_from_this());
    string msg;
    msg += '\0';
    msg += (granted ? 0x5A : 0x5B);
    msg += header.D_PORT_CHAR[0] + header.D_PORT_CHAR[1];
    msg += header.D_IP_CHAR[0] + header.D_IP_CHAR[1] + header.D_IP_CHAR[2] + header.D_IP_CHAR[3];
    for (auto &i: msg) cout << hex << i - '\0' << " ";
    cout <<endl;
    cli_sock->async_send(boost::asio::buffer(msg), [this, self](boost::system::error_code ec, size_t _){});
}

void conn::sock_redirect(bool enableser_socket, bool enablecli_socket)
{
    auto self(shared_from_this());
    if(enableser_socket){
        ser_sock.async_read_some(boost::asio::buffer(data_, data_.size()), [this, self](boost::system::error_code ec, size_t length){
            cout << data_[0] << data_[1] << endl;
            if(!ec){
                cli_sock->async_send(boost::asio::buffer(data_, length), [this, self](boost::system::error_code ec, size_t){});
                data_.fill('\0');
                sock_redirect(1, 0);
            }
            if(ec == boost::asio::error::eof){
                cli_sock->close();
            }
        });
    }
    if(enablecli_socket){
        cli_sock->async_read_some(boost::asio::buffer(data_, data_.size()), [this, self](boost::system::error_code ec, size_t length) {
            if(!ec){
                ser_sock.async_send(boost::asio::buffer(data_, length), [this, self](boost::system::error_code ec, size_t){});
                data_.fill('\0');
                sock_redirect(0, 1);
            }
        });
    }
}

void conn::do_connect()
{
    auto self(shared_from_this());
    tcp::endpoint ep(boost::asio::ip::make_address(header.D_IP), stoi(header.D_PORT));
    ser_sock.async_connect(ep, [this, self](const boost::system::error_code &ec){
        if(!ec){
            sock_reply(true);
            sock_redirect(1, 1);
        }
        else
            cerr << "connect failed" << endl;
    });
}

void conn::do_accept()
{
    auto self(shared_from_this());
    cout << "accept\n";
    tcp::acceptor tcp_acceptor(io_context_, tcp::endpoint(tcp::v4(), 0));
    tcp_acceptor.listen();
    int b_port = tcp_acceptor.local_endpoint().port();
    header.D_PORT = b_port;
    header.D_PORT_CHAR[0] = b_port/256;
    header.D_PORT_CHAR[1] = b_port%256;
    for(int i=0; i<4; i++)
        header.D_IP_CHAR[i] = 0;
    sock_reply(true);
    try{
        tcp_acceptor.accept(ser_sock);
        sock_reply(true);
        sock_redirect(1, 1);
    }
    catch (boost::system::error_code ec){
        cout << "failed: " << ec << endl;
    }

}

bool single_conn::firewall()
{
    char send_mode = header.CD == 0x01 ? 'c' : 'b';
    ifstream infile("socks.conf");

    string go, ip;
    char mode;
    while(infile >> go >> mode >> ip){
        if(go == "permit"){
            if(mode == send_mode){
                stringstream ssip(ip);
                string s;
                auto i = 4, pos = 0;
                while(getline(ssip, s, '.'))
                {
                    if ((header.D_IP).substr(pos, (header.D_IP).find(".", pos))!= s && s != "*")
                        break;
                    pos = (header.D_IP).find(".", pos) + 1;
                    i--;
                }
                if (!i)
                    return true;
            }
        }
    }
    return false;

}

void single_conn::do_read()
{
    auto self(shared_from_this());
    socket_.async_read_some(
    boost::asio::buffer(data_, max_length),
        [this, self](boost::system::error_code ec, size_t length) {
            if (!ec) {
                string data(length, '\0');
                for(int i=0; i<length; i++)
                {
                    data[i] = data_[i];
                }
                header.VN = data_[0];
                header.CD = data_[1];
                auto port_ = (data_[2] << 8) | data[3];
                stringstream ss;
                ss <<setfill('0') << static_cast<unsigned>(port_);
                for (auto &j: ss.str())
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
                    cout << header.D_IP << " ";
                    if (ss.str() != "0")
                    {
                        fourA = false;
                    }
                }
                ss.str("");
                ss << setfill('0');
                ss << static_cast<unsigned>(data_[7]);
                for (auto &j: ss.str())
                    header.D_IP += j;
                
                if (fourA && (ss.str() != "0"))
                    fourA = false;
                if (fourA)
                {
                    string domain_name = "";
                    auto pos = 8;
                    while(data[pos++] == 0);
                    while(pos != max_length - 1) domain_name += data[pos++];
                    tcp::resolver resolv(io_context_);
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

                tcp::endpoint remote_ep = socket_.remote_endpoint();
                boost::asio::ip::address remote_ad = remote_ep.address();

                cout << "<S_IP>: " << remote_ad.to_string() << endl;
                cout << "<S_PORT>: " << remote_ep.port() << endl;
                cout << "<D_IP>: " << header.D_IP << endl;
                cout << "<D_PORT>: " << header.D_PORT << endl;
                cout << "<Command>: ";
                if(header.CD == 1)
                    cout << "CONNECT" << endl;
                else if(header.CD == 2)
                    cout << "BIND" << endl;
                if (firewall())
                {
                    cout << "<Reply> Accept" << endl << endl;
                    make_shared<conn>(io_context_, move(socket_), header)->start(int(header.CD));
                }
                else
                {
                    cout << "<Reply> Reject" << endl << endl;
                    make_shared<conn>(io_context_, move(socket_), header)->start(0);
                }
                data_.fill('\0');
                do_read();
            }
            else
                cout << ec.message();
            });
}

void server::do_accept(boost::asio::io_context& io_context)
{
    acceptor_.async_accept(
        [this, &io_context](boost::system::error_code ec, boost::asio::ip::tcp::socket socket)
        {
            if (!ec)
            {
                io_context.notify_fork(boost::asio::io_context::fork_prepare);
                if(fork() == 0){
                    io_context.notify_fork(boost::asio::io_context::fork_child);
                    make_shared<single_conn>(move(socket), io_context)->start();
                }
                else{
                    io_context.notify_fork(boost::asio::io_context::fork_parent);
                    socket.close();
                    do_accept(io_context);
                }
            }
            do_accept(io_context);
    });
}

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: async_tcp_echo_server <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;
    server s(io_context, std::atoi(argv[1]));
    io_context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
