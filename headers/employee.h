#ifndef EMPLOYEE
#define EMPLOYEE

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
#include "../structures/transaction.h"
#include "./commonFunc.h"
#include "./const.h"

#define MAX_CLIENTS 10
#define MAX_MESSAGE_SIZE 256

bool handle_employee( int client_socket, struct User *employee, int emp_id );
int addCustomer( int client_socket );
int modifyCustomer( int client_socket );
int processLoanApplication( int client_socket, int empId );
int viewAssignedLoans( int client_socket, int empId );
int changePasswordManager( int client_socket, int emp_id );

int addCustomer( int client_socket ){
    struct Customer customer;
    int read_bytes, write_bytes;
    char read_buffer[1000], write_buffer[1000];

    memset(read_buffer, 0, sizeof(read_buffer));
    memset(write_buffer, 0, sizeof(write_buffer));
    
    send( client_socket, ASK_NAME, strlen(ASK_NAME), 0 );

    recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
    strcpy( customer.name, read_buffer );
    memset(read_buffer, 0, sizeof(read_buffer));

    send( client_socket, ASK_AGE, strlen(ASK_AGE), 0 );
    while( 1 ){
        // memset(read_buffer, 0, sizeof(read_buffer));
        recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
        customer.age = atoi(read_buffer);
        memset(read_buffer, 0, sizeof(read_buffer));
    
        if( customer.age > 0 && customer.age < 150){
            break;
        }
        else{
            strcpy( write_buffer, "Enter valid age\n" );
            strcat( write_buffer, ASK_AGE );
            send( client_socket, write_buffer, sizeof(write_buffer), 0 );
            memset(write_buffer, 0, sizeof(write_buffer));
        }
    }

    send( client_socket, ASK_GENDER, strlen(ASK_GENDER), 0 );
    while( 1 ){
        // memset(read_buffer, 0, sizeof(read_buffer));
        recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
        // strcpy( customer.gender, read_buffer );
        customer.gender = read_buffer[0];
        memset(read_buffer, 0, sizeof(read_buffer));
    
        if( customer.gender == 'M' || customer.gender == 'F' ){
            break;
        }
        else{
            strcpy( write_buffer, "Enter valid gender\n" );
            strcat( write_buffer, ASK_GENDER );
            send( client_socket, write_buffer, sizeof(write_buffer), 0 );
            memset(write_buffer, 0, sizeof(write_buffer));
        }
    }

    send( client_socket, ASK_CUST_UNAME, strlen(ASK_CUST_UNAME), 0 );

    recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
    strcpy( customer.username, read_buffer );
    memset(read_buffer, 0, sizeof(read_buffer));

    send( client_socket, ASK_CUST_PASS, strlen(ASK_CUST_PASS), 0 );

    recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
    strcpy( customer.password, read_buffer );
    memset(read_buffer, 0, sizeof(read_buffer));

    int total_cust_fd = open( "./dataBaseFiles/customer/totalCust.txt", O_RDWR );
    int cust_list_fd = open( "./dataBaseFiles/customer/customer.txt", O_RDWR );

    int total_cust = 0;
    int file_size = lseek( total_cust_fd, 0, SEEK_END );
    apply_file_lock( total_cust_fd, LOCK_SHARED, file_size, 0 );

    printf("fd %d\n", file_size);
    lseek( total_cust_fd, 0, SEEK_SET );

    if( read( total_cust_fd, &total_cust, sizeof(total_cust) ) <= 0 ){
        release_file_lock( total_cust_fd, file_size, 0 );

        total_cust = 0;

        customer.acc_no = total_cust;
        customer.balance = 0;
        customer.active = true;
        customer.loanID = -1;
        for( int i = 0; i < 15; i++ ){
            customer.transaction[i] = -1;
        }
        customer.tp = 0;

        printf( "%d total_cust 0\n", total_cust );
        apply_file_lock( cust_list_fd, LOCK_EXCLUSIVE, sizeof(customer), sizeof(customer) * customer.acc_no );

        lseek( cust_list_fd, 0, SEEK_SET );
        write( cust_list_fd, &customer, sizeof(customer) );

        release_file_lock( cust_list_fd, sizeof(customer), sizeof(customer) * customer.acc_no  );

        apply_file_lock( total_cust_fd, LOCK_EXCLUSIVE, file_size, 0 );

        total_cust++;
        lseek( total_cust_fd, 0, SEEK_SET );
        write( total_cust_fd, &total_cust, sizeof(total_cust) );

        release_file_lock( total_cust_fd, file_size, 0  );
    }
    else{
        release_file_lock( total_cust_fd, file_size, 0 );

        customer.acc_no = total_cust;

        customer.acc_no = total_cust;
        customer.balance = 0;
        customer.active = true;
        customer.loanID = -1;
        for( int i = 0; i < 15; i++ ){
            customer.transaction[i] = -1;
        }
        customer.tp = 0;

        printf( "%d total_cust 1\n", total_cust );
        apply_file_lock( cust_list_fd, LOCK_EXCLUSIVE, sizeof(customer), sizeof(customer) * customer.acc_no );


        lseek( cust_list_fd, sizeof(customer) * total_cust, SEEK_SET );
        write( cust_list_fd, &customer, sizeof(customer) );
    
        release_file_lock( cust_list_fd, sizeof(customer), sizeof(customer) * customer.acc_no  );

        apply_file_lock( total_cust_fd, LOCK_EXCLUSIVE, file_size, 0 );

        total_cust++;
        lseek( total_cust_fd, 0, SEEK_SET );
        write( total_cust_fd, &total_cust, sizeof(total_cust) );

        release_file_lock( total_cust_fd, file_size, 0  );
    }

    close( total_cust_fd );
    close( cust_list_fd );

    return 0;
}

int modifyCustomer( int client_socket ){
    struct Customer customer;
    int read_bytes, write_bytes;
    char read_buffer[1000], write_buffer[1000];

    memset(read_buffer, 0, sizeof(read_buffer));
    memset(write_buffer, 0, sizeof(write_buffer));
    
    send( client_socket, ASK_CUST_ACCNO, strlen(ASK_CUST_ACCNO), 0 );
    
    recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
    int cust_id = atoi(read_buffer);
    memset(read_buffer, 0, sizeof(read_buffer));

    
    int cust_list_fd = open( "./dataBaseFiles/customer/customer.txt", O_RDWR );


    apply_file_lock( cust_list_fd, LOCK_SHARED, sizeof(customer), sizeof(customer) *cust_id );

    lseek( cust_list_fd, sizeof(customer) * cust_id, SEEK_SET );
    read( cust_list_fd, &customer, sizeof(customer) );

    release_file_lock( cust_list_fd, sizeof(customer), sizeof(customer) *cust_id );


    sprintf( write_buffer, "Acc. No: %d\nName: %s\nAge: %d\nGender: %c\n\n", customer.acc_no, customer.name, customer.age, customer.gender );
    strcat( write_buffer, ASK_NAME );

    send( client_socket, write_buffer, sizeof(write_buffer), 0 );
    // send( client_socket, ASK_NAME, strlen(ASK_NAME), 0 );

    recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
    strcpy( customer.name, read_buffer );
    memset(read_buffer, 0, sizeof(read_buffer));

    send( client_socket, ASK_AGE, strlen(ASK_AGE), 0 );
    while( 1 ){
        // memset(read_buffer, 0, sizeof(read_buffer));
        recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
        customer.age = atoi(read_buffer);
        memset(read_buffer, 0, sizeof(read_buffer));
    
        if( customer.age > 0 && customer.age < 150){
            break;
        }
        else{
            strcpy( write_buffer, "Enter valid age\n" );
            strcat( write_buffer, ASK_AGE );
            send( client_socket, write_buffer, sizeof(write_buffer), 0 );
            memset(write_buffer, 0, sizeof(write_buffer));
        }
    }

    send( client_socket, ASK_GENDER, strlen(ASK_GENDER), 0 );
    while( 1 ){
        // memset(read_buffer, 0, sizeof(read_buffer));
        recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
        // strcpy( customer.gender, read_buffer );
        customer.gender = read_buffer[0];
        memset(read_buffer, 0, sizeof(read_buffer));
    
        if( customer.gender == 'M' || customer.gender == 'F' || customer.gender == 'O' ){
            break;
        }
        else{
            strcpy( write_buffer, "Enter valid gender\n" );
            strcat( write_buffer, ASK_GENDER );
            send( client_socket, write_buffer, sizeof(write_buffer), 0 );
            memset(write_buffer, 0, sizeof(write_buffer));
        }
    }


    apply_file_lock( cust_list_fd, LOCK_EXCLUSIVE, sizeof(customer), sizeof(customer) *cust_id );

    lseek( cust_list_fd, sizeof(customer) * cust_id, SEEK_SET );
    write( cust_list_fd, &customer, sizeof(customer) );

    release_file_lock( cust_list_fd, sizeof(customer), sizeof(customer) *cust_id );

    close( cust_list_fd );

    return 0;
};

int processLoanApplication( int client_socket, int empId ){
    struct Employee emp;
    int read_bytes, write_bytes;
    char read_buffer[1000], write_buffer[1000], buffer[1000];

    memset(read_buffer, 0, sizeof(read_buffer));
    memset(write_buffer, 0, sizeof(write_buffer));

    int emp_list_fd = open( "./dataBaseFiles/employee/employee.txt", O_RDWR );

    apply_file_lock( emp_list_fd, LOCK_SHARED, sizeof(emp), sizeof(emp) * empId );

    lseek( emp_list_fd, sizeof(emp) * empId, SEEK_SET );
    read( emp_list_fd, &emp, sizeof(emp) );

    release_file_lock( emp_list_fd, sizeof(emp), sizeof(emp) * empId );
    
    write_bytes = send( client_socket, ASK_LOAN_ID, sizeof(ASK_LOAN_ID), 0 );
    memset(write_buffer, 0, sizeof(write_buffer));

    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
    int loan_id = atoi( read_buffer );

    write_bytes = send( client_socket, ASK_LOAN_APPR, strlen(ASK_LOAN_APPR), 0 );
    memset(write_buffer, 0, sizeof(write_buffer));
    
    memset(read_buffer, 0, sizeof(read_buffer));
    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
    int loan_status = atoi( read_buffer );
    

    int cust_list_fd = open( "./dataBaseFiles/customer/customer.txt", O_RDWR );
    int loan_list_fd = open( "./dataBaseFiles/loan/loanList.txt", O_RDWR );

    struct Loan loan_details;
    apply_file_lock( loan_list_fd, LOCK_EXCLUSIVE, sizeof(loan_details), sizeof(loan_details) * loan_id );

    lseek( loan_list_fd, sizeof(loan_details) * loan_id, SEEK_SET );
    read( loan_list_fd, &loan_details, sizeof(loan_details) );

    loan_details.loanStatus = loan_status;

    lseek( loan_list_fd, sizeof(loan_details) * loan_id, SEEK_SET );
    write( loan_list_fd, &loan_details, sizeof(loan_details) );

    release_file_lock( loan_list_fd, sizeof(loan_details), sizeof(loan_details) * loan_id );

    struct Customer customer;
    apply_file_lock( cust_list_fd, LOCK_EXCLUSIVE, sizeof(customer), sizeof(customer) * loan_details.custId );

    lseek( cust_list_fd, sizeof(customer) * loan_details.custId, SEEK_SET );
    read( cust_list_fd, &customer, sizeof(customer) );

    customer.loanID = -1;
    if( loan_status == 1 ){
        customer.balance += loan_details.amount;

        struct Transaction transaction;
        transaction.tID = -1;
        transaction.custID = loan_details.custId;
        transaction.amount = loan_details.amount;
        transaction.transactionType = 5;
        transaction.transactionTime = time(NULL);

        addTransaction( &transaction );
        addTransactionToCustomer( &customer, &transaction );
    }

    lseek( cust_list_fd, sizeof(customer) * loan_details.custId, SEEK_SET );
    write( cust_list_fd, &customer, sizeof(customer) );

    release_file_lock( cust_list_fd, sizeof(customer), sizeof(customer) * loan_details.custId );

    for( int i = 0; i < 15; i++ ){
        if( emp.loanAssigned[i] == loan_id ){
            printf("found\n");
            emp.loanAssigned[i] = -1;

            apply_file_lock( emp_list_fd, LOCK_EXCLUSIVE, sizeof(emp), sizeof(emp) * empId );

            lseek( emp_list_fd, sizeof(emp) * empId, SEEK_SET );
            write( emp_list_fd, &emp, sizeof(emp) );

            release_file_lock( emp_list_fd, sizeof(emp), sizeof(emp) * empId );

            break;
        }
    }

    close( emp_list_fd );
    close( loan_list_fd );
    close( cust_list_fd );

    return 0;
}

int viewAssignedLoans( int client_socket, int empId ){
    struct Employee emp;
    int read_bytes, write_bytes;
    char read_buffer[1000], write_buffer[1000], buff[5];

    memset(read_buffer, 0, sizeof(read_buffer));
    memset(write_buffer, 0, sizeof(write_buffer));
    memset(buff, 0, sizeof(buff));

    int emp_list_fd = open( "./dataBaseFiles/employee/employee.txt", O_RDWR );

    apply_file_lock( emp_list_fd, LOCK_SHARED, sizeof(emp), sizeof(emp) * empId );

    lseek( emp_list_fd, sizeof(emp) * empId, SEEK_SET );
    read( emp_list_fd, &emp, sizeof(emp) );

    release_file_lock( emp_list_fd, sizeof(emp), sizeof(emp) * empId );

    sprintf( buff, "%d", emp.loanAssigned[0] );
    strcpy( write_buffer, buff );
    for( int i = 1; i < 15; i++ ){
        sprintf( buff, "%d", emp.loanAssigned[i] );
        strcat( write_buffer, ", " );
        strcat( write_buffer, buff );
    }

    write_bytes = send( client_socket, write_buffer, sizeof(write_buffer), 0 );

    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );

    return 0;
}

int changePasswordEmployee( int client_socket, int emp_id ){
    char read_buffer[1000], write_buffer[1000];
    int read_bytes, write_bytes;

    memset(read_buffer, 0, sizeof(read_buffer));
    memset(write_buffer, 0, sizeof(write_buffer));

    int emp_list_fd = open( "./dataBaseFiles/employee/employee.txt", O_RDWR );

    write_bytes = send( client_socket, NEW_PASSWORD, strlen(NEW_PASSWORD), 0 );

    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );


    struct Employee manager;
    apply_file_lock( emp_list_fd, LOCK_EXCLUSIVE, sizeof(manager), sizeof(manager) *emp_id );

    lseek( emp_list_fd, sizeof(manager) * emp_id, SEEK_SET );
    read( emp_list_fd, &manager, sizeof(manager) );

    strcpy( manager.password, read_buffer );
    lseek( emp_list_fd, sizeof(manager) * emp_id, SEEK_SET );
    write_bytes = write( emp_list_fd, &manager, sizeof(manager) );

    release_file_lock( emp_list_fd, sizeof(manager), sizeof(manager) *emp_id );


    close( emp_list_fd );

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

bool handle_employee( int client_socket, struct User *employee, int emp_id ){
    char read_buffer[1000], write_buffer[1000];
    int read_bytes, write_bytes;

    memset(read_buffer, 0, sizeof(read_buffer));
    memset(write_buffer, 0, sizeof(write_buffer));

    printf("employee started\n");
    while( 1 ){
        write_bytes = send(client_socket, EMPLOYEE_MENU, strlen(EMPLOYEE_MENU), 0);
        if (write_bytes == -1) {
            perror("Writing employee menu\n");
            return 0;
        }

        memset(read_buffer, 0, sizeof(read_buffer));
        read_bytes = recv(client_socket, &read_buffer, sizeof(read_buffer), 0);

        int opt = atoi( read_buffer );

        printf("opt val %d\n", opt);
        switch ( opt ){
            case 1 :   // Add New Bank Employee
                addCustomer( client_socket );
                break;
            case 2 :   // Modify Customer Details
                modifyCustomer( client_socket );
                break;
            case 3 : // Process Loan Applications
                processLoanApplication( client_socket, emp_id );
                break;
            case 4 : // View Assigned Loan Applications
                viewAssignedLoans( client_socket, emp_id );
                break;
            case 5 :  // Change Password
                changePasswordEmployee( client_socket, emp_id );
                break;
            case 6 :  // Logout
                strcpy( write_buffer, "#*#logout#*#" );
                send( client_socket, write_buffer, sizeof(write_buffer), 0 );
                memset(write_buffer, 0, sizeof(write_buffer));
                return true;
            
            case 7 :  // Exit
                strcpy( write_buffer, "#*#exit#*#" );
                send( client_socket, write_buffer, sizeof(write_buffer), 0 );
                memset(write_buffer, 0, sizeof(write_buffer));
                return true;
            
            default :
                send(client_socket, "Invalid choice", MAX_MESSAGE_SIZE, 0);
                return true;
                break;
            
        }
    
    }

    return false;
}

#endif
