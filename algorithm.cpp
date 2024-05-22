#include <bits/stdc++.h>
#include <mpi.h>
#include <ctime>
#include "algorithm.h"

using namespace std;

vector<Edge> edges_p;
queue<pair<int,vector<int>>> deferred;

int findIndex(int j){

    for(int ind = 0; ind < edges_p.size(); ind++) 
        if(edges_p[ind].node == j) return ind;
    
    return -1;
    
}
void wakeUp(){

    int m = edges_p[0].node;
    edges_p[0].type = BRANCH_E;
    LN = 0;
    SN = FOUND;
    rec_p = 0;
    
    // Connect(0) on edge m
    int buff_send = 0;
    MPI_Send(&buff_send, 1, MPI_INT, m, CONNECT, MPI_COMM_WORLD);
}

// 2. receipt of CONNECT
void receiveConnect(int j, int L){

    if (SN == SLEEP){
        wakeUp();
    }

    int ind = findIndex(j);

    if(L < LN){
        edges_p[ind].type = BRANCH_E;

        int buff_send[3] = {LN,FN,SN};
        MPI_Send(&buff_send,3,MPI_INT,j,INITIATE,MPI_COMM_WORLD);
    }
    else if(edges_p[ind].type == BASIC_E){
        // wait
        vector<int> temp = {j, L};
        deferred.push(make_pair(CONNECT,temp));
    }
    else{

        int buff_send[3] = {LN+1,edges_p[ind].weight,FIND};
        MPI_Send(&buff_send,3,MPI_INT,j,INITIATE,MPI_COMM_WORLD);
    }
}

void receiveInitiate(int L, int F, int S, int j){

    LN = L, FN = F, SN = S, father = j, bestch = UDEF, bestwt = INF;

    for(int i = 0; i < edges_p.size(); i++){
        int nxt_node = edges_p[i].node;
        int nxt_type = edges_p[i].type;
        if(nxt_node != j && nxt_type == BRANCH_E){
            int buff_send[3] = {L,F,S};
            MPI_Send(&buff_send,3,MPI_INT,nxt_node,INITIATE,MPI_COMM_WORLD);
        }
    }

    if(SN == FIND){
        rec_p = 0;
        test();
    }
}

void test(){

    bool flag_p = false;
    int indx = -1;

    for(int i = 0; i < edges_p.size(); i++){

        if(edges_p[i].type == BASIC_E){
            flag_p = true;
            indx = i;
            break;
        }
    }

    if(flag_p){
        testch = edges_p[indx].node;
        int buff_send[2] = {LN,FN};
        MPI_Send(&buff_send, 2, MPI_INT, testch, TEST, MPI_COMM_WORLD);
    }
    else{
        testch = UDEF;
        report();
    }
}

void receiveTest(int L, int F, int j){

    int ind = findIndex(j);
    if(L > LN){
        vector<int> temp = {L,F,j};
        deferred.push(make_pair(TEST,temp));
    }
    else if(F != FN){
        int buff = 0;
        MPI_Send(&buff, 1, MPI_INT, j, ACCEPT, MPI_COMM_WORLD);
    }
    else{
        if(edges_p[ind].type == BASIC_E) edges_p[ind].type = REJECT_E;

        if(j != testch){
            int buff = 0;
            MPI_Send(&buff, 1, MPI_INT, j, REJECT, MPI_COMM_WORLD);
        }
        else{
            test();
        }
    }
}

void receiveAccept(int j){

    int ind = findIndex(j);
    testch = UDEF;

    if(edges_p[ind].weight < bestwt){
        bestwt = edges_p[ind].weight;
        bestch = j;
    }

    report();
}

void receiveReject(int j){
    int ind = findIndex(j);
    if(edges_p[ind].type == BASIC_E) edges_p[ind].type = REJECT_E;

    test();
}

int find_report_num(){
    
    int num = 0;
    for(int i = 0;i < edges_p.size();i++)
    if(edges_p[i].type == BRANCH_E and edges_p[i].node != father) num++;

    return num;
    
}

void report(){

    int num = find_report_num();

    if((rec_p == num) && (testch == UDEF)){
        SN = FOUND;
        int buff = bestwt;

        MPI_Send(&buff,1,MPI_INT,father, REPORT, MPI_COMM_WORLD);
    }

}
void receiveReport(int W, int j){

    int ind = findIndex(j);

    if(j == father){
        if(SN == FIND){
            vector<int> temp = {W,j};
            deferred.push(make_pair(REPORT,temp));
        }
        else if(W > bestwt) 
            changeRoot();
        else if((W == bestwt) && (bestwt == INF)){
            terminateProcess(-1);
        }
    }
    else{
        if(W < bestwt){
            bestwt = W;
            bestch = j;
        }
        rec_p += 1;
        report();
    }
}
void changeRoot(){

    int ind = findIndex(bestch);

    if(edges_p[ind].type == BRANCH_E){
        int buff = 0;
        MPI_Send(&buff, 1, MPI_INT, bestch, CHANGE_ROOT, MPI_COMM_WORLD);
    }
    else{
        edges_p[ind].type = BRANCH_E;
        int buff = LN;
        MPI_Send(&buff, 1, MPI_INT,bestch, CONNECT, MPI_COMM_WORLD);
    }

}

void receiveChangeRoot()
{
    changeRoot();
}

void terminateProcess(int j){

    for(int i = 0; i < edges_p.size(); i++){
        if(edges_p[i].node != j){
            if(edges_p[i].type == BRANCH_E){
                int buff = 0;
                MPI_Send(&buff, 1, MPI_INT, edges_p[i].node, TERMINATE, MPI_COMM_WORLD);
            }
        } 
    }
}

int main(int argc, char* argv[]) {
   
    // Main function to run the GHS algorithm
    // Input: Number of nodes and adjacency matrix of the graph
    // Output: MST of the graph

    // Initialize MPI
    MPI_Init(&argc, &argv);
    int size, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    
    int n;

    if (rank == ROOT) {

        cin >> n;

        if(n-size != 0){
            cout << endl;
            cout << "Number of nodes should be equal to number of processors" << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
            exit(1);
        }
        
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                
                int x; cin >> x;
                
                int v[2];
                v[0] = j;
                v[1] = x;

                MPI_Send(v, 2, MPI_INT, i, 0, MPI_COMM_WORLD);
            }
        }

    }

    MPI_Barrier(MPI_COMM_WORLD);
    // Broadcast the number of nodes
    MPI_Bcast(&n, 1, MPI_INT, ROOT, MPI_COMM_WORLD);

    int buff[2];
    for (int i = 0; i < n; ++i) {
        MPI_Recv(buff, 2, MPI_INT, ROOT, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        if(buff[1] != INF){
            Edge e;
            e.node = buff[0];
            e.weight = buff[1];
            e.type = 0;
            edges_p.push_back(e);
        }
    }

    // sort the edges_p
    sort(edges_p.begin(), edges_p.end(), [](Edge a, Edge b) {
        return a.weight < b.weight;
    });

    MPI_Barrier(MPI_COMM_WORLD);

    // Activating the GHS algorithm
    wakeUp();

    while(true){
        
        int flag;
        MPI_Status status;
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);


        if(flag){

            int n_count, MESG, SRC;
            SRC = status.MPI_SOURCE;
            MESG = status.MPI_TAG;

            MPI_Get_count(&status, MPI_INT, &n_count);
            int* n_buff = (int*)malloc(n_count*sizeof(int));

            MPI_Recv(n_buff, n_count, MPI_INT, SRC, MESG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            if(MESG == CONNECT){
                receiveConnect(SRC,n_buff[0]);
            }
            else if(MESG == INITIATE){
                receiveInitiate(n_buff[0],n_buff[1],n_buff[2],SRC);
            }
            else if(MESG == TEST){
                receiveTest(n_buff[0],n_buff[1],SRC);
            }
            else if(MESG == ACCEPT){
                receiveAccept(SRC);
            }
            else if(MESG == REJECT){
                receiveReject(SRC);
            }
            else if(MESG == REPORT){
                receiveReport(n_buff[0],SRC);
            }
            else if(MESG == CHANGE_ROOT){
                receiveChangeRoot();
            }
            else if(MESG == TERMINATE){
                terminateProcess(SRC);
                break;
            }
        }

        if(!deferred.empty()){

            auto data = deferred.front();
            deferred.pop();

            switch (data.first)
            {
                case CONNECT:
                    receiveConnect(data.second[0],data.second[1]);
                    break;
                case REPORT:
                    receiveReport(data.second[0],data.second[1]);
                    break;
                case TEST:
                    receiveTest(data.second[0],data.second[1],data.second[2]);
                    break;
                default:
                    break;
            }
        }
    }

    for(int i = 0;i < edges_p.size();i++)
    {
        int Node = edges_p[i].node;
        int Wgt = edges_p[i].weight;
        int Type = edges_p[i].type;

        if((Type == BRANCH_E) && (Node > rank))
            cout << rank << " " << Node << " " << Wgt << endl;
    }
    MPI_Finalize();

    return 0;
}
