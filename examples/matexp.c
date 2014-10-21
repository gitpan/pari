/*@Ccom $Id: matexp.c,v 1.3.2.1 2003/11/03 13:34:05 bill Exp $ */

#include "pari.h"

GEN
matexp(GEN x,long prec)
{
  ulong ltop = avma;
  long lx=lg(x),i,k,n;
  GEN y,r,s,p1,p2;

  /*@Ccom check that x is a square matrix */
  if (typ(x) != t_MAT) err(typeer,"matexp");
  if (lx == 1) return cgetg(1, t_MAT);
  if (lx != lg(x[1])) err(talker,"not a square matrix");

  /*@Ccom convert x to real or complex of real and compute its $L_2$ norm */
  s = gzero; r = cgetr(prec+1); affsr(1,r); x = gmul(r,x);
  for (i=1; i<lx; i++)
    s = gadd(s, gnorml2((GEN)x[i]));
  if (typ(s) == t_REAL) setlg(s,3);
  s = gsqrt(s,3); /*@Ccom we do not need much precision on s */

  /*@Ccom if s$ < 1$ we are happy */
  k = expo(s);
  if (k < 0) { n = 0; p1 = x; }
  else { n = k+1; p1 = gmul2n(x,-n); setexpo(s,-1); }

  /*@Ccom initializations before the loop */
  y = gscalmat(r,lx-1); /*@Ccom creates scalar matrix with r on diagonal */
  p2 = p1; r = s; k = 1;
  y = gadd(y,p2);

  /*@Ccom the main loop */
  while (expo(r) >= -BITS_IN_LONG*(prec-1))
  {
    k++; p2 = gdivgs(gmul(p2,p1),k);
    r = gdivgs(gmul(s,r),k); y = gadd(y,p2);
  }

  /*@Ccom square back n times if necessary */
  for (i=0; i<n; i++) y = gsqr(y);
  return gerepileupto(ltop,y);
}


int
main()
{
  long d, prec = 3;
  GEN x;

  /*@Ccom take a stack of $10^6$ bytes, no prime table */
  pari_init(1000000, 2);
  printf("precision of the computation in decimal digits:\n");
  d = itos(lisGEN(stdin));
  if (d > 0) prec = (long)(d*pariK1+3);

  printf("input your matrix in GP format:\n");
  x = matexp(lisGEN(stdin), prec);

  sor(x, 'g', d, 0);
  exit(0);
}
