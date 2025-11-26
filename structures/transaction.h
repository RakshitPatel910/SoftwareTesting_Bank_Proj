#ifndef TRANSACTION
#define TRANSACTION

#include<time.h>

struct Transaction {
    int tID;
    int custID;
    float amount;
    int transactionType;
    time_t transactionTime;
};

#endif