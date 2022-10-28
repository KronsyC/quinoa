struct Person{
    int age;
    char* name;
};



int main(){
    struct Person me;
    me.age = 16;
    return me.age;
}