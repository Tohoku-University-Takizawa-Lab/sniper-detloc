#include <string>
#include <vector>
 
//using std::vector;
//using std::string;
using namespace std;
 
void splitString(vector<string> &v_str,const string &str,const char ch)
{
    string sub;
    string::size_type pos = 0;
    string::size_type old_pos = 0;
    bool flag=true;
     
    while(flag)
    {
        pos=str.find_first_of(ch,pos);
        if(pos == string::npos)
        {
            flag = false;
            pos = str.size();
        }
        sub = str.substr(old_pos,pos-old_pos);  // Disregard the '.'
        v_str.push_back(sub);
        old_pos = ++pos;
    }
}
