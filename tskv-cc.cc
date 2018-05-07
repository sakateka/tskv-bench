#include <fstream>
#include <sstream>
#include <iostream>
#include <map>

using namespace std;

int main()
{
    ifstream f("access.log");

    string line;
    map<size_t, size_t> data;
    while (getline(f, line)) {
        stringstream s(line);


        size_t count = 0;
        string token;
        while (getline(s, token, '\t')) {
            count++;
        }
        data[count]++;
        count = 0;
    }
    for (auto& t: data) {
        std::cout << "data[" << t.first  << ", " << t.second << "]\n";
    }
}
