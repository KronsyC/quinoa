#pragma once


#include <vector>
#include <string>
#include "./error.h"
template <typename T>
std::size_t push(std::vector<T> &vec, T item);

template <typename T>
T pop(std::vector<T> &vec);

template <typename T>
std::size_t pushf(std::vector<T> &vec, T item);

template <typename T>
T popf(std::vector<T> &vec){
    if(vec.size() == 0)error("Cannot remove item from empty vector");
    T first = vec[0];
    vec.erase(vec.begin());
    return first;
}

char popf(std::string &str);


template <typename T>
bool includes(std::vector<T> &items, T item){
    return find(items.begin(), items.end(), item) != items.end();
}

