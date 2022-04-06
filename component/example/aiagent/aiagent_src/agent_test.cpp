#include "stdio.h"
#include "string.h"
#include <iostream>
#include "agent_test.h"

#include <vector>
#include <list>

using namespace std;

extern "C" void test1(void)
{
    printf("test1 -> lambda test\r\n");
    
    //
    int x = 10;
    auto f = [=]() mutable -> void {
        x = 20;
        printf("%d\r\n", x);
    };
    f();
    printf("%d\r\n", x);

    //
    int y = 10;
    auto g = [&]() mutable -> void {
        y = 20;
        printf("%d\r\n", y);
    };
    g();
    printf("%d\r\n", y);
    
    //
    [](const char *name) {
        printf("Hello, %s!\r\n", name);
    }("Ameba");
}

extern "C" void test2(void)
{
    printf("test2 -> new delete test\r\n");
    
    //
    int **arr = new int*[2];
    for(int i = 0; i < 2; i++) {
        arr[i] = new int[3]{0};
    }
    for(int i = 0; i < 2; i++) {
        for(int j = 0; j < 3; j++) {
            printf("%d ", arr[i][j]);
        }
        printf("\r\n");
    }
    for(int i = 0; i < 2; i++) {
        delete [] arr[i];
    }
    delete [] arr; 
}

extern "C" void test3(void)
{
    printf("test3 -> vector test\r\n");
    
    //
    vector<int> number;
    for(int i = 0; i < 10; i++) {
        number.push_back(i);
    }
    while(!number.empty()) {
        int n = number.back();
        number.pop_back();
        printf("%d\r\n", n);
    }
    
    //
    int number2[] = {10, 20, 30, 40, 50};
    vector<int> v(begin(number2), end(number2));
    for(vector<int>::iterator it = v.begin(); it != v.end(); it++) {
        auto n = *it;
        printf("%d\r\n", n);
    }
    
}

class Account { 
private:
    string id;  
    string name; 
    double balance;

public: 
    Account(string id, string name, double balance);
    string to_string();
};

Account::Account(string id, string name, double balance) {
    this->id = id;
    this->name = name;
    this->balance = balance;
}

string Account::to_string() {
    return string("Account(") + 
           this->id + ", " +
           this->name + ", " +
           std::to_string(this->balance) + ")";
}

extern "C" void test4(void)
{
    printf("test4 -> class and constructor test\r\n");
    
    //
    Account acct = {"123-456-789", "Ameba Pro", 9999};
    string s = acct.to_string();
    int n = s.length();
    char char_array[n + 1];
    strcpy(char_array, s.c_str());
    printf("acct.to_string(): %s\r\n", char_array);
}

extern "C" void test5(string str)
{
    printf("test5 -> cout test\r\n");
    cout << str << endl;
}

extern "C" void test6(void)
{
    printf("test6 -> list test\r\n");
    
    //
    list<int> my_list = {1, 2, 3};
    for (list<int>::iterator it = my_list.begin(); it != my_list.end(); it++) {
        printf("%d ", *it);
    }
    printf("\r\n");
    
    my_list.push_back(4);
    for (list<int>::iterator it = my_list.begin(); it != my_list.end(); it++) {
        printf("%d ", *it);
    }
    printf("\r\n");

    my_list.push_front(0);
    for (list<int>::iterator it = my_list.begin(); it != my_list.end(); it++) {
        printf("%d ", *it);
    }
    printf("\r\n");
    
    my_list.push_front(0);
    for (list<int>::reverse_iterator it = my_list.rbegin(); it != my_list.rend(); it++) {
        printf("%d ", *it);
    }
    printf("\r\n");
}


