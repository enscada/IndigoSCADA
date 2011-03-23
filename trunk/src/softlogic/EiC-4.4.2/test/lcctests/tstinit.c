#include <stdio.h>
typedef struct { int codes[3]; char name[6]; } Word;

Word words[] = {
	{{1, 2, 3,}, "if"},
	{ { 4, 5 }, { 'f', 'o', 'r' } },
	{{6, 7, 8}, "else"},
	{ { 9, 10, 11,}, {'w', 'h', 'i', 'l', 'e',} },
	{ {0} },
}, *wordlist = words;

int x[][5] = { {1, 2, 3, 4, 0}, { 5, 6 }, { 7 } };
int *y[] = { x[0], x[1], x[2], 0 };


void f() {
	static char *keywords[] = {"if", "for", "else", "while", 0, };
	char **p;

	for (p = keywords; *p != 0; p++)
		printf("%s\n", *p);
}

void h()
{
	int i;

	for (i = 0; i < sizeof(words)/sizeof(Word); i++)
		printf("%d %d %d %s\n", words[i].codes[0],
			words[i].codes[1], words[i].codes[2],
			&words[i].name[0]);
}

void g(Word *p)
{
	int i;

	for ( ; p->codes[0]; p++) {
		for (i = 0; i < sizeof p->codes/sizeof(p->codes[0]); i++)
			printf("%d ", p->codes[i]);
		printf("%s\n", p->name);
	}
	h();
}


int main()
{
	int i, j;

	for (i = 0; y[i] != 0; i++) {
		for (j = 0; y[i][j]; j++)
			printf(" %d", y[i][j]);
		printf("\n");
	}
	f();
	g(wordlist);
	return 0;
}

#ifdef EiCTeStS
main();
#endif
