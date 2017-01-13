/*Clock Object Specification File*/
#ifndef CLOCK_H
#define CLOCK_H

#include <iostream>
#include <ctime>

using namespace std;

class Clock
{
private:
    time_t *tptr;
    string timestamp;
    int currenttime;
public:
    //default constructor
    Clock()
    {
        tptr = new time_t;
        *tptr = time(tptr);
        timestamp = asctime(localtime(tptr));
        // dateATOM();
    }
    Clock(double epoch)
    {
        tptr = new time_t;
        *tptr = static_cast<time_t>(epoch);
        timestamp = asctime(localtime(tptr));
    }

    ~Clock()
    {
        delete tptr;
    }

    double updateCurrentTime();

    string getTimestamp();

    //parses Timestamp Into ATOM
    void dateATOM();

};

#endif // CLOCK_H

