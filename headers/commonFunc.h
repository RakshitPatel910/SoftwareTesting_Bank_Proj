#ifndef COMMONFUNC
#define COMMONFUNC

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/sem.h>
#include <string.h>
#include <errno.h>

#include "../structures/users.h"
#include "../structures/transaction.h"

typedef enum {
    LOCK_SHARED = 1,
    LOCK_EXCLUSIVE = 2
} LockType;


int apply_file_lock(int fd, LockType lock_type, size_t struct_size, off_t offset) {
    struct flock lock;
    memset(&lock, 0, sizeof(lock));

    lock.l_type = (lock_type == LOCK_EXCLUSIVE) ? F_WRLCK : F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = offset;
    lock.l_len = struct_size;

    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("Failed to apply lock");
        return -1;
    }

    return 0;
}

int release_file_lock(int fd, size_t struct_size, off_t offset) {
    struct flock lock;
    memset(&lock, 0, sizeof(lock));

    lock.l_type = F_UNLCK; 
    lock.l_whence = SEEK_SET;
    lock.l_start = offset;
    lock.l_len = struct_size;

    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("Failed to release lock");
        return -1;
    }

    return 0;
}

int addTransactionToCustomer( struct Customer* customer, struct Transaction* transaction ){
    int transaction_list_fd = open( "./dataBaseFiles/transaction/transaction.txt", O_RDWR );
    int file_size = lseek( transaction_list_fd, 0, SEEK_END );
    transaction->tID = file_size / sizeof(*transaction);

    close( transaction_list_fd );

    customer->transaction[customer->tp] = transaction->tID;
    customer->tp = (customer->tp + 1) % 15;
    
    int cust_list_fd = open( "./dataBaseFiles/customer/customer.txt", O_RDWR );

    apply_file_lock( cust_list_fd, LOCK_EXCLUSIVE, sizeof(*customer), sizeof(*customer) * customer->acc_no );
    // lock_file( cust_list_fd, LOCK_EXCLUSIVE );

    lseek( cust_list_fd, sizeof(*customer) * customer->acc_no, SEEK_SET );
    write( cust_list_fd, customer, sizeof(*customer) );

    release_file_lock( cust_list_fd, sizeof(*customer), sizeof(*customer) * customer->acc_no );
    // unlock_file( cust_list_fd );

    close( cust_list_fd );

    return 0;
}

int addTransaction(  struct Transaction* transaction ){
    int transaction_list_fd = open( "./dataBaseFiles/transaction/transaction.txt", O_RDWR );
    int file_size = lseek( transaction_list_fd, 0, SEEK_END );
    transaction->tID = file_size / sizeof(*transaction);


    apply_file_lock( transaction_list_fd, LOCK_EXCLUSIVE, sizeof(*transaction), sizeof(*transaction) * transaction->tID );
    // lock_file( transaction_list_fd, LOCK_EXCLUSIVE );

    lseek( transaction_list_fd, sizeof(*transaction) * transaction->tID, SEEK_SET );
    write( transaction_list_fd, transaction, sizeof(*transaction) );

    release_file_lock( transaction_list_fd, sizeof(*transaction), sizeof(*transaction) * transaction->tID );
    // unlock_file( transaction_list_fd );

    close( transaction_list_fd );

    return 0;
}



#endif