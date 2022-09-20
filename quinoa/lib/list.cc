#include "./list.h"
#include<algorithm>
#include<vector>
using namespace std;

template<typename T>
size_t push(vector<T>& items, T item){
    items.push_back(item);
    return items.size();
}

template <typename T>
T pop(std::vector<T>& vec){
    auto item = vec.pop_back();
    return item;
}

template<typename T>
size_t pushf(vector<T>& items, T item){
    items.insert(items.begin(), item);
    return items.size();
}



char popf(string& str){
    if(str.size() == 0)error("Cannot remove character from empty string");

    auto item = str[0];
    str.erase(str.begin());
    return item;
}




