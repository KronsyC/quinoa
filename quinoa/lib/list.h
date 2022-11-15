#pragma once

#include "../core/AST/ast.hh"
#include "./error.h"
#include <algorithm>
#include <string>
#include <vector>
template <typename T> std::size_t push(std::vector<T>& vec, T item);

template <typename T> T pop(std::vector<T>& vec);

template <typename T> size_t pushf(std::vector<T>& items, T item)
{
    items.insert(items.begin(), item);
    return items.size();
}
template <typename T> T popf(std::vector<T>& vec)
{
    if(vec.size() == 0)
	error("Cannot remove item from empty vector", true);
    T first = vec[0];
    vec.erase(vec.begin());
    return first;
}
template <typename T> long long indexof(std::vector<T>& items, T& item)
{
    int idx = 0;
    for(auto& i : items) {
	if(i == item)
	    return idx;
	idx++;
    }
    return -1;
}

char popf(std::string& str);

template <typename T> bool includes(std::vector<T>& items, T item)
{
    return find(items.begin(), items.end(), item) != items.end();
}

template <typename T> std::pair<std::vector<T>, std::vector<T>> split(std::vector<T> vec, unsigned int index)
{
    if(index >= vec.size())
	error("Cannot Split a List at an index larger than it's size");
    std::vector<T> v1(vec.begin(), vec.begin() + index);
    std::vector<T> v2(vec.begin() + index + 1, vec.end());
    return { v1, v2 };
}
