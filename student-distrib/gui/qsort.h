//
// Created by liuzikai on 12/11/19.
// Reference：[QuickSort - GeeksforGeeks](https://www.geeksforgeeks.org/quick-sort/)
//

#ifndef _QSORT_H
#define _QSORT_H

// A utility function to swap two elements
static inline void swap(int *a, int *b) {
    int t = *a;
    *a = *b;
    *b = t;
}

static inline int partition(int arr[], int low, int high) {
    int pivot = arr[high];  // pivot
    int i = (low - 1);  // Index of smaller element
    int j;
    for (j = low; j <= high - 1; j++) {
        // If current element is smaller than the pivot
        if (arr[j] < pivot) {
            i++; // increment index of smaller element
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

/**
 * Quick sort the array
 * @param arr     Array to be sorted
 * @param low     Starting index
 * @param high    Ending index
 */
static inline void quick_sort(int *arr, int low, int high) {
    if (low < high) {

        int pi = partition(arr, low, high);

        // Separately sort elements before
        // partition and after partition
        quick_sort(arr, low, pi - 1);
        quick_sort(arr, pi + 1, high);
    }
}

#endif //_QSORT_H
