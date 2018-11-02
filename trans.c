//Jason King, jpking
//Kamil Gumienny, kmgumienny

/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
	//DiagIndx and diagVal are used for values on the diagonal that will not be flipped
	int diagIndx = 0;
	int diagVal = 0;


	/*
	 * blockSize holds the size of the array that will be blocked. When working with a matrix of unknown size,
	 * having a block size of value 16 seems to work the best for matrices of arbitrary size, it's a
	 * good middle ground for between small and large, even though that in worst case this results in a
	 * single blocked out matrix to be cached at a time.
	 */

	int blockSize = 8;

	/*When the matrix is 32x32, the block size is 8x8 values.
	 * We are working with a 1024 byte cache so by using a block size
	 * of 8, there are
	 * 256 bytes to hold Matrix A (8x8 of ints)
	 * 256 bytes to hold Matrix A (8x8 of ints)
	 * 128 Bytes to hold a whole Row of Matrix A
	 * 128 Bytes to hold a whole Row of Matrix B
	 * 128 Bytes to hold a whole Column of Matrix A
	 * 128 Bytes to hold a whole Column of Matrix B
	*/
	if((M == 32) & (N == 32))
		blockSize = 8;

	/*When the matrix is 64x64, the block size is 4x4 values.
	 * We are working with a 1024 byte cache so by using a block size
	 * of 4, there are
	 * 64 bytes to hold Matrix A (4x4 of ints)
	 * 64 bytes to hold Matrix A (4x4 of ints)
	 * 256 Bytes to hold a whole Row of Matrix A
	 * 256 Bytes to hold a whole Row of Matrix B
	 * 256 Bytes to hold a whole Column of Matrix A
	 * 256 Bytes to hold a whole Column of Matrix B
	 *
	 * This results in 128 bytes that aren't stored in cache which results in more
	 * misses than optimal but these numbers seem to work the best
	 */
	if((M == 64) & (N == 64))
		blockSize = 4;


	//4 loops running, first 2 blocking 8/4 rows&columns at the same time and
	//2 inner loops going 1 row and column at a time to transpose matrix.
	//the two inner loops check to make sure there isn't a seg fault with uneven matrix
	for(int cBlock = 0; cBlock < N; cBlock += blockSize) {
		for(int rBlock = 0; rBlock < N; rBlock += blockSize) {
			for(int rNum = rBlock; (rNum < (rBlock + blockSize) && (rNum < N)); rNum++) {
				for(int cNum = cBlock; (cNum < (cBlock + blockSize) && (cNum < M)); cNum++) {
					//checks to make sure there is no diagonal and then switches numbers
					if(rNum != cNum) {
						B[cNum][rNum] = A[rNum][cNum];
						//if diagonal is hit, it is stored in temp values so B other data isn't evicted from cache
					}else {
						diagIndx = rNum;
						diagVal = A[rNum][cNum];
					}
				}
				//Once the whole column is traversed in A, B gets the diagonal values assigned
				if (cBlock == rBlock)
					B[diagIndx][diagIndx] = diagVal;

			}
		}
	}
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

