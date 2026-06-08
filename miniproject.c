#include <stdio.h>
void input(int n, int arr[n]);
int find_max_index(int n, int arr[n]);
void output(int arr[], int max_index);

int main()
{
    int n;

    printf("Enter number of elements: ");
    scanf("%d", &n);

    if(n <= 0)
    {
        printf("Invalid array size\n");
        return 0;
    }

    int arr[n];

    input(n, arr);

    int max_index = find_max_index(n, arr);

    output(arr, max_index);

    return 0;
}

void input(int n, int arr[n])
{
    for(int i = 0; i < n; i++)
    {
        printf("Enter element at index %d: ", i);
        scanf("%d", &arr[i]);
    }
}

int find_max_index(int n, int arr[n])
{
    int max_index = 0;

    for(int i = 1; i < n; i++)
    {
        if(arr[i] > arr[max_index])
        {
            max_index = i;
        }
    }

    return max_index;
}

void output(int arr[], int max_index)
{
    printf("Maximum value = %d\n", arr[max_index]);
    printf("Index of maximum value = %d\n", max_index);
}
