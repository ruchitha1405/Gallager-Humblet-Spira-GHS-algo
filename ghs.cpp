// implement GHS algorithm using MPIC++
// assuming there can be edges with same weight
#include <bits/stdc++.h>
#include <mpi.h>

using namespace std;

#define INF 100000
// #define UDEF -1
// state of edjes
#define BASIC 0
#define BRANCH 1
#define REJECT 2

// Tuple indices of edge
#define NODE2 0
#define WEIGHT 1
#define EDGEID 2
#define STATE 3

// node states
#define SLEEPING 0
#define FIND 1
#define FOUND 2
// messages
#define CONNECT 0
#define INITIATE 1
#define TEST 2
#define ACCEPT 3
#define REJECT 4
#define REPORT 5
#define CHANGE_ROOT 6
#define TERMINATE 7

vector<tuple<int, int, int, int>> edgesFromNode; // (node2,weight,edgeId, state)
#define NULL_EDGE make_tuple(-1,-1,-1,-1)
#define INF_WEIGHT make_pair(INF,INF)// weight,edgeId
queue<pair<int,vector<int>>> messageQueue;
int pRank, commSize;
int stateOfNode = SLEEPING;
int levelOfNode;
int fragmentName1, fragmentName2;
int rec_p;
int father;
tuple<int, int, int, int> bestch,testch;
pair<int,int> bestwt; // weight, edgeId
int termination = 1;

void wakeup() {
    int minWeightNode2 = get<NODE2>(edgesFromNode[0]);
    get<STATE>(edgesFromNode[0]) = BRANCH;
    levelOfNode = 0;
    stateOfNode = FOUND;
    rec_p = 0;
    // int buffer = levelOfNode
    MPI_Send(&levelOfNode, 1, MPI_INT, minWeightNode2, CONNECT, MPI_COMM_WORLD);
}
int getIndexOfEdgeWithNode2 (int node2){
    for (int i =0 ; i< edgesFromNode.size();i++){
        if (get<NODE2>(edgesFromNode[i]) == node2){
            return i;
        }
    }
    return -1;
}
void receiveConnect(int L, int j) {
    // if (stateOfNode == SLEEPING){
    //     wakeup();
    // }
    // get edge j
    int indexOfEdgeJ = getIndexOfEdgeWithNode2(j);
    if (indexOfEdgeJ == -1){
        cout << "Error: Edge_j not found in receiveConnect of pRank "<< pRank << endl;
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    if (L < levelOfNode){
        get<STATE>(edgesFromNode[indexOfEdgeJ]) = BRANCH;
        // send INITIATE
        int buffer[4] = {levelOfNode,fragmentName1, fragmentName2,stateOfNode};
        MPI_Send(&buffer, 4, MPI_INT, j, INITIATE, MPI_COMM_WORLD);
        if (stateOfNode == FIND){
            rec_p++;
        }
    }
    else {
        if (get<STATE>(edgesFromNode[indexOfEdgeJ]) == BASIC){
            vector<int> data = {L,j};
            messageQueue.push({make_pair(CONNECT,data)});
        }
        else{
            int buffer[4] = {levelOfNode+1, get<WEIGHT>(edgesFromNode[indexOfEdgeJ]), get<EDGEID>(edgesFromNode[indexOfEdgeJ]),FIND };
            MPI_Send(&buffer, 4, MPI_INT, j, INITIATE, MPI_COMM_WORLD);
        }
    }
}
void receiveInitiate(int L, int F1, int F2,int S, int j) {
    levelOfNode , fragmentName1, fragmentName2, stateOfNode = L,F1,F2,S;
    bestch = NULL_EDGE;
    bestwt = INF_WEIGHT;
    for (auto edge: edgesFromNode){
        if (get<NODE2>(edge) != j && get<STATE>(edge) == BRANCH) {
            int buffer[4] = {L,F1,F2,S};
            MPI_Send(&buffer,4,MPI_INT,get<NODE2>(edge),INITIATE,MPI_COMM_WORLD);
            // if (S == FIND){
            //     rec_p++;
            // }
        }
    }
    if (S == FIND){
        rec_p = 0; // not in paper
        test();
    }
}
void test(){
    int minWeightBasicEdge = -1;
    for (int ind = 0; ind < edgesFromNode.size(); ind++){
        if (get<STATE>(edgesFromNode[ind]) == BASIC){
            minWeightBasicEdge = ind;
            break;
        }
    }
    if (minWeightBasicEdge != -1){
        testch = edgesFromNode[minWeightBasicEdge];
        int buffer[3]= {levelOfNode,fragmentName1,fragmentName2};
        MPI_Send(&buffer,3,MPI_INT,get<NODE2>(testch),TEST,MPI_COMM_WORLD);
    }
    else {
        testch = NULL_EDGE; // not in paper
        report();
    }
}
void receiveTest(int L, int F1,int F2, int j) {
    if (stateOfNode == SLEEPING) wakeup();
    if (L > levelOfNode){
        vector<int> data = {L,F1,F2,j};
        messageQueue.push(make_pair(TEST,data));
    }
    else if (F1 != fragmentName1 && F2 != fragmentName2){
        int buffer = 0;
        MPI_Send(&buffer,1,MPI_INT,j,ACCEPT,MPI_COMM_WORLD);
    }
    else {
        int indexOfEdgeJ = getIndexOfEdgeWithNode2(j);
        if (indexOfEdgeJ == -1){
            cout << "Error: Edge_j not found in receiveTest of pRank "<< pRank << endl;
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }      
        if (get<STATE>(edgesFromNode[indexOfEdgeJ])== BASIC) {
            get<STATE>(edgesFromNode[indexOfEdgeJ]) = REJECT;
        }
        if (get<NODE2>(testch) != j){
            int buffer = 0;
            MPI_Send(&buffer,1,MPI_INT,j,REJECT,MPI_COMM_WORLD);
        }
        else {
            test();
        }
    }
}
void receiveAccept(int j) {
    testch = NULL_EDGE;
    int indexOfEdgeJ = getIndexOfEdgeWithNode2(j);
    if (indexOfEdgeJ == -1){
        cout << "Error: Edge_j not found in receiveAccept of pRank "<< pRank << endl;
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }      
    if (get<WEIGHT>(edgesFromNode[indexOfEdgeJ]) < bestwt.first || (get<WEIGHT>(edgesFromNode[indexOfEdgeJ]) == bestwt.first && get<EDGEID>(edgesFromNode[indexOfEdgeJ]) < bestwt.second)){
        bestch = edgesFromNode[indexOfEdgeJ];
        bestwt = make_pair(get<WEIGHT>(edgesFromNode[indexOfEdgeJ]),get<EDGEID>(edgesFromNode[indexOfEdgeJ]));
    }
    report();
}
void receiveReject(int j) {
    int indexOfEdgeJ = getIndexOfEdgeWithNode2(j);
    if (indexOfEdgeJ == -1){
        cout << "Error: Edge_j not found in receiveReject of pRank "<< pRank << endl;
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    if(get<STATE>(edgesFromNode[indexOfEdgeJ]) == BASIC){
        get<STATE>(edgesFromNode[indexOfEdgeJ]) = REJECT;
    }
    test();
}
void report(){
    // if (rec_p ==0 && testch == NULL_EDGE){
    //     stateOfNode = FOUND;
    //     int buffer[2]= {bestwt.first,bestwt.second};
    //     MPI_Send(&buffer,2,MPI_INT,father,REPORT,MPI_COMM_WORLD);
    // }
    int numOfChildren = 0;
    for(auto edge: edgesFromNode){
        if(get<STATE>(edge) == BRANCH && get<NODE2>(edge) != father){
            numOfChildren++;
        }
    }
    if (rec_p == numOfChildren && testch == NULL_EDGE){
        stateOfNode = FOUND;
        int buffer[2] = {bestwt.first,bestwt.second};
        MPI_Send(&buffer,2,MPI_INT,father,REPORT,MPI_COMM_WORLD);
    }
}
void receiveReport(int w,int id, int j) {
    int indexOfEdgeJ = getIndexOfEdgeWithNode2(j);
    if (indexOfEdgeJ == -1){
        cout << "Error: Edge_j not found in receiveReject of pRank "<< pRank << endl;
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    if(father != j){
        if (w<bestwt.first || (w == bestwt.first && id < bestwt.second)){
            bestwt = make_pair(w,id);
            bestch = edgesFromNode[indexOfEdgeJ];
        }
        rec_p++;
        report();
    }
    else {
        if (stateOfNode == FIND){
            vector<int> data = {w,id,j};
            messageQueue.push(make_pair(REPORT,data));
        }
        else if (w > bestwt.first || (w == bestwt.first && id > bestwt.second)){
            changeRoot();
        }
        else if(w==bestwt.first && id == bestwt.second && bestwt.first == INF && bestwt.second == INF){
            terminateP(-1);
            termination = 0;
        }
    }
}
void changeRoot(){
    int indexOfEdgeJ = getIndexOfEdgeWithNode2(get<NODE2>(bestch));
    if (indexOfEdgeJ == -1){
        cout << "Error: Edge_j not found in receiveReject of pRank "<< pRank << endl;
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    if (get<STATE>(edgesFromNode[indexOfEdgeJ]) == BRANCH){
        int buffer =0;
        MPI_Send(&buffer,1,MPI_INT,get<NODE2>(bestch),CHANGE_ROOT,MPI_COMM_WORLD);
    }
    else {
        get<STATE>(edgesFromNode[indexOfEdgeJ]) = BRANCH;
        int buffer = levelOfNode;
        MPI_Send(&buffer,1,MPI_INT,get<NODE2>(bestch),CONNECT,MPI_COMM_WORLD);
    }
}
void receiveChangeRoot() {
    changeRoot();
}
// send the TERMINATE message to neighbours with state BRANCH
void terminateP(int j) {
    for (auto edge: edgesFromNode){
        if(get<STATE>(edge) == BRANCH && get<NODE2>(edge) != j){
            int buffer = 0;
            MPI_Send(&buffer, 1, MPI_INT, get<NODE2>(edge), TERMINATE, MPI_COMM_WORLD);
        }
    }
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    
    MPI_Comm_rank(MPI_COMM_WORLD, &pRank);
    MPI_Comm_size(MPI_COMM_WORLD, &commSize);

    int numOfNodes;
    // assuming root is 0
    int rootRank = 0;
    // read input at root
    if (pRank == rootRank) {
        cin >> numOfNodes;
        if (numOfNodes != commSize) {
            cout << "Number of nodes and processes should be equal" << endl;
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
        // take in adjacency matrix
        for (int i=0; i< numOfNodes;i++){
            for (int j=0; j< numOfNodes;j++){
                int weight;
                cin >> weight;
                // assign a unique id to edge
                int uniqueId = i*numOfNodes + j;
                 {
                    if (i == rootRank){
                        if (weight != INF) {
                            edgesFromNode.push_back(make_tuple(j, weight, uniqueId, BASIC));
                        }
                    }
                    else{
                        int buffer[4] = {j, weight, uniqueId, BASIC};
                        MPI_Send(&buffer, 4, MPI_INT, i, 0, MPI_COMM_WORLD);
                    }
                }

            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Bcast(&numOfNodes, 1, MPI_INT, rootRank, MPI_COMM_WORLD);
    // receive outgoing edges of the current node
    if (pRank != rootRank) {
        for (int i=0; i< numOfNodes;i++){
            int buffer[4];
            MPI_Recv(&buffer, 4, MPI_INT, rootRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (buffer[1] != INF) {
                edgesFromNode.push_back(make_tuple(buffer[0], buffer[1], buffer[2], buffer[3]));
            }
        }
    }

    sort(edgesFromNode.begin(), edgesFromNode.end(), [](tuple<int, int, int, int> a, tuple<int, int, int, int> b) {
        if (get<WEIGHT>(a) == get<WEIGHT>(b)) {
            return get<EDGEID>(a) < get<EDGEID>(b);
        }
        return get<WEIGHT>(a) < get<WEIGHT>(b);
    });

    MPI_Barrier(MPI_COMM_WORLD);

    // GHS algorithm starts here

    // Initialize nodes
    wakeup();

    while (termination){
        MPI_Status status;
        int flag;
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
        if (flag){
            int message = status.MPI_TAG;
            int source = status.MPI_SOURCE;
            int numElements;
            MPI_Get_count(&status, MPI_INT, &numElements);
            int buffer[numElements];
            MPI_Recv(&buffer, numElements, MPI_INT, source, message, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            switch (message){
                case CONNECT:
                    int level = buffer[0];
                    receiveConnect(level, source);
                    break;
                case INITIATE:
                    int level = buffer[0];
                    int f1 = buffer[1];
                    int f2 = buffer[2];
                    int nodeState = buffer[3];
                    receiveInitiate(level, f1,f2, nodeState, source);
                    break;
                case TEST:
                    int level = buffer[0];
                    int f1 = buffer[1];
                    int f2 = buffer[2];
                    receiveTest(level, f1,f2, source);
                    break;
                case ACCEPT:
                    receiveAccept(source);
                    break;
                case REJECT:
                    receiveReject(source);             
                    break;
                case REPORT:
                    int w = buffer[0];
                    int id = buffer[1];
                    receiveReport(w,id, source);
                    break;
                case CHANGE_ROOT:
                    receiveChangeRoot();
                    break;
                case TERMINATE:
                    terminateP(source);
                    break;
                default:
                    cout << "Invalid message" << endl;
            }
        }
        if (messageQueue.size() > 0){
            pair<int,vector<int>> message = messageQueue.front();
            messageQueue.pop();
            switch (message.first){
                case CONNECT :
                    receiveConnect(message.second[0],message.second[1]);
                    break;
                case REPORT:
                    receiveReport(message.second[0],message.second[1],message.second[2]);
                    break;
                case TEST:
                    receiveTest(message.second[0],message.second[1],message.second[2],message.second[3]);
            }
        }

    }
    for (auto edge: edgesFromNode){
        if(get<STATE>(edge)== BRANCH && get<NODE2>(edge)>pRank){
            cout<< pRank << "->" << get<NODE2>(edge)<< "  "<<get<WEIGHT>(edge)<<" "<<get<EDGEID>(edge) <<endl;
        }
    }
    MPI_Finalize();
    return 0;
}
