#ifndef TIME_WHEEL_TIMER
#define TIME_WHEEL_TIMER

#include <time.h>
#include <netinet/in.h>
#include <stdio.h>

#define BUFFER_SIZE 64

class tw_timer {
public:
    tw_timer(int rot, int ts)
    : next(NULL), prev(NULL), rotation(rot), time_slot(ts){}

public:
    
}