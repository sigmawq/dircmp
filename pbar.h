//
// Created by swql on 1/14/21.
//

#ifndef DIRCMP_PBAR_H
#define DIRCMP_PBAR_H

#include <iostream>

#define BAR '|'

template<typename CounterType>
class pbar{
    int bars;

    CounterType current = 0;
    CounterType max;

    CounterType step;
    CounterType old = 0;

    bool bar = true;
    bool see_max = true;

    void draw_progress_no_max();
    void draw_progress();
    void draw_bar();
public:
    pbar(CounterType max, CounterType step, int bars) : max(max), step(step), bars(bars)
        {};
    void inc(CounterType inc_by);
    void set_bar(bool val);
    void set_max(bool val);
};

template<typename CounterType>
void pbar<CounterType>::inc(CounterType inc_by) {
    current += inc_by;
    if ((current - old) > step) {
        if (!bar){
            !max ? draw_progress_no_max() : draw_progress();
        }
        else {
            draw_bar();
        }
        old = current;
    }
}

template<typename CounterType>
void pbar<CounterType>::draw_bar() {
    float perecent = (current / (float)(max));
    int bars_to_draw = static_cast<int>(perecent) * bars;

    std::cout << perecent * 100 << " [";
    for (int i = 0; i < bars_to_draw; i++){
        std::cout << BAR;
    }
    std::cout << ']' << std::endl;
}

template<typename CounterType>
void pbar<CounterType>::set_bar(bool val) {
    bar = val;
}

template<typename CounterType>
void pbar<CounterType>::set_max(bool val) {
    max = val;
}

template<typename CounterType>
void pbar<CounterType>::draw_progress() {
    std::cout << current << '/' << max << std::endl;
}

template<typename CounterType>
void pbar<CounterType>::draw_progress_no_max() {
    std::cout << current << std::endl;
}

#endif //DIRCMP_PBAR_H
