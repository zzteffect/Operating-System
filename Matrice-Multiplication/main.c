//Author: Jayson Boubin -- Fall 2022
#include "matmul.h"

int* mat1 = NULL;
int* mat2 = NULL;
int* matRes = NULL;
int* matSol = NULL;

int num = 0;

int getVal(int* mat, int nCols, int rows, int cols){
    return mat[rows*nCols + cols];
}

void setMat(int row, int col, int val){
    matRes[row*K + col] = val;
}

int getProduct(int* rowMat, int rowMatCol, int* colMat, int colMatCol, int row, int col){
    int dot = 0;
    for(int i = 0; i < rowMatCol; i++){
        dot += getVal(rowMat, rowMatCol, row, i) * getVal(colMat, colMatCol, i, col);
    }
    return dot;
}

void multiply() {
    int val = 0;

    for (int i = 0; i < M; i++) {
        for (int j = 0; j < K; j++) {
            val = getProduct(mat1, N, mat2, K, i, j);
            setMat(i, j, val);
            //printf("%d\t",val);
        }
        //printf("\n");
    }
}

typedef struct threadInfo{
    int threadNum;
    int nThreads;
}threadInfo;

//multiply one thread
void * thread_work(void * arg) {
    int val = 0;
    threadInfo* info = (threadInfo*) arg;
    int count = 0;
    int portion = M/info->nThreads;
    int stop =(info->threadNum+1)*portion;

    if(info->threadNum == info->nThreads -1){
        stop = M;
    }

    for(int i = info->threadNum*portion; i < stop; i++){
        for (int j = 0; j < K; j++) {
                val = getProduct(mat1, N, mat2, K, i, j);
                setMat(i, j, val);
            }
        }
       

    // while(count == (info->nThreads-1)){//info->threadNum-1
    //     for(int i = (count)*(M/info->threadNum); i < (count+1)*(M/info->threadNum); i++){
    //         for (int j = 0; j < K; j++) {
    //             val = getProduct(mat1, N, mat2, K, i, j);
    //             setMat(i, j, val);
    //         }
    //     }
    //     count++;
    // }
}

//printf("%d\t",val);
//printf("\n");

void multiplyWithThreads(int nThreads){
    pthread_t threads[nThreads];
    num = nThreads;
   
    for(int i = 0; i < nThreads; i++ ) {
        threadInfo* tInfo = malloc(sizeof(threadInfo));
        tInfo->threadNum = i;
        tInfo->nThreads = nThreads;
        pthread_create(&threads[i], NULL, thread_work,tInfo);
      }
    
    for (int j = 0; j < nThreads; j++){
        pthread_join(threads[j], NULL);
    }
}

//--- DO NOT MODIFY BELOW HERE ---
int main(int argc, char* argv[])
{
    
    struct timeval start, end;
    double mult_time;

    if(argc != 2) {
        printf("Please provide 1 argument: #threads (int)\n");
        exit(-1);
    }

    int nthreads = atoi(argv[1]);
    
    if(nthreads <= 0){
        printf("Please enter a correct value for #threads\n");
        exit(-1);
    }
    printf("--- Reading in Matrices ---\n");

    mat1 = initialize(M, N, MAT_A);
    mat2 = initialize(N, K, MAT_B);
    matSol = initialize(M, K, MAT_C);
    matRes = malloc(M * K * sizeof(int)); 

    printf("--- Matrices Successfully Read, Multiplying Now ---\n");

    gettimeofday(&start, NULL);
    if(nthreads > 1){
        printf("--- Multiplying with %d threads ---\n", nthreads);
        multiplyWithThreads(nthreads);
    } else {
        printf("--- Multiplying with a single thread ---\n");
        multiply();
    }
    
    gettimeofday(&end, NULL); 
    mult_time = getTotalTime(start, end);

    if(compare()){
        printf("Correct! Your matrices multiplied properly\n");
    } else {
        printf("Incorrect! Your matrices did not multiply properly. Check your solution and try again.\n");
    }
    
    printf("This operation took %f seconds\n", mult_time);

    //Don't forget to free allocated heap memory!
    
    free(mat1);
    free(mat2);
    free(matRes);
    free(matSol);
    
    return 0;
}


  // for (int i = 0; i < M; i++) {
    //     for (int j = 0; j < K; j++) {
    //         //matSol = 0; initialize 
    //         for (int k = 0; k < N; k++) {
               
    //         }
    //         printf("%d\t", matRes[i*K + N]);
    //     }
    //     printf("\n");
    // }