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
int pRank, commSize;
int stateOfNode = SLEEPING;
int levelOfNode;
int rec_p;
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

void receiveConnect(int L, int j) {
    if (stateOfNode == SLEEPING){
        wakeup();
    }
}
void receiveInitiate(int L, int F, int S, int j) {
}
void receiveTest(int L, int F, int j) {
}
void receiveAccept(int j) {
}
void receiveReject(int j) {
}
void receiveReport(int w, int j) {
}
void receiveChangeRoot() {
}
void terminateP(int j) {
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
                    int fragment = buffer[1];
                    int nodeState = buffer[2];
                    receiveInitiate(level, fragment, nodeState, source);
                    break;
                case TEST:
                    int level = buffer[0];
                    int fragment = buffer[1];
                    receiveTest(level, fragment, source);
                    break;
                case ACCEPT:
                    receiveAccept(source);
                    break;
                case REJECT:
                    receiveReject(source);             
                    break;
                case REPORT:
                    int w = buffer[0];
                    receiveReport(w, source);
                    break;
                case CHANGE_ROOT:
                    receiveChangeRoot();
                    break;
                case TERMINATE:
                    terminateP(source);
                    break;
                case default:
                    cout << "Invalid message" << endl;
            }
        }

    }
}
