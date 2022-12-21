#include <iomanip>
#include <fstream>
#include <iostream>
using namespace std;

class ostreamFork           // Write same data to two ostreams
{
public:
    ostream& os1;
    ostream& os2;

    ostreamFork(ostream& os_one, ostream& os_two);

    template <class Data>
    friend ostreamFork& operator<<(ostreamFork& osf, Data d)
    {
        osf.os1 << d;
        osf.os2 << d;
        return osf;
    }
    
    // For manipulators: endl, flush
    friend ostreamFork& operator<<(ostreamFork& osf, ostream& (*f)(ostream&))
    {
        osf.os1 << f;
        osf.os2 << f;
        return osf;
    }

    // For setw() , ...
    template<class ManipData>
    friend ostreamFork& operator<<(ostreamFork& osf, ostream& (*f)(ostream&, ManipData))
    {
        osf.os1 << f;
        osf.os2 << f;
        return osf;
    }
};