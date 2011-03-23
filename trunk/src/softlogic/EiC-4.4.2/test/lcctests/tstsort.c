#include <stdio.h>
int in[] = {10, 32, -1, 567, 3, 18, 1, -51, 789, 0};


/* putd - output decimal number */
void putd(int n)
{
	if (n < 0) {
		putchar('-');
		n = -n;
	}
	if (n/10)
		putd(n/10);
	putchar(n%10 + '0');
}

int *xx;

/* exchange - exchange *x and *y */
void exchange(int *x, int *y)
{
	int t;

	printf("exchange(%d,%d)\n", x - xx, y - xx);
	t = *x; *x = *y; *y = t;
}

/* partition - partition a[i..j] */
int partition(int *a, int i, int j)
{
	int v, k;

	j++;
	k = i;
	v = a[k];
	while (i < j) {
		i++; while (a[i] < v) i++;
		j--; while (a[j] > v) j--;
		if (i < j) exchange(&a[i], &a[j]);
	}
	exchange(&a[k], &a[j]);
	return j;
}

/* quick - quicksort a[lb..ub] */
void quick(int a[], int lb, int ub)
{
	int k, partition(int *a,int i, int j);

	if (lb >= ub)
		return;
	k = partition(a, lb, ub);
	quick(a, lb, k - 1);
	quick(a, k + 1, ub);
}


/* sort - sort a[0..n-1] into increasing order */
void sort(int a[], int n)
{
	quick(xx = a, 0, --n);
}


int main()
{
	int i;

	sort(in, (sizeof in)/(sizeof in[0]));
	for (i = 0; i < (sizeof in)/(sizeof in[0]); i++) {
		putd(in[i]);
		putchar('\n');
	}
	return 0;
}

