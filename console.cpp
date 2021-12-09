#include <iostream>
#include <vector>  
#include <string>  
#include <utility>
#include <stdio.h>  
#include <stdlib.h>
#include <set>
#include <boost/asio.hpp>
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/interprocess/streams/bufferstream.hpp>
#include <boost/thread/mutex.hpp>
#include "cgi_parser.hpp"
#include "client.hpp"
#include "client_set.hpp"
#include "output_func.hpp"

using namespace std;
void client_set::start(std::shared_ptr<client> c)
{
    connections_.insert(c);
    c->start();
}
void client_set::stop(std::shared_ptr<client> c)
{
    connections_.erase(c);
    c->stop();
}
void client_set::stop_all(){
    for (auto &i : connections_) {
        i->stop();
    }
    connections_.clear();
}
client::client(boost::asio::io_context& io_context, client_set& cs, string s, string h, string p, string f)
    : host_(h), port_(p), file_(f), session(s), idx(0), cs_(cs), socket(io_context)\
    , resolver(io_context), io_context_(io_context){}

void client::start()
{
    auto path = boost::filesystem::path("test_case") / file_;
    if (boost::filesystem::is_regular_file(path))
    {
        boost::filesystem::ifstream f(path);
        string line;
        while (getline(f, line))
	{
	    if (line.empty() || line == "\r") {
                break; // end of headers reached
            }
            if (line.back() == '\r') {
                line.resize(line.size()-1);
            }
            cmd_list.push_back(line + "\n");
	}
        f.close();
        boost::asio::ip::tcp::resolver::query query(host_, port_);
        resolver.async_resolve(query, [this](const boost::system::error_code& ec,
            boost::asio::ip::tcp::resolver::iterator it) {
            if (!ec)
                socket.async_connect(it->endpoint(), [this](const boost::system::error_code& ec) {
                    if (!ec)
                                do_read();
                    else
                        output_shell(session, ec.message().c_str());
                });
            else
                output_shell(session, ec.message().c_str());
        });
    }
}

void client::do_read()
{
    auto self(shared_from_this());
    socket.async_read_some(
        boost::asio::buffer(data_, max_length),
        [this, self](const boost::system::error_code& ec, size_t length) {
	        if (!ec) {
                string line = data_.data();
                data_.fill('\0');
                output_shell(session, line);
                if (line.find("% ") != -1)
                    do_write();
                else
                    do_read();
                
            }
	        //else
	        //{
                //output_shell(session, ec.message());
			    //cs_.stop_all();
		        //socket.close();
			    //output_shell(session, to_string(self.use_count()));
	        //}
        });
}

void client::do_write()
{
    auto self(shared_from_this());
    string cmd = cmd_list[idx++];
    
    output_command(session, cmd);
    socket.async_send(
        boost::asio::buffer(cmd, cmd.length()),
        [this, self](boost::system::error_code ec, size_t _) {
            if (!ec) {
                do_read();
            }
		    else
			    output_command(session, ec.message());
        });
    
}

cgi_parser::cgi_parser(const char* query){
    string ans(query);
    env = ans;
    num = 5;
    parser();
}

void cgi_parser::parser()
{
    int start = 0;
    int end = env.find('&');
    int mid;
    while (end != -1) {
        mid = env.find('=', start);
        if (end - mid == 1)
        {
            num = query_big.size() / 3;
            break;
        }
        query_big[env.substr(start, mid - start)] = env.substr(mid + 1, end - mid - 1);
        start = end + 1;
        end = env.find('&', start);
    }
    mid = env.find('=', start);
    if (env.length() - mid != 1)
        query_big[env.substr(start, mid - start)] = env.substr(mid + 1, env.length() - mid - 1);
}

int main ()
{
    cgi_parser query_parse(getenv("QUERY_STRING"));
    cout << "<!DOCTYPE html>\r\n\r\n";
    cout << "<html lang=\"en\">\r\n";
    cout << "<head>\r\n";
    cout << "<meta charset=\"UTF-8\" />\r\n";
    cout << "<title>NP Project 3 Sample Console</title>\r\n";
    cout << "<link\r\n";
    cout << "  rel=\"stylesheet\"\r\n";
    cout << "  href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css\"\r\n";
    cout << "  integrity=\"sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\"\r\n";
    cout << "  crossorigin=\"anonymous\"\r\n";
    cout << "/>\r\n";
    cout << "<link\r\n";
    cout << "  href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\r\n";
    cout << "  rel=\"stylesheet\"\r\n";
    cout << "/>\r\n";
    cout << "<link\r\n";
    cout << "  rel=\"icon\"\r\n";
    cout << "  type=\"image/png\"\r\n";
    cout << "  href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\r\n";
    cout << "/>\r\n";
    cout << "<style>\r\n";
    cout << "  * {\r\n";
    cout << "    font-family: 'Source Code Pro', monospace;\r\n";
    cout << "    font-size: 1rem !important;\r\n";
    cout << "  }\r\n";
    cout << "  body {\r\n";
    cout << "    background-color: #212529;\r\n";
    cout << "  }\r\n";
    cout << "  pre {\r\n";
    cout << "    color: #cccccc;\r\n";
    cout << "  }\r\n";
    cout << "  b {\r\n";
    cout << "    color: #01b468;\r\n";
    cout << "  }\r\n";
    cout << "</style>\r\n";
    cout << "</head>";
    cout << "  <body>\r\n";
    cout << "<table class=\"table table-dark table-bordered\">\r\n";
    cout << "  <thead>\r\n";
    cout << "    <tr>\r\n";
    for (int i = 0; i< query_parse.get_num(); i++)
    {
        cout << "      <th scope=\"col\">";
        string temp = "h" + to_string(i);
        cout << query_parse.get_attri(temp) << ":";
        temp[0] = 'p';
        cout << query_parse.get_attri(temp) << "</th>\r\n";
    }
    //cout << "      <th scope=\"col\">nplinux1.cs.nctu.edu.tw:1234</th>\r\n";
    cout << "    </tr>\r\n";
    cout << "  </thead>\r\n";
    cout << "  <tbody>\r\n";
    cout << "    <tr>\r\n";
    for (int i = 0; i< query_parse.get_num(); i++)
    {
        cout << "      <td><div class=\"scrollable\"><pre class=\"pre-scrollable\" id=\"";
        string temp = "s" + to_string(i);
        cout << temp << "\" class=\"mb-0\"></pre></div></td>\r\n";
    }
    //cout << "      <td><pre id=\"s0\" class=\"mb-0\"></pre></td>\r\n";
    cout << "    </tr>\r\n";
    cout << "  </tbody>\r\n";
    cout << "</table>\r\n";
    cout << "</body>\r\n";
    cout << "</html>\r\n";

    //output_shell("s0", "test");    
    //output_shell("s0", "testtest"); 

    boost::asio::io_context io_context_;
    client_set cs;
    //set<shared_ptr<client> > clients;
    for (int i = 0; i < query_parse.get_num(); i++) {
        auto ptr = make_shared<client>(io_context_, cs,"s" + to_string(i), query_parse.get_attri("h" + to_string(i)),\
		 query_parse.get_attri("p" + to_string(i)), query_parse.get_attri("f" + to_string(i)));
        cs.start(ptr);
        //clients.insert(ptr);
        //ptr->start();
    }
    io_context_.run();


}
