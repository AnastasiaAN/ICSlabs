/*	name:Luo Anqi
 *	loginID:515030910431
 */
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
    int i, j;
    int t0, t1, t2, t3, t4, t5, t6, t7;

    if (M == 32 && N == 32) 
    {
        for (j = 0; j < 32; j += 8) 
		{	
	        for (i = 0; i < 32; i++)
			{
                t0 = A[i][j];
                t1 = A[i][j+1];
                t2 = A[i][j+2];
                t3 = A[i][j+3];
                t4 = A[i][j+4];
                t5 = A[i][j+5];
                t6 = A[i][j+6];
                t7 = A[i][j+7];
                B[j][i] = t0;
                B[j+1][i] = t1;
                B[j+2][i] = t2;
                B[j+3][i] = t3;
                B[j+4][i] = t4;
                B[j+5][i] = t5;
                B[j+6][i] = t6;
                B[j+7][i] = t7;
            }
        }
    } 
    else if (M == 64 && N == 64) 
    {
		int k;
		for (j = 0; j < 64; j += 8) 
		{
            for (i = 0; i < 64; i += 8) 
			{
				for (k = 0; k < 4; k++) 
				{
                    t0 = A[i+k][j];
                    t1 = A[i+k][j+1];
                    t2 = A[i+k][j+2];
                    t3 = A[i+k][j+3];
                    t4 = A[i+k][j+4];
                    t5 = A[i+k][j+5];
                    t6 = A[i+k][j+6];
                    t7 = A[i+k][j+7];

                    B[j][i+k] = t0;
                    B[j+1][i+k] = t1;
                    B[j+2][i+k] = t2;
                    B[j+3][i+k] = t3;

                    B[j][i+k+4] = t4;
                    B[j+1][i+k+4] = t5;
                    B[j+2][i+k+4] = t6;
                    B[j+3][i+k+4] = t7;
                }
                
                for (k = 0; k < 4; k++) 
				{
                    t0 = B[j+k][i+4];
                    t1 = B[j+k][i+5];
                    t2 = B[j+k][i+6];
                    t3 = B[j+k][i+7];

                    B[j+k][i+4] = A[i+4][j+k];
                    B[j+k][i+5] = A[i+5][j+k];
                    B[j+k][i+6] = A[i+6][j+k];
                    B[j+k][i+7] = A[i+7][j+k];

                    t4 = A[i+4][j+k+4];
                    t5 = A[i+5][j+k+4];
                    t6 = A[i+6][j+k+4];
                    t7 = A[i+7][j+k+4];

                    B[j+k+4][i] = t0;
                    B[j+k+4][i+1] = t1;
                    B[j+k+4][i+2] = t2;
                    B[j+k+4][i+3] = t3;
                    B[j+k+4][i+4] = t4;
                    B[j+k+4][i+5] = t5;
                    B[j+k+4][i+6] = t6;
                    B[j+k+4][i+7] = t7;
                }
            }       
		}
		
		
		/*fail:1651 score:4*/
		/*
		for (j = 0; j < 64; j += 4) 
		{
            for (i = 0; i < 64; i++) 
			{
                t0 = A[j][i];
                t1 = A[j+1][i];
                t2 = A[j+2][i];
                t3 = A[j+3][i];
                B[i][j] = t0;
                B[i][j+1] = t1;
                B[i][j+2] = t2;
                B[i][j+3] = t3;
            }
		}
		*/
	}
    else 
	{//M == 61 && N == 67)  
       for (j = 0; j < 64; j += 8) 
		{
            for (i = 0; i < 61; i++) 
			{
                t0 = A[j][i];
                t1 = A[j+1][i];
                t2 = A[j+2][i];
                t3 = A[j+3][i];
                t4 = A[j+4][i];
                t5 = A[j+5][i];
                t6 = A[j+6][i];
                t7 = A[j+7][i];
                B[i][j] = t0;
                B[i][j+1] = t1;
                B[i][j+2] = t2;
                B[i][j+3] = t3;
                B[i][j+4] = t4;
                B[i][j+5] = t5;
                B[i][j+6] = t6;
                B[i][j+7] = t7;
            }
        }
        for (i = 0; i < 61; i++) 
		{
            t0 = A[64][i];
            t1 = A[65][i];
            t2 = A[66][i];
            B[i][64] = t0;
            B[i][65] = t1;
            B[i][66] = t2;
        }
		
		/*fail:2082*/
	/*	for (j = 0; j < 64; j += 4) 
		{
            for (i = 0; i < 61; i++) 
			{
                t0 = A[j][i];
                t1 = A[j+1][i];
                t2 = A[j+2][i];
                t3 = A[j+3][i];
                               B[i][j] = t0;
                B[i][j+1] = t1;
                B[i][j+2] = t2;
                B[i][j+3] = t3;
                          }
        }
		for (i = 0; i < 61; i++) 
		{
            t0 = A[64][i];
            t1 = A[65][i];
            t2 = A[66][i];
            B[i][64] = t0;
            B[i][65] = t1;
            B[i][66] = t2;
        }
	*/
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

