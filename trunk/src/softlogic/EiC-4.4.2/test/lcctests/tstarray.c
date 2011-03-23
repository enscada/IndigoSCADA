#include <stdio.h>

int x[3][4], *y[3];

void f()
{
	int i, j;

	for (i = 0; i < 3; i++)
		for (j = 0; j < 4; j++)
			printf(" %d", x[i][j]);
	printf("\n");
	for (i = 0; i < 3; i++)
		for (j = 0; j < 4; j++)
			printf(" %d", y[i][j]);
	printf("\n");
}

void g(int x[][4], int *y[])
{
	int i, j;

	for (i = 0; i < 3; i++)
		for (j = 0; j < 4; j++)
			printf(" %d", x[i][j]);
	printf("\n");
	for (i = 0; i < 3; i++)
		for (j = 0; j < 4; j++)
			printf(" %d", y[i][j]);
	printf("\n");
}


int main()
{
	int z[3][4];
	int i, j, *p;

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 4; j++)
			x[i][j] = 1000*i + j;
		y[i] = x[i];
	}
	f();
	for (i = 0; i < 3; i++) {
		y[i] = p = &z[i][0];
		for (j = 0; j < 4; j++)
			p[j] = x[i][j];
	}
	g(z, y);
	return 0;
}


#ifdef EiCTeStS
main();
#endif
