#ifndef ADMIN
#define ADMIN

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
#include "./commonFunc.h"
#include "./const.h"

#define MAX_CLIENTS 10
#define MAX_MESSAGE_SIZE 256


bool handle_admin( int client_socket, struct User *admin );
int addEmployee( int client_socket );
int modifyCustEmp( int client_socket );
int manage_user_role( int client_socket );
int changePasswordAdmin( int client_socket, int admin_id );

int addEmployee( int client_socket ){
    printf("op 1\n");
    struct Employee emp;

    char read_buffer[1000], write_buffer[1000];  //, buffer[1000]
    int read_bytes, write_bytes;
    
    memset(read_buffer, 0, sizeof(read_buffer));
    memset(write_buffer, 0, sizeof(write_buffer));

    send( client_socket, ASK_NAME, strlen(ASK_NAME), 0 );

    recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
    strcpy( emp.name, read_buffer );
    memset(read_buffer, 0, sizeof(read_buffer));

    send( client_socket, ASK_AGE, strlen(ASK_AGE), 0 );
    while( 1 ){
        // memset(read_buffer, 0, sizeof(read_buffer));
        recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
        emp.age = atoi(read_buffer);
        memset(read_buffer, 0, sizeof(read_buffer));
    
        if( emp.age > 0 && emp.age < 150){
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
        emp.gender = read_buffer[0];
        memset(read_buffer, 0, sizeof(read_buffer));
    
        if( emp.gender == 'M' || emp.gender == 'F' || emp.gender == 'O' ){
            break;
        }
        else{
            strcpy( write_buffer, "Enter valid gender\n" );
            strcat( write_buffer, ASK_GENDER );
            send( client_socket, write_buffer, sizeof(write_buffer), 0 );
            memset(write_buffer, 0, sizeof(write_buffer));
        }
    }


    write_bytes = send(client_socket, USERNAME, strlen(USERNAME), 0);

    read_bytes = recv(client_socket, &read_buffer, sizeof(read_buffer), 0);
    strcpy(emp.username, read_buffer);
    memset(read_buffer, 0, sizeof(read_buffer));

    write_bytes = send(client_socket, ROLE, strlen(ROLE), 0);
    read_bytes = recv(client_socket, &read_buffer, sizeof(read_buffer), 0);
    emp.role = atoi( read_buffer );
    memset(read_buffer, 0, sizeof(read_buffer));


    write_bytes = send(client_socket, PASSWORD, strlen(PASSWORD), 0);

    read_bytes = recv(client_socket, &read_buffer, sizeof(read_buffer), 0);
    strcpy(emp.password, read_buffer);

    int total_emp_fd = open( "./dataBaseFiles/employee/totalEmp.txt", O_RDWR );
    int emp_list_fd = open( "./dataBaseFiles/employee/employee.txt", O_RDWR );

    int total_emp = 0;
    lseek( total_emp_fd, 0, SEEK_SET );


    apply_file_lock( total_emp_fd, LOCK_SHARED, sizeof(total_emp), 0 );
    if( read( total_emp_fd, &total_emp, sizeof(total_emp) ) <= 0 ){
        release_file_lock( total_emp_fd, sizeof(total_emp), 0 );


        total_emp = 0;

        emp.id = total_emp;
        for( int i = 0; i < 15; i++ ){
            emp.loanAssigned[i] = -1;
        }


        apply_file_lock( emp_list_fd, LOCK_EXCLUSIVE, sizeof(emp), sizeof(emp) * emp.id );

        lseek( emp_list_fd, 0, SEEK_SET );
        write( emp_list_fd, &emp, sizeof(emp) );

        release_file_lock( emp_list_fd, sizeof(emp), sizeof(emp) * emp.id );


        apply_file_lock( total_emp_fd, LOCK_EXCLUSIVE, sizeof(total_emp), 0 );
        
        total_emp++;
        lseek( total_emp_fd, 0, SEEK_SET );
        write( total_emp_fd, &total_emp, sizeof(total_emp) );

        release_file_lock( total_emp_fd, sizeof(total_emp), 0 );
    }
    else{
        release_file_lock( total_emp_fd, sizeof(total_emp), 0 );


        emp.id = total_emp;
        for( int i = 0; i < 15; i++ ){
            emp.loanAssigned[i] = -1;
        }


        apply_file_lock( emp_list_fd, LOCK_EXCLUSIVE, sizeof(emp), sizeof(emp) * emp.id );

        lseek( emp_list_fd, sizeof(emp) * total_emp, SEEK_SET );
        write( emp_list_fd, &emp, sizeof(emp) );

        release_file_lock( emp_list_fd, sizeof(emp), sizeof(emp) * emp.id );

    
        apply_file_lock( total_emp_fd, LOCK_EXCLUSIVE, sizeof(total_emp), 0 );

        total_emp++;
        lseek( total_emp_fd, 0, SEEK_SET );
        write( total_emp_fd, &total_emp, sizeof(total_emp) );

        release_file_lock( total_emp_fd, sizeof(total_emp), 0 );
    }

    close( total_emp_fd );
    close( emp_list_fd );

    return 0;
}

int modifyCustEmp( int client_socket ){
    int read_bytes, write_bytes;
    char read_buffer[1000], write_buffer[1000];

    memset(read_buffer, 0, sizeof(read_buffer));
    memset(write_buffer, 0, sizeof(write_buffer));

    send( client_socket, CUST_OR_EMP, strlen(CUST_OR_EMP), 0 );

    int choice;
    recv(client_socket, read_buffer, sizeof(read_buffer), 0);
    choice = atoi(read_buffer);
    printf("choice val %d\n", choice);

    if (choice == 1) { // Modify Customer Details
        struct Customer customer;
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
        
        strcpy( write_buffer, "Changed Successfully\n");
        send( client_socket, write_buffer, sizeof(write_buffer), 0 );

    } 
    else if (choice == 2) { // Modify Employee Details
        struct Employee emp;
        send( client_socket, ASK_CUST_ACCNO, strlen(ASK_CUST_ACCNO), 0 );
        
        recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
        int emp_id = atoi(read_buffer);
        memset(read_buffer, 0, sizeof(read_buffer));

        
        int emp_list_fd = open( "./dataBaseFiles/employee/employee.txt", O_RDWR );


        apply_file_lock( emp_list_fd, LOCK_SHARED, sizeof(emp), sizeof(emp) *emp_id );

        lseek( emp_list_fd, sizeof(emp) * emp_id, SEEK_SET );
        read( emp_list_fd, &emp, sizeof(emp) );

        release_file_lock( emp_list_fd, sizeof(emp), sizeof(emp) *emp_id );


        sprintf( write_buffer, "Acc. No: %d\nName: %s\nAge: %d\nGender: %c\n\n", emp.id, emp.name, emp.age, emp.gender );
        strcat( write_buffer, ASK_NAME );

        send( client_socket, write_buffer, sizeof(write_buffer), 0 );
        // send( client_socket, ASK_NAME, strlen(ASK_NAME), 0 );

        recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
        strcpy( emp.name, read_buffer );
        memset(read_buffer, 0, sizeof(read_buffer));

        send( client_socket, ASK_AGE, strlen(ASK_AGE), 0 );
        while( 1 ){
            // memset(read_buffer, 0, sizeof(read_buffer));
            recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
            emp.age = atoi(read_buffer);
            memset(read_buffer, 0, sizeof(read_buffer));
        
            if( emp.age > 0 && emp.age < 150){
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
            emp.gender = read_buffer[0];
            memset(read_buffer, 0, sizeof(read_buffer));
        
            if( emp.gender == 'M' || emp.gender == 'F' || emp.gender == 'O' ){
                break;
            }
            else{
                strcpy( write_buffer, "Enter valid gender\n" );
                strcat( write_buffer, ASK_GENDER );
                send( client_socket, write_buffer, sizeof(write_buffer), 0 );
                memset(write_buffer, 0, sizeof(write_buffer));
            }
        }


        apply_file_lock( emp_list_fd, LOCK_EXCLUSIVE, sizeof(emp), sizeof(emp) *emp_id );

        lseek( emp_list_fd, sizeof(emp) * emp_id, SEEK_SET );
        write( emp_list_fd, &emp, sizeof(emp) );

        release_file_lock( emp_list_fd, sizeof(emp), sizeof(emp) *emp_id );

        close( emp_list_fd );

        strcpy( write_buffer, "Changed Successfully\n");
        send( client_socket, write_buffer, sizeof(write_buffer), 0 );

    } 
    else {
        send(client_socket, "Invalid choice", MAX_MESSAGE_SIZE, 0);
    }
            
    recv( client_socket, read_buffer, sizeof(read_buffer), 0 );

    return 0;
}

int manage_user_role( int client_socket ){
    struct Employee emp;
    int read_bytes, write_bytes;
    char read_buffer[1000], write_buffer[1000], buffer[1000];

    memset(read_buffer, 0, sizeof(read_buffer));
    memset(write_buffer, 0, sizeof(write_buffer));

    strcpy(write_buffer, "Enter employee Id:");
    write_bytes = send( client_socket, write_buffer, sizeof(write_buffer), 0 );
    memset(write_buffer, 0, sizeof(write_buffer));

    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
    int emp_id = atoi( read_buffer );

    int emp_list_fd = open( "./dataBaseFiles/employee/employee.txt", O_RDWR );

    apply_file_lock( emp_list_fd, LOCK_SHARED, sizeof(emp), sizeof(emp) * emp_id );

    lseek( emp_list_fd, sizeof(emp) * emp_id, SEEK_SET );
    read( emp_list_fd, &emp, sizeof(emp) );

    release_file_lock( emp_list_fd, sizeof(emp), sizeof(emp) * emp_id );

    if( emp.role == 0 ){
        // role is employee
        sprintf( write_buffer, "ID: %d, Role: Employee\nEnter new role (0 for employee and 1 for manager):\n", emp_id );
    }
    else if ( emp.role == 1){
        //role is manager
        sprintf( write_buffer, "ID: %d, Role: Manager\nEnter new role (0 for employee and 1 for manager):\n", emp_id );
    }
    
    write_bytes = send( client_socket, write_buffer, sizeof(write_buffer), 0 );
    memset(write_buffer, 0, sizeof(write_buffer));

    memset(read_buffer, 0, sizeof(read_buffer));
    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
    emp.role = atoi( read_buffer );

    apply_file_lock( emp_list_fd, LOCK_EXCLUSIVE, sizeof(emp), sizeof(emp) * emp_id );

    lseek( emp_list_fd, sizeof(emp) * emp_id, SEEK_SET );
    write_bytes = write( emp_list_fd, &emp, sizeof(emp) );

    release_file_lock( emp_list_fd, sizeof(emp), sizeof(emp) * emp_id );

    if( write_bytes != -1 ){
        strcpy( write_buffer, "Role changed succesfully!\n" );
        send( client_socket, write_buffer, sizeof(write_buffer), 0 );
        recv( client_socket, read_buffer, sizeof(read_buffer), 0 );
    }

    return 0;

}

int changePasswordAdmin( int client_socket, int admin_id ){
    char read_buffer[1000], write_buffer[1000];
    int read_bytes, write_bytes;

    memset(read_buffer, 0, sizeof(read_buffer));
    memset(write_buffer, 0, sizeof(write_buffer));

    int admin_list_fd = open( "./dataBaseFiles/admin/admin.txt", O_RDWR );

    write_bytes = send( client_socket, NEW_PASSWORD, strlen(NEW_PASSWORD), 0 );

    read_bytes = recv( client_socket, read_buffer, sizeof(read_buffer), 0 );


    struct Admin admin;
    apply_file_lock( admin_list_fd, LOCK_EXCLUSIVE, sizeof(admin), sizeof(admin) *admin_id );

    lseek( admin_list_fd, sizeof(admin) * admin_id, SEEK_SET );
    read( admin_list_fd, &admin, sizeof(admin) );

    strcpy( admin.password, read_buffer );
    lseek( admin_list_fd, sizeof(admin) * admin_id, SEEK_SET );
    write_bytes = write( admin_list_fd, &admin, sizeof(admin) );

    release_file_lock( admin_list_fd, sizeof(admin), sizeof(admin) *admin_id );


    close( admin_list_fd );

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

bool handle_admin( int client_socket, struct User *admin ){
    char read_buffer[1000], write_buffer[1000];
    int read_bytes, write_bytes;

    memset(read_buffer, 0, sizeof(read_buffer));
    memset(write_buffer, 0, sizeof(write_buffer));

    printf("admin started\n");
    while( 1 ){
        write_bytes = send(client_socket, ADMINISTRATOR_MENU, strlen(ADMINISTRATOR_MENU), 0);
        if (write_bytes == -1) {
            perror("Writing admin menu\n");
            return 0;
        }

        memset(read_buffer, 0, sizeof(read_buffer));
        read_bytes = recv(client_socket, &read_buffer, sizeof(read_buffer), 0);

        int opt = atoi( read_buffer );
        // recv( client_socket, &opt, sizeof(opt), 0 );

        printf("opt val %d\n", opt);
        switch ( opt ){
            case 1 :   // Add New Bank Employee
                addEmployee( client_socket );
                break;
            case 2 :  // Modify Customer/Employee Details
                modifyCustEmp( client_socket );
                break;
            case 3 :  // Manage User Roles
                manage_user_role( client_socket );
                break;
            case 4 :  // Change Password
                changePasswordAdmin( client_socket, admin->id );
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
                send(client_socket, "Invalid choice", MAX_MESSAGE_SIZE, 0);
                return true;
                break;
            
        }
    
    }
    return false;
}


#endif