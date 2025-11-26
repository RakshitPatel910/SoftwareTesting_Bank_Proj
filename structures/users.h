#ifndef USER_STRUCT
#define USER_STRUCT

#include <stdbool.h>

struct User {
    // int acc_no;
    // float balance;
    // bool active;
    int id;
    int access_lvl;

    char username[50];
    char password[50];
};

struct Customer {
    int acc_no;
    float balance;
    bool active;
    int loanID;

    char username[15];
    char password[15];
    int transaction[15];
    int tp;

    char name[40];
    char gender;
    int age;
};

struct Employee {
    int id;
    int role;

    char username[15];
    char password[15];

    int totalLoanAssigned;
    int loanAssigned[15];

    char name[40];
    char gender;
    int age;
};

struct Admin{
    int id;
    char username[15];
    char password[15];
};

#endif