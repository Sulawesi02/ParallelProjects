#include <iostream>
#include <ctime>
#include <cstdlib>
#include <immintrin.h> // AVX-512

using namespace std;

const int N = 5000; // 定义最大矩阵大小

void reset(float**& A, float*& b, int n) {
    A = new float* [n];
    b = new float[n];
    srand(time(nullptr)); // 初始化随机数种子
    for (int i = 0; i < n; i++) {
        A[i] = new float[n];
        A[i][i] = 1.0;
        for (int j = i; j < n; j++)
            A[i][j] = static_cast<float>(rand()) / RAND_MAX;
    }
    for (int k = 0; k < n; k++)
        for (int i = k + 1; i < n; i++)
            for (int j = 0; j < n; j++)
                A[i][j] += A[k][j];
    // 生成随机向量b
    for (int i = 0; i < n; i++) {
        b[i] = static_cast<float>(rand()) / RAND_MAX;
    }
}

void avx512_align(float** A, float* b, int n) {
    // 消元过程
    for (int k = 0; k < n; k++) {
        __m512 div = _mm512_set1_ps(A[k][k]);
        int j = k + 1;
        while ((int)(&A[k][j]) % 32)
        {
            A[k][j] = A[k][j] / A[k][k];
            j++;
        }
        for (; j + 16 <= n; j += 16) {
            __m512 row_k = _mm512_load_ps(&A[k][j]);
            row_k = _mm512_div_ps(row_k, div);
            _mm512_store_ps(&A[k][j], row_k);
        }
        for (; j < n; j++) {
            A[k][j] = A[k][j] / A[k][k];
        }
        b[k] /= A[k][k];
        A[k][k] = 1.0;
        for (int i = k + 1; i < n; i++) {
            __m512 vik = _mm512_set1_ps(A[i][k]);
            j = k + 1;
            while ((int)(&A[k][j]) % 32)
            {
                A[i][j] = A[i][j] - A[i][k] * A[k][j];
                j++;
            }
            for (; j + 16 <= n; j += 16) {
                __m512 vkj = _mm512_load_ps(&A[k][j]);
                __m512 vij = _mm512_loadu_ps(&A[i][j]);
                __m512 vx = _mm512_mul_ps(vik, vkj);
                vij = _mm512_sub_ps(vij, vx);
                _mm512_storeu_ps(&A[i][j], vij);
            }
            for (; j < n; j++) {
                A[i][j] = A[i][j] - A[i][k] * A[k][j];
            }
            b[i] -= A[i][k] * b[k];
            A[i][k] = 0;
        }
    }
    // 回代过程
    float* x = new float[n];
    x[n - 1] = b[n - 1] / A[n - 1][n - 1];
    for (int i = n - 2; i >= 0; i--) {
        float sum = b[i];
        for (int j = i + 1; j < n; j++) {
            sum -= A[i][j] * x[j];
        }
        x[i] = sum / A[i][i];
    }
}

int main() {
    float** A;
    float* b;

    // 对不同的数据规模进行测试
    for (int sizes : {500, 1000, 2000, 5000}) {
        reset(A, b, sizes);

        clock_t start = clock(); // 开始时间
        avx512_align(A, b, sizes); // 执行高斯消去法
        clock_t end = clock(); // 结束时间

        float time_taken = float(end - start) / CLOCKS_PER_SEC; // 计算时间差
        cout << "Sizes: " << sizes << ", Time taken: " << time_taken << " seconds" << endl;

    }

    return 0;
}
