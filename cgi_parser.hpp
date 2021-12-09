#include <iostream>
#include <vector>  
#include <string> 
#include <map>

using namespace std;
class cgi_parser{
private:
    string env;
    int num;
    map<string, string> query_big;
    void parser();
public:
    cgi_parser(const char* query);
    int get_num(){return num;}
    string get_attri(string key){return query_big.find(key)->second;}
};