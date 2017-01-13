/*Clock Object Implementation File*/

#include "clock.h"
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <string>
#include <iostream>

using namespace std;

string Clock::getTimestamp()
{
    time(tptr);
    timestamp = asctime(localtime(tptr));
    return timestamp.substr(4, 20);
}

double Clock::updateCurrentTime() {
    time(tptr);
    currenttime = *tptr;
    return currenttime;
}

//parses string and converts into long long
void Clock::dateATOM()
{
    /*Timestamp = Wed Feb 13 17:17:11 2013
                  123456789111111111122222
                           012345678901234
    2015-09-12T18:06:00+00:00
    */
    string month = timestamp.substr(4, 3);
    string day = timestamp.substr(8, 2);
    string year = timestamp.substr(20, 4);
    string hour = timestamp.substr(11, 2);
    string minute = timestamp.substr(14, 2);
    string second = timestamp.substr(17, 2);
    if (month == "Jan")
        month = "01";
    else if (month == "Feb")
        month = "02";
    else if (month == "Mar")
        month = "03";
    else if (month == "Apr")
        month = "04";
    else if (month == "May")
        month = "05";
    else if (month == "Jun")
        month = "06";
    else if (month == "Jul")
        month = "07";
    else if (month == "Aug")
        month = "08";
    else if (month == "Sep")
        month = "09";
    else if (month == "Oct")
        month = "10";
    else if (month == "Nov")
        month = "11";
    else
        month = "12";

    string date = year + "-" + month + "-" + day;
    string hours = hour + ":" + minute + ":" + second;

    string str;
    str +=  date
            + "T"
            + hours
            + "+00:00";
    timestamp = str;

    return;
}
