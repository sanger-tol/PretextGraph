
#ifndef UTILS_H_pretextGraph
#define UTILS_H_pretextGraph

#include <queue>
#include <functional>
#include <vector>
#include "Header.h"



template <typename T>
T find_medium(const T* vec, const u32& num)
{
    // std::priority_queue<T> max_heap;
    std::priority_queue<T, std::vector<T>, std::greater<T>> min_heap;


    T avg = 0;

    for (u32 i=0; i<num; ++i)
    {   
        avg += vec[i];
        T elem = vec[i];
        if (  min_heap.size() < num/4 )
        {
            min_heap.push(elem);
        }
        else if (elem > min_heap.top())
        {
            min_heap.pop();
            min_heap.push(elem);
        }

    }
    avg /= num;
    if (min_heap.size() == 0)
    {
        printf("Warining: min_heap is empty!\n");
        return 0;
    }
    else return Max(Max(min_heap.top(), avg), 1);
}



template <typename T>
void apply_noise_filter_vec(T* vec, const u32& num, const T& medium_value, u32 max_threshold=7)
{
    for (u32 i = 0; i < num; i++)
    {   
        vec[i] = Min(vec[i], max_threshold * medium_value);
    }
}

#endif //UTILS_H_pretextGraph