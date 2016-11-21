#ifndef UTIL_H
#define UTIL_H
#include <string>
#include <vector>

using namespace std;

class Util{
private:

public:
    Util();
    ~Util();
    static int trim(string&);
    static vector<string> split(const string& str, const char separator);
    static int printConfig();
    static int writePid(const char* fileName);
    static int writeToFile(const string content, const string file);
    static int printServiceMap();
};
#endif
