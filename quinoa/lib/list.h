#pragma once
#include<vector>

template <typename T>
std::size_t push(std::vector<T>& vec, T item);

template <typename T>
T pop(std::vector<T>& vec);

template <typename T>
std::size_t pushf(std::vector<T>& vec, T item);

template <typename T>
T popf(std::vector<T>& vec);

