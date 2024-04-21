// variables.h
#ifndef VARIABLES_H
#define VARIABLES_H

#define ROOT 0
#define UDEF -1
#define INF 100000

// States of the nodes
#define SLEEP 0
#define FIND 1
#define FOUND 2

// Types of the edges
#define BASIC_E 0
#define BRANCH_E 1
#define REJECT_E -1

int size, rank, rec_p;
int LN = 0; // level
int SN = 0; // State
int FN;     // fragment name
int father;
int bestch;
int bestwt;
int testch;

enum {
    CONNECT, 
    INITIATE, 
    TEST, 
    ACCEPT, 
    REJECT, 
    REPORT, 
    CHANGE_ROOT,
    TERMINATE
};
// Structure to store the edge
struct Edge {
    int node, weight, type;
};

void wakeUp();
void receiveConnect(int j, int L);
void receiveInitiate(int L, int F, int S, int j);
void test();
void receiveTest(int L, int F, int j);
void receiveAccept(int j);
void receiveReject(int j);
void report();
void receiveReport(int W, int j);
void changeRoot();
void receiveChangeRoot();
void terminateProcess(int j);
int findIndex(int j);
int find_report_num();

#endif
