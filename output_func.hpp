#include <iostream>
#include <string> 
using namespace std;

string xml(string origin)
{
    string res;
    for(int i = 0; i < origin.length(); i++){
        char c = origin[i];
        switch(c){
            case '&':   res += "&amp;";     break;
            case '\'':  res += "&apos;";    break;
            case '\"':  res += "&quot;";    break;
            case '<':   res += "&lt;";      break;
            case '>':   res += "&gt;";      break;
            case '\r':                      break;
            case '\n':  res += "&NewLine;"; break;
            default:   res += c;            break;
        }
    }
    return res;
}

void output_shell(string session, string content){
    cout << "<script>document.getElementById(\"";
    cout << session << "\").innerHTML += '";
    cout << xml(content).c_str() << "';</script>\r\n";
}

void output_command(string session, string content){
    cout << "<script>document.getElementById(\"";
    cout << session << "\").innerHTML += '<b>";
    cout << xml(content).c_str() << "</b>';</script>\r\n";
}