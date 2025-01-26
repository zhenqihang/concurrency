#include "heaptimer.h"
#include <iostream>

void timeoutCallback()
{
    std::cout << "Timer exprired\n";
}

int main()
{
    HeapTimer timer;
    timer.add(1, 5000, timeoutCallback);

    timer.adjust(1, 3000);

    std::this_thread::sleep_for(std::chrono::seconds(4));
    timer.tick();

    return 0;
}