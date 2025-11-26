#ifndef CUSTOMER
#define CUSTOMER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdbool.h>

#include "../structures/users.h"
#include "../structures/loan.h"
#include "../structures/feedback.h"
#include "../structures/transaction.h"
#include "./commonFunc.h"
#include "./const.h"

bool handle_customer( int client_socket, struct User* user, int cust_id );
int viewBalance( int client_socket, int cust_id );
int depositMoney( int client_socket, int cust_id );
int withdrawMoney( int client_socket, int cust_id );
int transferMoney( int client_socket, int cust_id );
int loanApplication( int client_socket, int cust_id );
int addFeedback( int client_socket, int cust_id );
int viewTransaction( int client_socket, int cust_id );
int changePasswordCustomer( int client_socket, int cust_id );

int viewBalance( int client_socket, int cust_id ){
    char read_buffer[1000], write_buffer[1000];
    int read_bytes, write_bytes;

    int cust_list_fd = open( "./dataBaseFiles/customer/customer.txt", O_RDONLY );

    struct Customer customer;
    apply_file_lock( cust_list_fd, LOCK_SHARED, sizeof(customer), sizeof(customer) *cust_id );

    lseek( cust_list_fd, sizeof(customer) * cust_id, SEEK_SET );
    read( cust_list_fd, &customer, sizeof(customer) );

    release_file_lock( cust_list_fd, sizeof(customer), sizeof(customer) *cust_id );
    
    sprintf( write_buffer, "Balance: %f\n\n", customer.balance );
    write_bytes = send( client_socket, write_buffer, sizeof(write_buffer), 0 );

    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );

    close( cust_list_fd );

    return 0;
}

int depositMoney( int client_socket, int cust_id ){
    char read_buffer[1000], write_buffer[1000];
    int read_bytes, write_bytes;

    printf("deposit started\n");

    int cust_list_fd = open( "./dataBaseFiles/customer/customer.txt", O_RDWR );
    
    printf("%d\n", cust_list_fd);

    write_bytes = send( client_socket, DEPOSIT_AMOUNT, strlen(DEPOSIT_AMOUNT), 0 );
    
    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
    float deposit_amount = atof( read_buffer );

    struct Customer customer;
    struct Transaction transaction;
    transaction.tID = -1;
    transaction.custID = cust_id;
    transaction.amount = deposit_amount;
    transaction.transactionType = 1;
    

    apply_file_lock( cust_list_fd, LOCK_EXCLUSIVE, sizeof(customer), sizeof(customer) *cust_id );

    lseek( cust_list_fd, sizeof(customer) * cust_id, SEEK_SET );
    read( cust_list_fd, &customer, sizeof(customer) );

    printf("init %f\n", customer.balance);
    customer.balance += deposit_amount;

    lseek( cust_list_fd, sizeof(customer) * cust_id, SEEK_SET );
    transaction.transactionTime = time( NULL );
    write( cust_list_fd, &customer, sizeof(customer) );

    lseek( cust_list_fd, sizeof(customer) * cust_id, SEEK_SET );
    read( cust_list_fd, &customer, sizeof(customer) );

    printf("after %f\n", customer.balance);

    release_file_lock( cust_list_fd, sizeof(customer), sizeof(customer) *cust_id );
    

    addTransaction( &transaction );
    addTransactionToCustomer( &customer, &transaction );

    sprintf( write_buffer, "Balance after deposit: %f\n\n", customer.balance );
    write_bytes = send( client_socket, write_buffer, sizeof(write_buffer), 0 );

    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );

    close( cust_list_fd );

    return 0;
}

int withdrawMoney( int client_socket, int cust_id ){
    char read_buffer[1000], write_buffer[1000];
    int read_bytes, write_bytes;

    int cust_list_fd = open( "./dataBaseFiles/customer/customer.txt", O_RDWR );


    struct Customer customer;
    apply_file_lock( cust_list_fd, LOCK_SHARED, sizeof(customer), sizeof(customer) *cust_id );

    lseek( cust_list_fd, sizeof(customer) * cust_id, SEEK_SET );
    read( cust_list_fd, &customer, sizeof(customer) );

    release_file_lock( cust_list_fd, sizeof(customer), sizeof(customer) *cust_id );


    write_bytes = send( client_socket, WITHDRAW_AMOUNT, strlen(WITHDRAW_AMOUNT), 0 );
    
    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
    float withdraw_amount = atof( read_buffer );

    if( withdraw_amount > customer.balance ){
        write_bytes = send( client_socket, INSUFFICIENT_BALANCE, strlen(INSUFFICIENT_BALANCE), 0 );

        read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );

        return 0;
    }
    else if( withdraw_amount < 0 ){
        write_bytes = send( client_socket, NEGATIVE_BALANCE, strlen(NEGATIVE_BALANCE), 0 );

        read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );

        return 0;
    }

    struct Transaction transaction;
    transaction.tID = -1;
    transaction.custID = cust_id;
    transaction.amount = withdraw_amount;
    transaction.transactionType = 2;


    apply_file_lock( cust_list_fd, LOCK_EXCLUSIVE, sizeof(customer), sizeof(customer) *cust_id );

    customer.balance -= withdraw_amount;
    lseek( cust_list_fd, sizeof(customer) * cust_id, SEEK_SET );
    transaction.transactionTime = time( NULL );
    write( cust_list_fd, &customer, sizeof(customer) );

    release_file_lock( cust_list_fd, sizeof(customer), sizeof(customer) *cust_id );
    

    addTransaction( &transaction );
    addTransactionToCustomer( &customer, &transaction );

    sprintf( write_buffer, "Balance after withdraw: %f\n\n", customer.balance );
    write_bytes = send( client_socket, write_buffer, sizeof(write_buffer), 0 );

    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );

    close( cust_list_fd );

    return 0;
}

int transferMoney( int client_socket, int cust_id ){
    char read_buffer[1000], write_buffer[1000];
    int read_bytes, write_bytes;

    memset(read_buffer, 0, sizeof(read_buffer));
    memset(write_buffer, 0, sizeof(write_buffer));

    int cust_list_fd = open( "./dataBaseFiles/customer/customer.txt", O_RDWR );


    struct Customer from_customer, to_customer;
    int cust_file_size = lseek( cust_list_fd, 0, SEEK_END );
    apply_file_lock( cust_list_fd, LOCK_SHARED, sizeof(from_customer), sizeof(from_customer) *cust_id );

    lseek( cust_list_fd, sizeof(from_customer) * cust_id, SEEK_SET );
    read( cust_list_fd, &from_customer, sizeof(from_customer) );

    release_file_lock( cust_list_fd, sizeof(from_customer), sizeof(from_customer) *cust_id );


    write_bytes = send( client_socket, WHOM_TO_TRANSFER, strlen(WHOM_TO_TRANSFER), 0 );
    
    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
    int to_cust_id = atoi( read_buffer );

    if( to_cust_id == cust_id ){
        strcpy( write_buffer,"You cannnot enter your own account.\n" );
        send( client_socket, write_buffer, sizeof(write_buffer), 0 );
        memset(write_buffer, 0, sizeof(write_buffer));

        recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
        memset(read_buffer, 0, sizeof(read_buffer));

        return 0;
    }

    if( to_cust_id >= (cust_file_size / sizeof(from_customer)) ){
        strcpy( write_buffer,"Customer with enterd Acc. no. does not exist.\n" );
        send( client_socket, write_buffer, sizeof(write_buffer), 0 );
        memset(write_buffer, 0, sizeof(write_buffer));

        recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
        memset(read_buffer, 0, sizeof(read_buffer));

        return 0;
    }


    apply_file_lock( cust_list_fd, LOCK_SHARED, sizeof(to_customer), sizeof(to_customer) * to_cust_id );

    printf("to cust: %d\n", to_cust_id);

    lseek( cust_list_fd, sizeof(to_customer) * to_cust_id, SEEK_SET );
    read( cust_list_fd, &to_customer, sizeof(to_customer) );

    release_file_lock( cust_list_fd, sizeof(to_customer), sizeof(to_customer) * to_cust_id );


    write_bytes = send( client_socket, TRANSFER_AMOUNT, strlen(TRANSFER_AMOUNT), 0 );
    
    memset(read_buffer, 0, sizeof(read_buffer));
    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
    float transfer_amount = atof( read_buffer );

    if( transfer_amount > from_customer.balance ){
        write_bytes = send( client_socket, INSUFFICIENT_BALANCE, strlen(INSUFFICIENT_BALANCE), 0 );

        read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );

        return 0;
    }
    else if( transfer_amount < 0 ){
        write_bytes = send( client_socket, NEGATIVE_BALANCE, strlen(NEGATIVE_BALANCE), 0 );

        read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );

        return 0;
    }

    struct Transaction from_transaction, to_transaction;
    from_transaction.tID = -1;
    from_transaction.custID = from_customer.acc_no;
    from_transaction.amount = transfer_amount;
    from_transaction.transactionType = 3;

    to_transaction.tID = -1;
    to_transaction.custID = to_customer.acc_no;
    to_transaction.amount = transfer_amount;
    to_transaction.transactionType =4;
    printf( "to transaction acc: %d\n", to_transaction.custID );

    apply_file_lock( cust_list_fd, LOCK_EXCLUSIVE, sizeof(from_customer), sizeof(from_customer) * cust_id );

    from_customer.balance -= transfer_amount;
    lseek( cust_list_fd, sizeof(from_customer) * cust_id, SEEK_SET );
    from_transaction.transactionTime = time( NULL );
    write( cust_list_fd, &from_customer, sizeof(from_customer) );

    release_file_lock( cust_list_fd, sizeof(from_customer), sizeof(from_customer) *cust_id );
    

    apply_file_lock( cust_list_fd, LOCK_EXCLUSIVE, sizeof(to_customer), sizeof(to_customer) * to_cust_id );

    to_customer.balance += transfer_amount;
    lseek( cust_list_fd, sizeof(to_customer) * to_cust_id, SEEK_SET );
    to_transaction.transactionTime = time( NULL );
    write( cust_list_fd, &to_customer, sizeof(to_customer) );

    release_file_lock( cust_list_fd, sizeof(to_customer), sizeof(to_customer) * to_cust_id );


    addTransaction( &from_transaction );
    addTransactionToCustomer( &from_customer, &from_transaction );
    
    addTransaction( &to_transaction );
    addTransactionToCustomer( &to_customer, &to_transaction );

    sprintf( write_buffer, "Balance after transfer: %f\n\n", from_customer.balance );
    write_bytes = send( client_socket, write_buffer, sizeof(write_buffer), 0 );

    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );

    close( cust_list_fd );

    return 0;
}

int loanApplication( int client_socket, int cust_id ){
    char read_buffer[1000], write_buffer[1000];
    int read_bytes, write_bytes;

    memset(read_buffer, 0, sizeof(read_buffer));
    memset(write_buffer, 0, sizeof(write_buffer));

    int cust_list_fd = open( "./dataBaseFiles/customer/customer.txt", O_RDWR );

    struct Customer customer;
    apply_file_lock( cust_list_fd, LOCK_EXCLUSIVE, sizeof(customer), sizeof(customer) * cust_id );

    lseek( cust_list_fd, sizeof(customer) * cust_id, SEEK_SET );
    read( cust_list_fd, &customer, sizeof(customer) );

    release_file_lock( cust_list_fd, sizeof(customer), sizeof(customer) * cust_id );


    if( customer.loanID > -1 ){
        write_bytes = send( client_socket, ALREADY_APPLIED, strlen(ALREADY_APPLIED), 0 );

        read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
        int amount_requested = atof( read_buffer );

        close( cust_id );

        return 0;
    }


    write_bytes = send( client_socket, ASK_LOAN_AMOUNT, strlen(ASK_LOAN_AMOUNT), 0 );

    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
    int amount_requested = atof( read_buffer );

    int loan_list_fd = open( "./dataBaseFiles/loan/loanList.txt", O_RDWR );
    int file_size = lseek( loan_list_fd, 0, SEEK_END );

    struct Loan loan;
    loan.amount = amount_requested;
    loan.custId = cust_id;
    loan.empId = -1;
    loan.loanId = file_size / sizeof(loan);
    loan.loanStatus = 0;


    apply_file_lock( loan_list_fd, LOCK_EXCLUSIVE, sizeof(loan), sizeof(loan) * loan.loanId );

    lseek( loan_list_fd, sizeof(loan) * loan.loanId, SEEK_SET );
    write( loan_list_fd, &loan, sizeof(loan) );

    release_file_lock( loan_list_fd, sizeof(loan), sizeof(loan) * loan.loanId );


    apply_file_lock( cust_list_fd, LOCK_EXCLUSIVE, sizeof(customer), sizeof(customer) * cust_id );

    customer.loanID = loan.loanId;
    lseek( cust_list_fd, sizeof(customer) * cust_id, SEEK_SET );
    write( cust_list_fd, &customer, sizeof(customer) );

    release_file_lock( cust_list_fd, sizeof(customer), sizeof(customer) * cust_id );


    write_bytes = send( client_socket, LOAN_APPL_SUCCESS, strlen(LOAN_APPL_SUCCESS), 0 );

    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );

    close( loan_list_fd );
    close( cust_list_fd );

    return 0;
}

int addFeedback( int client_socket, int cust_id ){
    char read_buffer[1000], write_buffer[1000];
    int read_bytes, write_bytes;

    memset(read_buffer, 0, sizeof(read_buffer));
    memset(write_buffer, 0, sizeof(write_buffer));

    write_bytes = send( client_socket, GIVE_FEEDBACK, strlen(GIVE_FEEDBACK), 0 );

    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
    
    int feedback_list_fd = open( "./dataBaseFiles/feedback/feedback.txt", O_RDWR );
    int file_size = lseek( feedback_list_fd, 0, SEEK_END );

    struct Feedback feedback;
    feedback.reviewStatus = 0;
    feedback.custId = cust_id;
    feedback.feedbackId = file_size / sizeof(feedback);
    strcpy( feedback.feedback, read_buffer );

    apply_file_lock( feedback_list_fd, LOCK_EXCLUSIVE, sizeof(feedback), sizeof(feedback) * feedback.feedbackId );

    lseek( feedback_list_fd, sizeof(feedback) * feedback.feedbackId, SEEK_SET );
    write_bytes = write( feedback_list_fd, &feedback, sizeof(feedback) );

    release_file_lock( feedback_list_fd, sizeof(feedback), sizeof(feedback) * feedback.feedbackId );

    if( write_bytes == -1 ){
        strcpy( write_buffer, "There was some error sending feedback.\n" );
        write_bytes = send( client_socket, write_buffer, sizeof(write_buffer), 0 );

        recv( client_socket, &read_buffer, sizeof(read_buffer), 0 );

        close( feedback_list_fd );
    }

    write_bytes = send( client_socket, FDBCK_ADD_SUCC, strlen(FDBCK_ADD_SUCC), 0 );

    recv( client_socket, &read_buffer, sizeof(read_buffer), 0 );

    close( feedback_list_fd );

    return 0;
}

int viewTransaction( int client_socket, int cust_id ){
    char read_buffer[1000], write_buffer[1000], buffer[100];
    int read_bytes, write_bytes;

    memset(read_buffer, 0, sizeof(read_buffer));
    memset(write_buffer, 0, sizeof(write_buffer));

    strcpy( write_buffer, "Your last 10 transactions are:\n\n" );

    int cust_list_fd = open( "./dataBaseFiles/customer/customer.txt", O_RDONLY );
    int transaction_list_fd = open( "./dataBaseFiles/transaction/transaction.txt", O_RDWR );

    struct Customer customer;
    apply_file_lock( cust_list_fd, LOCK_SHARED, sizeof(customer), sizeof(customer) * cust_id );

    lseek( cust_list_fd, sizeof(customer) * cust_id, SEEK_SET );
    read( cust_list_fd, &customer, sizeof(customer) );

    release_file_lock( cust_list_fd, sizeof(customer), sizeof(customer) * cust_id );
    
    int start = customer.tp;
    struct Transaction transaction;
    for( int i = 0; i < 10; i++ ){
        int ci = ( start - i + 15 ) % 15;

        if( customer.transaction[ci] == -1 ) continue;;
        // if( customer.transaction[ci] == -1 ) break;

        // apply_file_lock( transaction_list_fd, LOCK_SHARED, sizeof(transaction), sizeof(transaction) * customer.transaction[ci] );

        lseek( transaction_list_fd, sizeof(transaction) * (customer.transaction[ci] - 1), SEEK_SET );
        read( transaction_list_fd, &transaction, sizeof(transaction) );

        // release_file_lock( transaction_list_fd, sizeof(transaction), sizeof(transaction) * customer.transaction[ci] );
        
        char trtype[14];
        if( transaction.transactionType == 1 ){
            strcpy( trtype, "DEPOSIT      ");
        }
        else if ( transaction.transactionType == 2 )
        {
            strcpy( trtype, "WITHDRAW     ");
        }
        else if ( transaction.transactionType == 3 )
        {
            strcpy( trtype, "TRANSFER(SEN)");
        }
        else if ( transaction.transactionType == 4 )
        {
            strcpy( trtype, "TRANSFER(REC)");
        }
        else if ( transaction.transactionType == 5 )
        {
            strcpy( trtype, "LOAN REC     ");
        }
        

        char timestr[25];
        struct tm *localTime = localtime(&transaction.transactionTime);
        strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localTime);
        // sprintf( buffer, "tID: %d | %s | amount: %.2f | time: %s\n", transaction.tID, trtype, transaction.amount, timestr );
        snprintf(buffer, sizeof(buffer), "tID: %d | %s | amount: %.2f | time: %s\n", transaction.tID, trtype, transaction.amount, timestr);
        printf("%s",buffer);

        strcat( write_buffer, buffer );
    }

    write_bytes = send( client_socket, write_buffer, sizeof(write_buffer), 0 );

    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );

    close( cust_list_fd );
    close( transaction_list_fd );
    
    return 0;
}

int changePasswordCustomer( int client_socket, int cust_id ){
    char read_buffer[1000], write_buffer[1000];
    int read_bytes, write_bytes;

    memset(read_buffer, 0, sizeof(read_buffer));
    memset(write_buffer, 0, sizeof(write_buffer));

    int cust_list_fd = open( "./dataBaseFiles/customer/customer.txt", O_RDWR );

    write_bytes = send( client_socket, NEW_PASSWORD, strlen(NEW_PASSWORD), 0 );

    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );


    struct Customer customer;
    apply_file_lock( cust_list_fd, LOCK_EXCLUSIVE, sizeof(customer), sizeof(customer) *cust_id );

    lseek( cust_list_fd, sizeof(customer) * cust_id, SEEK_SET );
    read( cust_list_fd, &customer, sizeof(customer) );

    strcpy( customer.password, read_buffer );
    lseek( cust_list_fd, sizeof(customer) * cust_id, SEEK_SET );
    write_bytes = write( cust_list_fd, &customer, sizeof(customer) );

    release_file_lock( cust_list_fd, sizeof(customer), sizeof(customer) *cust_id );

    close( cust_list_fd );

    if( write_bytes == -1 ){
        strcpy( write_buffer, "Some error occured. Password change failed.\n" );
        write_bytes = send( client_socket, write_buffer, sizeof(write_buffer), 0 );
    }
    else{
        strcpy( write_buffer, "Password changed successfully.\n" );
        write_bytes = send( client_socket, write_buffer, sizeof(write_buffer), 0 );
    }

    recv( client_socket, read_buffer, sizeof(read_buffer), 0 );

    return 0;
}

bool handle_customer( int client_socket, struct User* user, int cust_id ){
    char read_buffer[1000], write_buffer[1000];
    int read_bytes, write_bytes;

    memset(read_buffer, 0, sizeof(read_buffer));
    memset(write_buffer, 0, sizeof(write_buffer));

    printf("customer started\n");
    while( 1 ){
        write_bytes = send(client_socket, CUSTOMER_MENU, strlen(CUSTOMER_MENU), 0);
        if (write_bytes == -1) {
            perror("Writing customer menu\n");
            return 0;
        }

        memset(read_buffer, 0, sizeof(read_buffer));
        read_bytes = recv(client_socket, &read_buffer, sizeof(read_buffer), 0);

        int opt = atoi( read_buffer );
        // recv( client_socket, &opt, sizeof(opt), 0 );

        printf("opt val %d\n", opt);
        switch ( opt ){
            case 1 :   // View Account Balance
                viewBalance(client_socket, cust_id);
                break;
            case 2 :  // Deposit Money
                depositMoney(client_socket, cust_id);
                break;
            case 3 :  // Withdraw Money
                withdrawMoney(client_socket, cust_id);
                break;
            case 4 :  // Transfer Funds
                transferMoney(client_socket, cust_id);
                break;
            case 5 :  // Apply for a Loan  
                loanApplication(client_socket, cust_id);
                break;
            case 6 :  // Adding Feedback
                addFeedback(client_socket, cust_id);
                break;
            case 7 :  // View Transaction History
                viewTransaction(client_socket, cust_id);
                break;
            case 8 :  // Change Password
                changePasswordCustomer(client_socket, cust_id);
                break;
            case 9 :  // Logout
                strcpy( write_buffer, "#*#logout#*#" );
                send( client_socket, write_buffer, sizeof(write_buffer), 0 );
                memset(write_buffer, 0, sizeof(write_buffer));
                return true;
            
            case 10 :  // Exit
                strcpy( write_buffer, "#*#exit#*#" );
                send( client_socket, write_buffer, sizeof(write_buffer), 0 );
                memset(write_buffer, 0, sizeof(write_buffer));
                return true;
            
            default :
                send(client_socket, "Invalid choice", sizeof(write_buffer), 0);
                return true;
                break;
            
        }
    
    }

    return false;
}

#endif