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

vector<tuple<int, int, int, int>> edgesFromNode; // (node2,weight,edgeId, state)

void wakeup() {
    
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    int pRank, commSize;
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

}
