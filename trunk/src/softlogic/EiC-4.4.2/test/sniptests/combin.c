/* +++Date last modified: 05-Jul-1997 */

/*
** Compute C(n,m) = the number of combinations of n items,
** taken m at a time.
**
** Written by Thad Smith III, Boulder County, CO.
** Released to the Public Domain  10/14/91.
**
** The def of this function is
**      C(n,m)  = n! / (m! * (n-m)!).
** Computing this formula can cause overflow for large values of n,
** even when C(n,m) would be defined.
**
** The first version will not overflow if C(n,m) * (n-m+1) < ULONG_MAX.
** The second version will not overflow if C(n,m) < ULONG_MAX, but
** is slightly more complex.
** Function domain: n >= 0,  0 <= m <= n.
**
** Both versions work by reducing the product as it is computed.  It
** relies on the property that the product on n consecutive integers
** must be evenly divisible by n.
**
** The first version can be changed to make cnm and the return value
** double to extend the range of the function.
*/

#include "snipmath.h"

DWORD ncomb1 (int n, int m)
{
      DWORD cnm = 1UL;
      int i;

      if (m*2 >n) m = n-m;
      for (i=1 ; i <= m; n--, i++)
            cnm = cnm * n / i;
      return cnm;
}

DWORD ncomb2 (int n, int m)
{
      DWORD cnm = 1UL;
      int i, f;

      if (m*2 >n) m = n-m;
      for (i=1 ; i <= m; n--, i++)
      {
            if ((f=n) % i == 0)
                  f   /= i;
            else  cnm /= i;
            cnm *= f;
      }
      return cnm;
}

#ifdef TEST

#include <stdio.h>
#include <stdlib.h>

#ifdef __WATCOMC__
 #pragma off (unreferenced);
#endif
#ifdef __TURBOC__
 #pragma argsused
#endif

int main (int argc, char *argv[]) {
    int n,m;
    n = atoi (argv[1]);
    m = atoi (argv[2]);
    printf ("ncomb1 = %lu, ncomb2 = %lu\n", ncomb1(n,m), ncomb2(n,m));
    return 0;
}

#endif /* TEST */




