#ifndef MANAGER
#define MANAGER

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
#include "./commonFunc.h"
#include "./const.h"

bool handle_manager( int client_socket, struct User* user );
int actDeactCustomerAccount( int client_socket );
int assignLoanApplication( int client_socket );
int reviewFeedback( int client_socket );
int changePasswordManager( int client_socket, int manager_id );

int actDeactCustomerAccount( int client_socket ){
    int read_bytes, write_bytes;
    char read_buffer[1000], write_buffer[1000], buffer[1000];

    memset(read_buffer, 0, sizeof(read_buffer));
    memset(write_buffer, 0, sizeof(write_buffer));

    write_bytes = send( client_socket, ASK_CUST_ACCNO, sizeof(ASK_CUST_ACCNO), 0 );
    memset(write_buffer, 0, sizeof(write_buffer));

    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
    int cust_acc_no = atoi( read_buffer );

    struct Customer customer;
    int cust_list_fd = open( "./dataBaseFiles/customer/customer.txt", O_RDWR );

    apply_file_lock( cust_list_fd, LOCK_SHARED, sizeof(customer), sizeof(customer) * cust_acc_no );

    lseek( cust_list_fd, sizeof(customer) * cust_acc_no, SEEK_SET );
    read( cust_list_fd, &customer, sizeof(customer) );

    release_file_lock( cust_list_fd, sizeof(customer), sizeof(customer) * cust_acc_no );

    if( customer.active == false ){
        sprintf( buffer, "Customer with acc. no. %d is deactive\nEnter 1 to activate the customer: \n", customer.acc_no );
        strcpy( write_buffer, buffer );
        write_bytes = send( client_socket, write_buffer, sizeof(write_buffer), 0 );
        memset(write_buffer, 0, sizeof(write_buffer));
        memset(buffer, 0, sizeof(buffer));

        read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
        int opt = atoi( read_buffer );

        if( opt == 1 ){
            customer.active = true;
            
            apply_file_lock( cust_list_fd, LOCK_SHARED, sizeof(customer), sizeof(customer) * cust_acc_no );
 
            lseek( cust_list_fd, sizeof(customer) * cust_acc_no, SEEK_SET );
            write( cust_list_fd, &customer, sizeof(customer) );

            release_file_lock( cust_list_fd, sizeof(customer), sizeof(customer) * cust_acc_no );

            sprintf( buffer, "Customer with acc. no. %d is activated.\n", customer.acc_no);
            strcpy( write_buffer, buffer );
            write_bytes = send( client_socket, write_buffer, sizeof(write_buffer), 0 );
            memset(write_buffer, 0, sizeof(write_buffer));
            memset(buffer, 0, sizeof(buffer));

            // read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
        }
    }
    else{
        sprintf( buffer, "Customer with acc. no. %d is active\nEnter 1 to deactivate the customer: \n", customer.acc_no );
        strcpy( write_buffer, buffer );
        write_bytes = send( client_socket, write_buffer, sizeof(write_buffer), 0 );
        memset(write_buffer, 0, sizeof(write_buffer));
        memset(buffer, 0, sizeof(buffer));

        read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
        int opt = atoi( read_buffer );

        if( opt == 1 ){
            customer.active = false;
            
            apply_file_lock( cust_list_fd, LOCK_SHARED, sizeof(customer), sizeof(customer) * cust_acc_no );
 
            lseek( cust_list_fd, sizeof(customer) * cust_acc_no, SEEK_SET );
            write( cust_list_fd, &customer, sizeof(customer) );

            release_file_lock( cust_list_fd, sizeof(customer), sizeof(customer) * cust_acc_no );

            sprintf( buffer, "Customer with acc. no. %d is deactivated.\n", customer.acc_no);
            strcpy( write_buffer, buffer );
            write_bytes = send( client_socket, write_buffer, sizeof(write_buffer), 0 );
            memset(write_buffer, 0, sizeof(write_buffer));
            memset(buffer, 0, sizeof(buffer));

            // read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
        }
    }
    
    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );

    close( cust_list_fd );

    return 0;
}

int assignLoanApplication( int client_socket ){
    int read_bytes, write_bytes;
    char read_buffer[1000], write_buffer[1000], buffer[1000];

    memset(read_buffer, 0, sizeof(read_buffer));
    memset(write_buffer, 0, sizeof(write_buffer));

    int loan_list_fd = open( "./dataBaseFiles/loan/loanList.txt", O_RDWR );
    int file_size = lseek( loan_list_fd, 0, SEEK_END );

    apply_file_lock( loan_list_fd, LOCK_SHARED, file_size, 0 );

    struct Loan loan_instance;
    lseek( loan_list_fd, 0, SEEK_SET );
    while( read( loan_list_fd, &loan_instance, sizeof(loan_instance) ) > 0){
        if( loan_instance.loanStatus == 0 ){
            sprintf( buffer, "Loan ID: %d, Customer ID: %d, Amount: %f\n", loan_instance.loanId, loan_instance.custId, loan_instance.amount );
            strcat( write_buffer, buffer );

            printf( "loan: %d, %s", loan_instance.loanId, buffer );
        }
    }

    release_file_lock( loan_list_fd, file_size, 0 );

    strcat( write_buffer, "\nEnter Loan ID to process: \n" );
    write_bytes = send( client_socket, write_buffer, sizeof(write_buffer), 0 );
    memset(write_buffer, 0, sizeof(write_buffer));

    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
    int loan_id = atoi( read_buffer );

    int emp_list_fd = open( "./dataBaseFiles/employee/employee.txt", O_RDWR );
    file_size = lseek( emp_list_fd, 0, SEEK_END );

    struct Employee emp;
    strcpy( write_buffer, ASK_EMP_ID );
    while( 1 ){
        write_bytes = send( client_socket, write_buffer, strlen(write_buffer), 0 );
        memset(write_buffer, 0, sizeof(write_buffer));

        memset(read_buffer, 0, sizeof(read_buffer));
        read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
        int emp_id = atoi( read_buffer );


        apply_file_lock( emp_list_fd, LOCK_SHARED, sizeof(emp), sizeof(emp) * emp_id );
        
        // printf("1e : %d\n", emp_id);
        lseek( emp_list_fd, sizeof(emp) * emp_id, SEEK_SET );
        read( emp_list_fd, &emp, sizeof(emp) );

        release_file_lock( emp_list_fd, sizeof(emp), sizeof(emp) * emp_id );

        if( emp.role == 1 ){
            strcpy( write_buffer, "Cant assign to a Manager\n" );
            strcat( write_buffer, ASK_EMP_ID );

            continue;
        }

        int isAvailable = 0;
        for( int i = 0; i < 15; i++ ){
            if( emp.loanAssigned[i] == -1 ){
                emp.loanAssigned[i] = loan_id;
                emp.totalLoanAssigned++;
                

                apply_file_lock( emp_list_fd, LOCK_EXCLUSIVE, sizeof(emp), sizeof(emp) * emp_id );
                // printf("2e\n");

                lseek( emp_list_fd, sizeof(emp) * emp_id, SEEK_SET );
                write( emp_list_fd, &emp, sizeof(emp) );

                release_file_lock( emp_list_fd, sizeof(emp), sizeof(emp) * emp_id );


                apply_file_lock( loan_list_fd, LOCK_SHARED, sizeof(loan_instance), sizeof(loan_instance) * loan_id );

                loan_instance.loanStatus = 1;
                lseek( loan_list_fd, sizeof(loan_instance) * loan_id, SEEK_SET );
                write( loan_list_fd, &loan_instance, sizeof(loan_instance) );

                release_file_lock( loan_list_fd, sizeof(loan_instance), sizeof(loan_instance) * loan_id );


                isAvailable = 1;
                break;
            }
        }

        if( isAvailable == 1) break;

        strcpy( write_buffer, "Employee already has 15 loans assigned.\n" );
        strcat( write_buffer, ASK_EMP_ID );
    }

    write_bytes = send( client_socket, LOAN_ASSIGNED, strlen(LOAN_ASSIGNED), 0 );
    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );

    return 0;
}

int reviewFeedback( int client_socket ){
    int read_bytes, write_bytes;
    char read_buffer[1000], write_buffer[1000], buffer[1000];

    memset(read_buffer, 0, sizeof(read_buffer));
    memset(write_buffer, 0, sizeof(write_buffer));

    int feedback_list_fd = open( "./dataBaseFiles/feedback/feedback.txt", O_RDWR );
    int file_size = lseek( feedback_list_fd, 0, SEEK_END );

    apply_file_lock( feedback_list_fd, LOCK_SHARED, file_size, 0 );

    struct Feedback feedback_instance;
    lseek( feedback_list_fd, 0, SEEK_SET );
    while( read( feedback_list_fd, &feedback_instance, sizeof(feedback_instance) ) > 0){
        if( feedback_instance.reviewStatus == 0 ){
            sprintf( buffer, "Feedback ID: %d, Customer ID: %d\nFeedback: %s\n\n", feedback_instance.feedbackId, feedback_instance.custId, feedback_instance.feedback );
            strcat( write_buffer, buffer );
        }
    }

    release_file_lock( feedback_list_fd, file_size, 0 );

    strcat( write_buffer, "\nEnter Feedback ID to be marked as reviewed: \n" );
    write_bytes = send( client_socket, write_buffer, sizeof(write_buffer), 0 );
    memset(write_buffer, 0, sizeof(write_buffer));

    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
    int feedback_id = atoi( read_buffer );

    write_bytes = send( client_socket, FEEDBACK_REVIEW, strlen(FEEDBACK_REVIEW), 0 );
    memset(write_buffer, 0, sizeof(write_buffer));

    memset(read_buffer, 0, sizeof(read_buffer));
    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
    int opt = atoi( read_buffer );

    if( opt == 1 ){
        apply_file_lock( feedback_list_fd, LOCK_EXCLUSIVE, sizeof(feedback_instance), sizeof(feedback_instance) * feedback_id );

        lseek( feedback_list_fd, sizeof(feedback_instance) * feedback_id, SEEK_SET );
        read( feedback_list_fd, &feedback_instance, sizeof(feedback_instance) );

        feedback_instance.reviewStatus = 1;

        lseek( feedback_list_fd, sizeof(feedback_instance) * feedback_id, SEEK_SET );
        write( feedback_list_fd, &feedback_instance, sizeof(feedback_instance) );

        release_file_lock( feedback_list_fd, sizeof(feedback_instance), sizeof(feedback_instance) * feedback_id );

        write_bytes = send( client_socket, FDBCK_REV_SUCC, strlen(FDBCK_REV_SUCC), 0 );
    }

    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );

    return 0;

}

int changePasswordManager( int client_socket, int manager_id ){
    char read_buffer[1000], write_buffer[1000];
    int read_bytes, write_bytes;

    memset(read_buffer, 0, sizeof(read_buffer));
    memset(write_buffer, 0, sizeof(write_buffer));

    int emp_list_fd = open( "./dataBaseFiles/employee/employee.txt", O_RDWR );

    write_bytes = send( client_socket, NEW_PASSWORD, strlen(NEW_PASSWORD), 0 );

    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );


    struct Employee manager;
    apply_file_lock( emp_list_fd, LOCK_EXCLUSIVE, sizeof(manager), sizeof(manager) *manager_id );

    lseek( emp_list_fd, sizeof(manager) * manager_id, SEEK_SET );
    read( emp_list_fd, &manager, sizeof(manager) );

    strcpy( manager.password, read_buffer );
    lseek( emp_list_fd, sizeof(manager) * manager_id, SEEK_SET );
    write_bytes = write( emp_list_fd, &manager, sizeof(manager) );

    release_file_lock( emp_list_fd, sizeof(manager), sizeof(manager) *manager_id );


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

bool handle_manager( int client_socket, struct User* manager ){
    char read_buffer[1000], write_buffer[1000];
    int read_bytes, write_bytes;

    memset(read_buffer, 0, sizeof(read_buffer));
    memset(write_buffer, 0, sizeof(write_buffer));

    printf("manager started\n");
    while( 1 ){
        write_bytes = send(client_socket, MANAGER_MENU, strlen(MANAGER_MENU), 0);
        if (write_bytes == -1) {
            perror("Writing manager menu\n");
            return 0;
        }

        memset(read_buffer, 0, sizeof(read_buffer));
        read_bytes = recv(client_socket, &read_buffer, sizeof(read_buffer), 0);

        int opt = atoi( read_buffer );
        // recv( client_socket, &opt, sizeof(opt), 0 );

        printf("opt val %d\n", opt);
        switch ( opt ){
            case 1 :   // Activate/Deactivate Customer Accounts
                actDeactCustomerAccount( client_socket );
                break;
            case 2 :  // Assign Loan Application Processes to Employees
                assignLoanApplication( client_socket );
                break;
            case 3 :  // Review Customer Feedback
                reviewFeedback( client_socket );
                break;
            case 4 :  // Change Password
                changePasswordManager( client_socket, manager->id );
                break;
            case 5 :  // Logout
                strcpy( write_buffer, "#*#logout#*#" );
                send( client_socket, write_buffer, sizeof(write_buffer), 0 );
                memset(write_buffer, 0, sizeof(write_buffer));
                return true;

            case 6 :  // Exit
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