#pragma once

// extern "C"
// {
//     double add(double a, double b);
//     void *add_arrays(double *arr, int n);
// }
extern "C"
{
    // C++ 函数，使用 extern "C" 来避免名称改编
    double add(double a, double b)
    {
        return a + b;
    }
    // 新增的 C++ 函数，用于处理数组
    void *add_arrays(double *arr, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            arr[i] = arr[i] + 1.0; // 例如，加上 1.0，你可以根据需要修改这个逻辑
        }
        return (void *)arr; // 返回指向结果数组的指针
    }
}