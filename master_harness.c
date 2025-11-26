// master_harness.c
// LOCATION: ~/Bank_Project/master_harness.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <stdbool.h>

// --- 1. MOCK NETWORK FUNCTIONS ---
ssize_t mock_recv(int sockfd, void *buf, size_t len, int flags) {
    char *c_buf = (char*)buf;
    size_t count = 0;
    char c;

    // Read byte-by-byte until we hit Newline or EOF
    while (count < len - 1) {
        ssize_t r = read(0, &c, 1);
        if (r <= 0) break; // End of input
        
        c_buf[count++] = c;
        if (c == '\n') break; // Found the Enter key!
    }

    c_buf[count] = '\0'; // Null terminate
    
    if (count == 0) exit(0); // Stop cleanly if no data left
    return count;
}

ssize_t mock_send(int sockfd, const void *buf, size_t len, int flags) {
    // Write to stdout (so we can debug if needed)
    return write(1, buf, len); 
}

#define recv mock_recv
#define send mock_send

// --- 2. INCLUDE HEADERS ---
#include "structures/users.h"
#include "structures/loan.h"
#include "structures/transaction.h"
#include "structures/feedback.h"

#include "headers/commonFunc.h"
#include "headers/admin.h"
#include "headers/customer.h"
#include "headers/employee.h"
#include "headers/manager.h"

// --- 3. DATABASE SETUP ---
void setup_dummy_db() {
    // Create folders
    system("mkdir -p dataBaseFiles/admin dataBaseFiles/customer dataBaseFiles/employee dataBaseFiles/loan dataBaseFiles/feedback dataBaseFiles/transaction");

    // --- 1. CREATE TRANSACTION HISTORY (Types 1, 2, 3, 4, 5) ---
    int fd_trans = open("./dataBaseFiles/transaction/transaction.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    struct Transaction t[5];
    memset(t, 0, sizeof(t));

    // ID 1: Deposit
    t[0].tID = 1; t[0].custID = 0; t[0].amount = 100.0; t[0].transactionType = 1; t[0].transactionTime = time(NULL);
    
    // ID 2: Withdraw
    t[1].tID = 2; t[1].custID = 0; t[1].amount = 50.0; t[1].transactionType = 2; t[1].transactionTime = time(NULL);
    
    // ID 3: Transfer Sent
    t[2].tID = 3; t[2].custID = 0; t[2].amount = 20.0; t[2].transactionType = 3; t[2].transactionTime = time(NULL);
    
    // ID 4: Transfer Rec
    t[3].tID = 4; t[3].custID = 0; t[3].amount = 300.0; t[3].transactionType = 4; t[3].transactionTime = time(NULL);
    
    // ID 5: Loan Rec
    t[4].tID = 5; t[4].custID = 0; t[4].amount = 5000.0; t[4].transactionType = 5; t[4].transactionTime = time(NULL);

    // Write them all to file sequentially
    for(int i=0; i<5; i++) {
        write(fd_trans, &t[i], sizeof(struct Transaction));
    }
    close(fd_trans);


    // --- 2. CREATE CUSTOMER (LINKED TO THESE TRANSACTIONS) ---
    int fd = open("./dataBaseFiles/customer/customer.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    struct Customer c1;
    memset(&c1, 0, sizeof(c1));
    
    // Core details
    c1.acc_no = 0; 
    c1.balance = 10000.0; 
    c1.active = true; 
    c1.loanID = -1; // Explicitly no loan
    strcpy(c1.password, "pass"); 
    strcpy(c1.name, "Tester");
    
    // LINKING THE TRANSACTIONS
    c1.tp = 4; 
    c1.transaction[0] = 1; 
    c1.transaction[1] = 2; 
    c1.transaction[2] = 3; 
    c1.transaction[3] = 4; 
    c1.transaction[4] = 5; 
    for(int i=5; i<10; i++) c1.transaction[i] = -1;

    // Create Friend User (ID 1)
    struct Customer c2;
    memset(&c2, 0, sizeof(c2));
    c2.acc_no = 1; c2.balance = 500.0; c2.active = true; c2.loanID = -1;
    strcpy(c2.password, "friend");
    
    write(fd, &c1, sizeof(c1));
    write(fd, &c2, sizeof(c2));
    close(fd);

    // --- 3. EMPLOYEE SETUP (UPDATED) ---
    // We create TWO employees to cover different Roles.
    int fd_emp = open("./dataBaseFiles/employee/employee.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    
    // Employee 0: Regular Employee (Role 0)
    struct Employee e1;
    memset(&e1, 0, sizeof(e1));
    e1.id = 0; e1.role = 0; e1.age = 30; e1.gender = 'M';
    e1.totalLoanAssigned = 0;
    for(int k=0; k<15; k++) e1.loanAssigned[k] = -1;
    e1.loanAssigned[0] = 0;
    strcpy(e1.username, "emp"); strcpy(e1.password, "pass"); strcpy(e1.name, "Worker");
    
    // Employee 1: Manager (Role 1) --> NEEDED FOR COVERAGE
    struct Employee e2;
    memset(&e2, 0, sizeof(e2));
    e2.id = 1; e2.role = 1; e2.age = 40; e2.gender = 'F';
    e2.totalLoanAssigned = 0;
    for(int k=0; k<15; k++) e2.loanAssigned[k] = -1;
    strcpy(e2.username, "mgr"); strcpy(e2.password, "pass"); strcpy(e2.name, "Boss");

    write(fd_emp, &e1, sizeof(e1));
    write(fd_emp, &e2, sizeof(e2));
    close(fd_emp);

    // --- 4. TOTAL EMP (Must match count) ---
    int fd_total = open("./dataBaseFiles/employee/totalEmp.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    int count = 2; // We now have 2 employees
    write(fd_total, &count, sizeof(count));
    close(fd_total);

    // --- 5. OTHER FILES ---
    int fd_loan = open("./dataBaseFiles/loan/loanList.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    struct Loan l;
    memset(&l, 0, sizeof(l));
    l.loanId = 0; 
    l.custId = 0; 
    l.amount = 1000.0; 
    l.loanStatus = 0; // 0 = Pending (Manager can assign this)
    l.empId = -1;     // -1 = Unassigned
    write(fd_loan, &l, sizeof(l));
    close(fd_loan);

    // --- 6. CREATE PENDING FEEDBACK (Crucial for Manager Coverage) ---
    int fd_feed = open("./dataBaseFiles/feedback/feedback.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    struct Feedback f;
    memset(&f, 0, sizeof(f));
    f.feedbackId = 0;
    f.custId = 0;
    f.reviewStatus = 0; // 0 = Not Reviewed (Manager can review this)
    strcpy(f.feedback, "Great Service!");
    write(fd_feed, &f, sizeof(f));
    close(fd_feed);
    
}

// --- 4. MAIN LOOP ---
int main() {
    setup_dummy_db();

    // 1. Read Selector Byte
    char selector;
    if (read(0, &selector, 1) <= 0) return 0;

    // 2. Eat the newline!
    char dummy;
    read(0, &dummy, 1); 

    // 3. Define users
    struct User adminUser = {0, 1, "admin", "pass"};
    struct User empUser   = {0, 2, "emp", "pass"};
    struct User mgrUser   = {0, 3, "mgr", "pass"};
    struct User custUser  = {0, 4, "cust", "pass"};

    // 4. Route
    switch(selector % 4) {
        case 0: handle_customer(0, &custUser, 0); break;
        case 1: handle_admin(0, &adminUser); break;
        case 2: handle_employee(0, &empUser, 0); break;
        case 3: handle_manager(0, &mgrUser); break;
    }
    return 0;
}