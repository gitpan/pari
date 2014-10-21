/* $Id: buch1.c,v 1.26.2.1 2001/02/08 18:51:59 karim Exp $

Copyright (C) 2000  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/*******************************************************************/
/*                                                                 */
/*         CLASS GROUP AND REGULATOR (McCURLEY, BUCHMANN)          */
/*                   QUADRATIC FIELDS                              */
/*                                                                 */
/*******************************************************************/
#include "pari.h"

const int narrow = 0; /* should set narrow = flag in buchquad, but buggy */

/* See buch2.c:
 * precision en digits decimaux=2*(#digits decimaux de Disc)+50
 * on prendra les p decomposes tels que prod(p)>lim dans la subbase
 * LIMC=Max(c.(log(Disc))^2,exp((1/8).sqrt(log(Disc).loglog(Disc))))
 * LIMC2=Max(6.(log(Disc))^2,exp((1/8).sqrt(log(Disc).loglog(Disc))))
 * subbase contient les p decomposes tels que prod(p)>sqrt(Disc)
 * lgsub=subbase[0]=#subbase;
 * subfactorbase est la table des form[p] pour p dans subbase
 * nbram est le nombre de p divisant Disc elimines dans subbase
 * powsubfactorbase est la table des puissances des formes dans subfactorbase
 */
#define HASHT 1024 /* power of 2 */
static const long CBUCH = 15; /* of the form 2^k-1 */
static const long randshift=BITS_IN_RANDOM-1 - 4; /* BITS_IN_RANDOM-1-k */

static long KC,KC2,lgsub,limhash,RELSUP,PRECREG;
static long *primfact,*primfact1, *exprimfact,*exprimfact1, *badprim;
static long *factorbase,*numfactorbase, *subbase, *vectbase, **hashtab;
static GEN  **powsubfactorbase,vperm,subfactorbase,Disc,sqrtD,isqrtD;

GEN buchquad(GEN D, double c, double c2, long RELSUP0, long flag, long prec);
extern GEN roots_to_pol_intern(GEN L, GEN a, long v, int plus);
extern GEN roots_to_pol(GEN L, long v);

GEN
quadclassunit0(GEN x, long flag, GEN data, long prec)
{
  long lx,RELSUP0;
  double cbach, cbach2;

  if (!data) lx=1;
  else
  {
    lx = lg(data);
    if (typ(data)!=t_VEC || lx > 7)
      err(talker,"incorrect parameters in quadclassunit");
    if (lx > 4) lx = 4;
  }
  cbach = cbach2 = 0.1; RELSUP0 = 5;
  switch(lx)
  {
    case 4: RELSUP0 = itos((GEN)data[3]);
    case 3: cbach2 = gtodouble((GEN)data[2]);
    case 2: cbach  = gtodouble((GEN)data[1]);
  }
  return buchquad(x,cbach,cbach2,RELSUP0,flag,prec);
}

/*******************************************************************/
/*                                                                 */
/*       Hilbert and Ray Class field using CM (Schertz)            */
/*                                                                 */
/*******************************************************************/

int
isoforder2(GEN form)
{
  GEN a=(GEN)form[1],b=(GEN)form[2],c=(GEN)form[3];
  return !signe(b) || absi_equal(a,b) || egalii(a,c);
}

GEN
getallforms(GEN D, long *pth, GEN *ptz)
{
  long d = itos(D), t, b2, a, b, c, h, dover3 = labs(d)/3;
  GEN z, L=cgetg(labs(d), t_VEC);
  b2 = b = (d&1); h = 0; z=gun;
  while (b2 <= dover3)
  {
    t = (b2-d)/4;
    for (a=b?b:1; a*a<=t; a++)
      if (t%a == 0)
      {
	c = t/a; z = mulsi(a,z);
        L[++h] = (long)qfi(stoi(a),stoi(b),stoi(c));
	if (b && a != b && a*a != t)
          L[++h] = (long)qfi(stoi(a),stoi(-b),stoi(c));
      }
    b+=2; b2=b*b;
  }
  *pth = h; *ptz = z; setlg(L,h+1); return L;
}

#define MOD4(x) ((((GEN)x)[2])&3)
#define MAXL 300
/* find P and Q two non principal prime ideals (above p,q) such that 
 *   (pq, z) = 1  [coprime to all class group representatives]
 *   cl(P) = cl(Q) if P has order 2 in Cl(K)
 * Try to have e = 24 / gcd(24, (p-1)(q-1)) as small as possible */
void
get_pq(GEN D, GEN z, GEN flag, GEN *ptp, GEN *ptq)
{
  GEN wp=cgetg(MAXL,t_VEC), wlf=cgetg(MAXL,t_VEC), court=icopy(gun);
  GEN p, form;
  long i, ell, l = 1, d = itos(D);
  byteptr diffell = diffptr + 2;

  if (typ(flag)==t_VEC)
  { /* assume flag = [p,q]. Check nevertheless */
    for (i=1; i<lg(flag); i++)
    {
      ell = itos((GEN)flag[i]);
      if (smodis(z,ell) && kross(d,ell) > 0)
      {
	form = redimag(primeform(D,(GEN)flag[i],0));
	if (!gcmp1((GEN)form[1])) {
	  wp[l]  = flag[i];
          l++; if (l == 3) break;
	}
      }
    }
    if (l<3) err(talker,"[quadhilbert] incorrect values in flag: %Z", flag);
    *ptp = (GEN)wp[1];
    *ptq = (GEN)wp[2]; return;
  }

  ell = 3;
  while (l < 3 || ell<=MAXL)
  {
    ell += *diffell++; if (!*diffell) err(primer1);
    if (smodis(z,ell) && kross(d,ell) > 0)
    {
      court[2]=ell; form = redimag(primeform(D,court,0));
      if (!gcmp1((GEN)form[1])) {
        wp[l]  = licopy(court);
        wlf[l] = (long)form; l++;
      }
    }
  }
  setlg(wp,l); setlg(wlf,l);

  for (i=1; i<l; i++)
    if (smodis((GEN)wp[i],3) == 1) break;
  if (i==l) i = 1;
  p = (GEN)wp[i]; form = (GEN)wlf[i];
  i = l;
  if (isoforder2(form))
  {
    long oki = 0;
    for (i=1; i<l; i++)
      if (gegal((GEN)wlf[i],form))
      {
        if (MOD4(p) == 1 || MOD4(wp[i]) == 1) break;
        if (!oki) oki = i; /* not too bad, still e = 2 */
      }
    if (i==l) i = oki;
    if (!i) err(bugparier,"quadhilbertimag (can't find p,q)");
  }
  else
  {
    if (MOD4(p) == 3)
    {
      for (i=1; i<l; i++)
        if (MOD4(wp[i]) == 1) break;
    }
    if (i==l) i = 1;
  }
  *ptp = p;
  *ptq = (GEN)wp[i];
}
#undef MAXL

static GEN
gpq(GEN form, GEN p, GEN q, long e, GEN sqd, GEN u, long prec)
{
  GEN a2 = shifti((GEN)form[1], 1);
  GEN b = (GEN)form[2], p1,p2,p3,p4;
  GEN w = lift(chinois(gmodulcp(negi(b),a2), u));
  GEN al = cgetg(3,t_COMPLEX);
  al[1] = lneg(gdiv(w,a2));
  al[2] = ldiv(sqd,a2);
  p1 = trueeta(gdiv(al,p),prec);
  p2 = egalii(p,q)? p1: trueeta(gdiv(al,q),prec);
  p3 = trueeta(gdiv(al,mulii(p,q)),prec);
  p4 = trueeta(al,prec);
  return gpowgs(gdiv(gmul(p1,p2),gmul(p3,p4)), e);
}

/* returns an equation for the Hilbert class field of the imaginary
 *  quadratic field of discriminant D if flag=0, a vector of
 *  two-component vectors [form,g(form)] where g() is the root of the equation
 *  if flag is non-zero.
 */
static GEN
quadhilbertimag(GEN D, GEN flag)
{
  long av=avma,h,i,e,prec;
  GEN z,L,P,p,q,qfp,qfq,up,uq,u;
  int raw = ((typ(flag)==t_INT && signe(flag)));

  if (DEBUGLEVEL>=2) timer2();
  if (gcmpgs(D,-11) >= 0) return polx[0];
  L = getallforms(D,&h,&z);
  if (DEBUGLEVEL>=2) msgtimer("class number = %ld",h);
  if (h == 1) { avma=av; return polx[0]; }

  get_pq(D, z, flag, &p, &q);
  e = 24 / cgcd((smodis(p,24)-1) * (smodis(q,24)-1), 24);
  if(DEBUGLEVEL>=2) fprintferr("p = %Z, q = %Z, e = %ld\n",p,q,e);
  qfp = primeform(D,p,0); up = gmodulcp((GEN)qfp[2],shifti(p,1));
  if (egalii(p,q))
  {
    u = (GEN)compimagraw(qfp,qfp)[2];
    u = gmodulcp(u, shifti(mulii(p,q),1));
  }
  else
  {
    qfq = primeform(D,q,0); uq = gmodulcp((GEN)qfq[2],shifti(q,1));
    u = chinois(up,uq);
  }
  prec = raw? DEFAULTPREC: 3;
  for(;;)
  {
    long av0 = avma, ex, exmax = 0;
    GEN lead, sqd = gsqrt(negi(D),prec);
    P = cgetg(h+1,t_VEC); 
    for (i=1; i<=h; i++)
    {
      GEN v, s = gpq((GEN)L[i], p, q, e, sqd, u, prec);
      if (raw)
      {
        v = cgetg(3,t_VEC); P[i] = (long)v;
        v[1] = L[i];
        v[2] = (long)s;
      }
      else P[i] = (long)s;
      ex = gexpo(s); if (ex > 0) exmax += ex; 
    }
    if (DEBUGLEVEL>=2) msgtimer("roots");
    if (raw) { P = gcopy(P); break; }
    /* to avoid integer overflow (1 + 0.) */
    lead = (exmax < bit_accuracy(prec))? gun: realun(prec);

    P = greal(roots_to_pol_intern(lead,P,0,0));
    P = grndtoi(P,&exmax);
    if (DEBUGLEVEL>=2) msgtimer("product, error bits = %ld",exmax);
    if (exmax <= -10)
    {
      if (typ(flag)==t_VEC && !issquarefree(P)) { avma=av; return gzero; }
      break;
    }
    avma = av0; prec += (DEFAULTPREC-2) + (1 + (exmax >> TWOPOTBITS_IN_LONG));
    if (DEBUGLEVEL) err(warnprec,"quadhilbertimag",prec);
  }
  return gerepileupto(av,P);
}

GEN quadhilbertreal(GEN D, GEN flag, long prec);

GEN
quadhilbert(GEN D, GEN flag, long prec)
{
  if (!flag) flag = gzero;
  if (typ(D)!=t_INT)
  {
    D = checkbnf(D);
    if (degree(gmael(D,7,1))!=2)
      err(talker,"not a polynomial of degree 2 in quadhilbert");
    D=gmael(D,7,3);
  }
  else
  {
    if (!isfundamental(D))
      err(talker,"quadhilbert needs a fundamental discriminant");
  }
  return (signe(D)>0)? quadhilbertreal(D,flag,prec)
                     : quadhilbertimag(D,flag);
}

/* AUXILLIARY ROUTINES FOR QUADRAYIMAGWEI */
#define to_approx(nf,a) ((GEN)gmul(gmael((nf),5,1), (a))[1])

/* Z-basis for a (over C) */
static GEN
get_om(GEN nf, GEN a)
{
  GEN om = cgetg(3,t_VEC);
  om[1] = (long)to_approx(nf,(GEN)a[2]);
  om[2] = (long)to_approx(nf,(GEN)a[1]);
  return om;
}

/* Compute all elts in class group G = [|G|,c,g], c=cyclic factors, g=gens.
 * Set list[j + 1] = g1^e1...gk^ek where j is the integer
 *   ek + ck [ e(k-1) + c(k-1) [... + c2 [e1]]...] */
static GEN
getallelts(GEN nf, GEN G)
{
  GEN C,c,g, *list, **pows, *gk;
  long lc,i,j,k,no;

  no = itos((GEN)G[1]);
  c = (GEN)G[2];
  g = (GEN)G[3]; lc = lg(c)-1;
  list = (GEN*) cgetg(no+1,t_VEC);
  if (!lc)
  {
    list[1] = idealhermite(nf,gun);
    return (GEN)list;
  }
  pows = (GEN**)cgetg(lc+1,t_VEC);
  c = dummycopy(c); settyp(c, t_VECSMALL);
  for (i=1; i<=lc; i++)
  {
    c[i] = k = itos((GEN)c[i]);
    gk = (GEN*)cgetg(k, t_VEC); gk[1] = (GEN)g[i];
    for (j=2; j<k; j++) gk[j] = idealmul(nf, gk[j-1], gk[1]);
    pows[i] = gk; /* powers of g[i] */
  }

  C = cgetg(lc+1, t_VECSMALL); C[1] = c[lc]; 
  for (i=2; i<=lc; i++) C[i] = C[i-1] * c[lc-i+1];
  /* C[i] = c(k-i+1) * ... * ck */
  /* j < C[i+1] <==> j only involves g(k-i)...gk */
  i = 1; list[1] = 0; /* dummy */
  for (j=1; j < C[1]; j++)
    list[j + 1] = pows[lc][j];
  for (   ; j<no; j++)
  {
    GEN p1,p2;
    if (j == C[i+1]) i++;
    p2 = pows[lc-i][j/C[i]]; 
    p1 = list[j%C[i] + 1]; if (p1) p2 = idealmul(nf,p2,p1);
    list[j + 1] = p2;
  }
  list[1] = idealhermite(nf,gun);
  return (GEN)list;
}

/* x quadratic integer (approximate), recognize it. If error return NULL */
static GEN
findbezk(GEN nf, GEN x)
{
  GEN a,b, M = gmael(nf,5,1), y = cgetg(3, t_COL), u = gcoeff(M,1,2);
  long ea,eb;

  b = grndtoi(gdiv(gimag(x), gimag(u)), &eb);
  a = grndtoi(greal(gsub(x, gmul(b,u))),&ea);
  if (ea>-20 || eb>-20) return NULL;
  if (!signe(b)) return a;
  y[1] = (long)a;
  y[2] = (long)b; return basistoalg(nf,y);
}

static GEN
findbezk_pol(GEN nf, GEN x)
{
  long i, lx = lgef(x);
  GEN y = cgetg(lx,t_POL);
  for (i=2; i<lx; i++)
    if (! (y[i] = (long)findbezk(nf,(GEN)x[i])) ) return NULL;
  y[1] = x[1]; return y;
}

GEN
form_to_ideal(GEN x)
{
  long tx = typ(x);
  GEN b,c, y = cgetg(3, t_MAT);
  if (tx != t_QFR && tx != t_QFI) err(typeer,"form_to_ideal");
  c = cgetg(3,t_COL); y[1] = (long)c;
  c[1] = x[1]; c[2] = zero;
  c = cgetg(3,t_COL); y[2] = (long)c;
  b = negi((GEN)x[2]);
  if (mpodd(b)) b = addis(b,1);
  c[1] = lshifti(b,-1); c[2] = un; return y;
}

/* P as output by quadhilbertimag, convert forms to ideals */
static void
convert_to_id(GEN P)
{
  long i,l = lg(P);
  for (i=1; i<l; i++)
  {
    GEN p1 = (GEN)P[i];
    p1[1] = (long)form_to_ideal((GEN)p1[1]);
  }
}

/* P approximation computed at initial precision prec. Compute needed prec
 * to know P with 1 word worth of trailing decimals */
static long
get_prec(GEN P, long prec)
{
  long k = gprecision(P);
  if (k == 3) return (prec<<1)-2; /* approximation not trustworthy */
  k = prec - k; /* lost precision when computing P */
  if (k < 0) k = 0;
  k += MEDDEFAULTPREC + (gexpo(P) >> TWOPOTBITS_IN_LONG);
  if (k <= prec) k = (prec<<1)-2; /* dubious: old prec should have worked */
  return k;
}

/* AUXILLIARY ROUTINES FOR QUADRAYSIGMA */
GEN
PiI2(long prec)
{
  GEN z = cgetg(3,t_COMPLEX);
  GEN x = mppi(prec); setexpo(x,2);
  z[1] = zero;
  z[2] = (long)x; return z; /* = 2I Pi */
}

/* Compute data for ellphist */
static GEN
ellphistinit(GEN om, long prec)
{
  GEN p1,res,om1b,om2b, om1 = (GEN)om[1], om2 = (GEN)om[2];

  if (gsigne(gimag(gdiv(om1,om2))) < 0)
  {
    p1=om1; om1=om2; om2=p1;
    p1=cgetg(3,t_VEC); p1[1]=(long)om1; p1[2]=(long)om2;
    om = p1;
  }
  om1b = gconj(om1);
  om2b = gconj(om2); res = cgetg(4,t_VEC);
  res[1] = ldivgs(elleisnum(om,2,0,prec),12);
  res[2] = ldiv(PiI2(prec), gmul(om2, gimag(gmul(om1b,om2))));
  res[3] = (long)om2b; return res;
}

/* Computes log(phi^*(z,om)), using res computed by ellphistinit */
static GEN
ellphist(GEN om, GEN res, GEN z, long prec)
{
  GEN u = gimag(gmul(z, (GEN)res[3]));
  GEN zst = gsub(gmul(u, (GEN)res[2]), gmul(z,(GEN)res[1]));
  return gsub(ellsigma(om,z,1,prec),gmul2n(gmul(z,zst),-1));
}

/* Computes phi^*(la,om)/phi^*(1,om) where om is an oriented basis of the
   ideal gf*gc^{-1} */
static GEN
computeth2(GEN nf, GEN gf, GEN gc, GEN la, long prec)
{
  GEN p1,p2,omdiv,res;

  omdiv = get_om(nf, idealdiv(nf,gf,gc));
  res = ellphistinit(omdiv,prec);
  p1 = gsub(ellphist(omdiv,res,la,prec), ellphist(omdiv,res,gun,prec));
  p2 = gimag(p1);
  if (gexpo(greal(p1))>20 || gexpo(p2)> bit_accuracy(min(prec,lg(p2)))-10)
    return NULL;
  return gexp(p1,prec);
}

/* Computes P_2(X)=polynomial in Z_K[X] closest to prod_gc(X-th2(gc)) where
   the product is over the ray class group bnr.*/
static GEN
computeP2(GEN bnr, GEN la, int raw, long prec)
{
  long av=avma, av2, clrayno,i, first = 1;
  GEN listray,P0,P,f,lanum, nf = checknf(bnr);
  
  f = gmael3(bnr,2,1,1);
  if (typ(la) != t_COL) la = algtobasis(nf,la);
  listray = getallelts(nf,(GEN)bnr[5]);
  clrayno = lg(listray)-1; av2 = avma;
PRECPB:
  if (!first)
  {
    if (DEBUGLEVEL) err(warnprec,"computeP2",prec);
    nf = gerepileupto(av2, nfnewprec(checknf(bnr),prec));
  }
  first = 0; lanum = to_approx(nf,la);
  P = cgetg(clrayno+1,t_VEC);
  for (i=1; i<=clrayno; i++)
  {
    GEN v, s = computeth2(nf,f, (GEN)listray[i],lanum,prec);
    if (!s) { prec = (prec<<1)-2; goto PRECPB; }
    if (raw)
    {
      v = cgetg(3,t_VEC); P[i] = (long)v;
      v[1] = (long)listray[i];
      v[2] = (long)s;
    }
    else P[i] = (long)s;
  }
  if (!raw)
  {
    P0 = roots_to_pol(P, 0);
    P = findbezk_pol(nf, P0);
    if (!P) { prec = get_prec(P0, prec); goto PRECPB; }
  }
  return gerepileupto(av, gcopy(P));
}

#define nexta(a) (a>0 ? -a : 1-a)
static GEN
do_compo(GEN x, GEN y)
{
  long a, ph = degree(y);
  GEN z;
  y = gmul(gpuigs(polx[0],ph), gsubst(y,0,gdiv(polx[MAXVARN],polx[0])));
  for  (a = 0;; a = nexta(a))
  {
    if (a) x = gsubst(x, 0, gaddsg(a, polx[0]));
    z = subres(x,y);
    z = gsubst(z, MAXVARN, polx[0]);
    if (issquarefree(z)) return z;
  }
}
#undef nexta

static GEN
galoisapplypol(GEN nf, GEN s, GEN x)
{
  long i, lx = lg(x);
  GEN y = cgetg(lx,t_POL);

  for (i=2; i<lx; i++) y[i] = (long)galoisapply(nf,s,(GEN)x[i]);
  y[1] = x[1]; return y;
}

/* x quadratic, write it as ua + v, u,v rational */
static GEN
findquad(GEN a, GEN x, GEN p)
{
  long tu,tv, av = avma;
  GEN u,v;
  if (typ(x) == t_POLMOD) x = (GEN)x[2];
  if (typ(a) == t_POLMOD) a = (GEN)a[2];
  u = poldivres(x, a, &v);
  u = simplify(u); tu = typ(u);
  v = simplify(v); tv = typ(v);
  if (!is_scalar_t(tu) || !is_scalar_t(tv))
    err(talker, "incorrect data in findquad");
  x = v;
  if (!gcmp0(u)) x = gadd(gmul(u, polx[varn(a)]), x);
  if (typ(x) == t_POL) x = gmodulcp(x,p);
  return gerepileupto(av, x);
}

static GEN
findquad_pol(GEN nf, GEN a, GEN x)
{
  long i, lx = lg(x);
  GEN p = (GEN)nf[1], y = cgetg(lx,t_POL);
  for (i=2; i<lx; i++) y[i] = (long)findquad(a, (GEN)x[i], p);
  y[1] = x[1]; return y;
}

static GEN
compocyclo(GEN nf, long m, long d, long prec)
{
  GEN sb,a,b,s,p1,p2,p3,res,polL,polLK,nfL, D = (GEN)nf[3];
  long ell,vx;

  p1 = quadhilbertimag(D, gzero);
  p2 = cyclo(m,0);
  if (d==1) return do_compo(p1,p2);

  ell = m&1 ? m : (m>>2);
  if (!cmpsi(-ell,D)) /* ell = |D| */
  {
    p2 = gcoeff(nffactor(nf,p2),1,1);
    return do_compo(p1,p2);
  }
  if (ell%4 == 3) ell = -ell;
  /* nf = K = Q(a), L = K(b) quadratic extension = Q(t) */
  polLK = quadpoly(stoi(ell)); /* relative polynomial */
  res = rnfequation2(nf, polLK);
  vx = varn(nf[1]);
  polL = gsubst((GEN)res[1],0,polx[vx]); /* = charpoly(t) */
  a = gsubst(lift((GEN)res[2]), 0,polx[vx]);
  b = gsub(polx[vx], gmul((GEN)res[3], a));
  nfL = initalg(polL,prec);
  p1 = gcoeff(nffactor(nfL,p1),1,1);
  p2 = gcoeff(nffactor(nfL,p2),1,1);
  p3 = do_compo(p1,p2); /* relative equation over L */
  /* compute non trivial s in Gal(L / K) */
  sb= gneg(gadd(b, truecoeff(polLK,1))); /* s(b) = Tr(b) - b */
  s = gadd(polx[vx], gsub(sb, b)); /* s(t) = t + s(b) - b */
  p3 = gmul(p3, galoisapplypol(nfL, s, p3));
  return findquad_pol(nf, a, p3);
}

/* I integral ideal in HNF. (x) = I, x small in Z ? */
static long
isZ(GEN I)
{
  GEN x = gcoeff(I,1,1);
  if (signe(gcoeff(I,1,2)) || !egalii(x, gcoeff(I,2,2))) return 0;
  return is_bigint(x)? -1: itos(x);
}

/* Treat special cases directly. return NULL if not special case */
static GEN
treatspecialsigma(GEN nf, GEN gf, int raw, long prec)
{
  GEN p1,p2,tryf, D = (GEN)nf[3];
  long Ds,fl,i;

  if (raw)
    err(talker,"special case in Schertz's theorem. Odd flag meaningless");
  i = isZ(gf);
  if (cmpis(D,-3)==0)
  {
    if (i == 4 || i == 5 || i == 7) return cyclo(i,0);
    if (cmpis(gcoeff(gf,1,1), 9) || cmpis(content(gf),3)) return NULL;
    p1 = (GEN)nfroots(nf,cyclo(3,0))[1]; /* f = P_3^3 */
    return gadd(gpowgs(polx[0],3), p1); /* x^3+j */
  }
  if (cmpis(D,-4)==0)
  {
    if (i == 3 || i == 5) return cyclo(i,0);
    if (i != 4) return NULL;
    p1 = (GEN)nfroots(nf,cyclo(4,0))[1];
    return gadd(gpowgs(polx[0],2), p1); /* x^2+i */
  }
  Ds = smodis(D,48);
  if (i)
  {
    if (i==2 && Ds%16== 8) return compocyclo(nf, 4,1,prec);
    if (i==3 && Ds% 3== 1) return compocyclo(nf, 3,1,prec);
    if (i==4 && Ds% 8== 1) return compocyclo(nf, 4,1,prec);
    if (i==6 && Ds   ==40) return compocyclo(nf,12,1,prec);
    return NULL;
  }

  p1 = gcoeff(gf,1,1);
  p2 = gcoeff(gf,2,2);
  if (gcmp1(p2)) { fl = 0; tryf = p1; }
  else {
    if (Ds % 16 != 8 || !egalii(content(gf),gdeux)) return NULL;
    fl = 1; tryf = shifti(p1,-1);
  }
  if (cmpis(tryf, 3) <= 0 || !gcmp0(resii(D, tryf)) || !isprime(tryf))
    return NULL;

  i = itos(tryf); if (fl) i *= 4;
  return compocyclo(nf,i,2,prec);
}

/* Compute ray class field polynomial using sigma; if raw=1, pairs
   [ideals,roots] are given instead so that congruence subgroups can be used */

static GEN
quadrayimagsigma(GEN bnr, int raw, long prec)
{
  GEN allf,bnf,nf,pol,w,f,la,P,labas,gfi,Ci,Cj,Cj2,D,nfun;
  long a,b,f2;

  allf = conductor(bnr,gzero,2,prec); bnr = (GEN)allf[2];
  f = gmael(allf,1,1);
  bnf= (GEN)bnr[1];
  nf = (GEN)bnf[7];
  pol= (GEN)nf[1];
  D  = (GEN)nf[3];
  if (gcmp1(dethnf_i(f)))
  {
    P = quadhilbertimag(D, stoi(raw));
    if (raw) convert_to_id(P);
    return gcopy(P);
  }
  P = treatspecialsigma(nf,f,raw,prec);
  if (P) return P;
  w = gmodulcp(polx[varn(pol)],pol);
  f2 = itos(gmul2n(gcoeff(f,1,1),1));
  gfi = invmat(f);
  if (cmpis(D,-4)) Ci = NULL;
  else
  {
    P = nfroots(nf,cyclo(4,0));
    Ci= algtobasis(nf, (GEN)P[1]); /* should be I */
  }
  if (cmpis(D,-3)) Cj = Cj2 = NULL;
  else
  {
    P  = nfroots(nf,cyclo(3,0));
    Cj = algtobasis(nf, (GEN)P[1]);
    Cj2= algtobasis(nf, (GEN)P[2]); /* should be j, j^2 */
  }
  nfun = algtobasis(nf,gun);
  for (a=0; a<f2; a++)
  {
    for (b=0; b<f2; b++)
    {
      if (DEBUGLEVEL>1) fprintferr("[%ld,%ld] ",a,b);
      la = gaddgs(gmulsg(a,w),b);
      if (smodis(gnorm(la), f2) != 1) continue;

      labas = algtobasis(nf, la);
      if (gcmp1(denom(gmul(gfi,gadd(labas,nfun))))
       || gcmp1(denom(gmul(gfi,gsub(labas,nfun))))) continue;
      if (Ci)
      {
        if (gcmp1(denom(gmul(gfi,gadd(labas,Ci))))
         || gcmp1(denom(gmul(gfi,gsub(labas,Ci))))) continue;	
      }
      else if (Cj)
      {
        if (gcmp1(denom(gmul(gfi,gadd(labas,Cj ))))
         || gcmp1(denom(gmul(gfi,gsub(labas,Cj ))))
         || gcmp1(denom(gmul(gfi,gadd(labas,Cj2))))
         || gcmp1(denom(gmul(gfi,gsub(labas,Cj2))))) continue;	
      }
      if (DEBUGLEVEL)
      {
        if (DEBUGLEVEL>1) fprintferr("\n");
        fprintferr("lambda = %Z\n",la);
      }
      return computeP2(bnr,labas,raw,prec);
    }
  }
  err(talker,"bug in quadrayimagsigma, please report");
  return NULL;
}

GEN
quadray(GEN D, GEN f, GEN flag, long prec)
{
  GEN bnr,y,p1,pol,bnf,lambda;
  long av = avma, raw;

  if (!flag) flag = gzero;
  if (typ(flag)==t_INT) lambda = NULL;
  else
  {
    if (typ(flag)!=t_VEC || lg(flag)!=3) err(flagerr,"quadray");
    lambda = (GEN)flag[1]; flag = (GEN)flag[2];
    if (typ(flag)!=t_INT) err(flagerr,"quadray");
  }
  if (typ(D)!=t_INT)
  {
    bnf = checkbnf(D);
    if (degree(gmael(bnf,7,1))!=2)
      err(talker,"not a polynomial of degree 2 in quadray");
    D=gmael(bnf,7,3);
  }
  else
  {
    if (!isfundamental(D))
      err(talker,"quadray needs a fundamental discriminant");
    pol = quadpoly(D); setvarn(pol, fetch_user_var("y"));
    bnf = bnfinit0(pol,signe(D)>0?1:0,NULL,prec);
  }
  raw = (mpodd(flag) && signe(D) < 0);
  bnr = bnrinit0(bnf,f,1,prec);
  if (gcmp1(gmael(bnr,5,1)))
  {
    avma = av; if (!raw) return polx[0];
    y = cgetg(2,t_VEC); p1 = cgetg(3,t_VEC); y[1] = (long)p1;
    p1[1]=(long)idmat(2);
    p1[2]=(long)polx[0]; return y;
  }
  if (signe(D) > 0)
    y = bnrstark(bnr,gzero, gcmp0(flag)?1:5, prec);
  else
  {
    if (lambda)
      y = computeP2(bnr,lambda,raw,prec);
    else
      y = quadrayimagsigma(bnr,raw,prec);
  }
  return gerepileupto(av, y);
}

/*******************************************************************/
/*                                                                 */
/*  Routines related to binary quadratic forms (for internal use)  */
/*                                                                 */
/*******************************************************************/
extern void comp_gen(GEN z,GEN x,GEN y);
extern GEN codeform5(GEN x, long prec);
extern GEN comprealform5(GEN x, GEN y, GEN D, GEN sqrtD, GEN isqrtD);
extern GEN redrealform5(GEN x, GEN D, GEN sqrtD, GEN isqrtD);
extern GEN rhoreal_aux(GEN x, GEN D, GEN sqrtD, GEN isqrtD);

#define rhorealform(x) rhoreal_aux(x,Disc,sqrtD,isqrtD)
#define redrealform(x) gcopy(fix_signs(redrealform5(x,Disc,sqrtD,isqrtD)))
#define comprealform(x,y) fix_signs(comprealform5(x,y,Disc,sqrtD,isqrtD))

/* compute rho^n(x) */
static GEN 
rhoreal_pow(GEN x, long n)
{
  long i, av = avma, lim = stack_lim(av,1);
  for (i=1; i<=n; i++)
  {
    x = rhorealform(x);
    if (low_stack(lim, stack_lim(av,1)))
    {
      if(DEBUGMEM>1) err(warnmem,"rhoreal_pow");	
      x = gerepileupto(av, gcopy(x));
    }
  }
  return gerepileupto(av, gcopy(x));
}

static GEN
fix_signs(GEN x)
{
  GEN a = (GEN)x[1];
  GEN c = (GEN)x[3];
  if (signe(a) < 0)
  {
    if (narrow || absi_equal(a,c)) return rhorealform(x);
    setsigne(a,1); setsigne(c,-1);
  }
  return x;
}

static GEN
redrealprimeform5(GEN Disc, long p)
{
  long av = avma;
  GEN y = primeform(Disc,stoi(p),PRECREG);
  y = codeform5(y,PRECREG);
  return gerepileupto(av, redrealform(y));
}

static GEN
redrealprimeform(GEN Disc, long p)
{
  long av = avma;
  GEN y = primeform(Disc,stoi(p),PRECREG);
  return gerepileupto(av, redrealform(y));
}

static GEN
comprealform3(GEN x, GEN y)
{
  long av = avma;
  GEN z = cgetg(4,t_VEC); comp_gen(z,x,y);
  return gerepileupto(av, redrealform(z));
}

static GEN
initrealform5(long *ex)
{
  GEN form = powsubfactorbase[1][ex[1]];
  long i;

  for (i=2; i<=lgsub; i++)
    form = comprealform(form, powsubfactorbase[i][ex[i]]);
  return form;
}

/*******************************************************************/
/*                                                                 */
/*                     Common subroutines                          */
/*                                                                 */
/*******************************************************************/
static void
buch_init(void)
{
  if (DEBUGLEVEL) timer2();
  primfact  = new_chunk(100);
  primfact1 = new_chunk(100);
  exprimfact  = new_chunk(100);
  exprimfact1 = new_chunk(100);
  badprim = new_chunk(100);
  hashtab = (long**) new_chunk(HASHT);
}

double
check_bach(double cbach, double B)
{
  if (cbach >= B)
   err(talker,"sorry, buchxxx couldn't deal with this field PLEASE REPORT!");
  cbach *= 2; if (cbach > B) cbach = B;
  if (DEBUGLEVEL) fprintferr("\n*** Bach constant: %f\n", cbach);
  return cbach;
}

static long
factorisequad(GEN f, long kcz, long limp)
{
  long i,p,k,av,lo;
  GEN q,r, x = (GEN)f[1];

  if (is_pm1(x)) { primfact[0]=0; return 1; }
  av=avma; lo=0;
  if (signe(x) < 0) x = absi(x);
  for (i=1; ; i++)
  {
    p=factorbase[i]; q=dvmdis(x,p,&r);
    if (!signe(r))
    {
      k=0; while (!signe(r)) { x=q; k++; q=dvmdis(x,p,&r); }
      lo++; primfact[lo]=p; exprimfact[lo]=k;
    }
    if (cmpis(q,p)<=0) break;
    if (i==kcz) { avma=av; return 0; }
  }
  p = x[2]; avma=av;
  /* p = itos(x) if lgefint(x)=3 */
  if (lgefint(x)!=3 || p > limhash) return 0;

  if (p != 1 && p <= limp)
  {
    for (i=1; i<=badprim[0]; i++)
      if (p % badprim[i] == 0) return 0;
    lo++; primfact[lo]=p; exprimfact[lo]=1;
    p = 1;
  }
  primfact[0]=lo; return p;
}

/* q may not be prime, but check for a "large prime relation" involving q */
static long *
largeprime(long q, long *ex, long np, long nrho)
{
  const long hashv = ((q & (2 * HASHT - 1)) - 1) >> 1;
  long *pt, i;

 /* If q = 0 (2048), very slim chance of getting a relation.
  * And hashtab[-1] is undefined anyway */
  if (hashv < 0) return NULL;
  for (pt = hashtab[hashv]; ; pt = (long*) pt[0])
  {
    if (!pt)
    {
      pt = (long*) gpmalloc((lgsub+4)<<TWOPOTBYTES_IN_LONG);
      *pt++ = nrho; /* nrho = pt[-3] */
      *pt++ = np;   /* np   = pt[-2] */
      *pt++ = q;    /* q    = pt[-1] */
      pt[0] = (long)hashtab[hashv];
      for (i=1; i<=lgsub; i++) pt[i]=ex[i];
      hashtab[hashv]=pt; return NULL;
    }
    if (pt[-1] == q) break;
  }
  for(i=1; i<=lgsub; i++)
    if (pt[i] != ex[i]) return pt;
  return (pt[-2]==np)? (GEN)NULL: pt;
}

static long
badmod8(GEN x)
{
  long r = mod8(x);
  if (!r) return 1;
  if (signe(Disc) < 0) r = 8-r;
  return (r < 4);
}

/* cree factorbase, numfactorbase, vectbase; affecte badprim */
static void
factorbasequad(GEN Disc, long n2, long n)
{
  long i,p,bad, av = avma;
  byteptr d=diffptr;

  numfactorbase = (long*) gpmalloc(sizeof(long)*(n2+1));
  factorbase    = (long*) gpmalloc(sizeof(long)*(n2+1));
  KC=0; bad=0; i=0; p = *d++;
  while (p<=n2)
  {
    switch(krogs(Disc,p))
    {
      case -1: break; /* inert */
      case  0: /* ramified */
      {
        GEN p1 = divis(Disc,p);
	if (smodis(p1,p) == 0)
          if (p!=2 || badmod8(p1)) { badprim[++bad]=p; break; }
        i++; numfactorbase[p]=i; factorbase[i] = -p; break;
      }
      default:  /* split */
        i++; numfactorbase[p]=i; factorbase[i] = p;
    }
    p += *d++; if (!*d) err(primer1);
    if (KC == 0 && p>n) KC = i;
  }
  if (!KC) { free(factorbase); free(numfactorbase); return; }
  KC2 = i;
  vectbase = (long*) gpmalloc(sizeof(long)*(KC2+1));
  for (i=1; i<=KC2; i++)
  {
    p = factorbase[i];
    vectbase[i]=p; factorbase[i]=labs(p);
  }
  if (DEBUGLEVEL)
  {
    msgtimer("factor base");
    if (DEBUGLEVEL>7)
    {
      fprintferr("factorbase:\n");
      for (i=1; i<=KC; i++) fprintferr("%ld ",factorbase[i]);
      fprintferr("\n"); flusherr();
    }
  }
  avma=av; badprim[0] = bad;
}

/* cree vectbase and subfactorbase. Affecte lgsub */
static long
subfactorbasequad(double ll, long KC)
{
  long i,j,k,nbidp,p,pro[100], ss;
  GEN y;
  double prod;

  i=0; ss=0; prod=1;
  for (j=1; j<=KC && prod<=ll; j++)
  {
    p = vectbase[j];
    if (p>0) { pro[++i]=p; prod*=p; vperm[i]=j; } else ss++;
  }
  if (prod<=ll) return -1;
  nbidp=i;
  for (k=1; k<j; k++)
    if (vectbase[k]<=0) vperm[++i]=k;

  y=cgetg(nbidp+1,t_COL);
  if (PRECREG) /* real */
    for (j=1; j<=nbidp; j++)
      y[j] = (long)redrealprimeform5(Disc, pro[j]);
  else
    for (j=1; j<=nbidp; j++) /* imaginary */
      y[j] = (long)primeform(Disc,stoi(pro[j]),0);
  subbase = (long*) gpmalloc(sizeof(long)*(nbidp+1));
  lgsub = nbidp; for (j=1; j<=lgsub; j++) subbase[j]=pro[j];
  if (DEBUGLEVEL>7)
  {
    fprintferr("subfactorbase: ");
    for (i=1; i<=lgsub; i++)
      { fprintferr("%ld: ",subbase[i]); outerr((GEN)y[i]); }
    fprintferr("\n"); flusherr();
  }
  subfactorbase = y; return ss;
}

static void
powsubfact(long n, long a)
{
  GEN unform, *y, **x = (GEN**) gpmalloc(sizeof(GEN*)*(n+1));
  long i,j;

  for (i=1; i<=n; i++)
    x[i] = (GEN*) gpmalloc(sizeof(GEN)*(a+1));
  if (PRECREG) /* real */
  {
    unform=cgetg(6,t_VEC);
    unform[1]=un;
    unform[2]=(mod2(Disc) == mod2(isqrtD))? (long)isqrtD: laddsi(-1,isqrtD);
    unform[3]=lshifti(subii(sqri((GEN)unform[2]),Disc),-2);
    unform[4]=zero;
    unform[5]=(long)realun(PRECREG);
    for (i=1; i<=n; i++)
    {
      y = x[i]; y[0] = unform;
      for (j=1; j<=a; j++)
	y[j] = comprealform(y[j-1],(GEN)subfactorbase[i]);
    }
  }
  else /* imaginary */
  {
    unform = primeform(Disc, gun, 0);
    for (i=1; i<=n; i++)
    {
      y = x[i]; y[0] = unform;
      for (j=1; j<=a; j++)
	y[j] = compimag(y[j-1],(GEN)subfactorbase[i]);
    }
  }
  if (DEBUGLEVEL) msgtimer("powsubfact");
  powsubfactorbase = x;
}

static void
desalloc(long **mat)
{
  long i,*p,*q;

  free(vectbase); free(factorbase); free(numfactorbase);
  if (mat)
  {
    free(subbase);
    for (i=1; i<lg(subfactorbase); i++) free(powsubfactorbase[i]);
    for (i=1; i<lg(mat); i++) free(mat[i]);
    free(mat); free(powsubfactorbase);
    for (i=1; i<HASHT; i++)
      for (p = hashtab[i]; p; p = q) { q=(long*)p[0]; free(p-3); }
  }
}

/* L-function */
static GEN
lfunc(GEN Disc)
{
  long av=avma, p;
  GEN y=realun(DEFAULTPREC);
  byteptr d=diffptr;

  for(p = *d++; p<=30000; p += *d++)
  {
    if (!*d) err(primer1);
    y = mulsr(p, divrs(y, p-krogs(Disc,p)));
  }
  return gerepileupto(av,y);
}

#define comp(x,y) x? (PRECREG? compreal(x,y): compimag(x,y)): y
static GEN
get_clgp(GEN Disc, GEN W, GEN *ptmet, long prec)
{
  GEN p1,p2,res,*init, u1u2=smith2(W), u1=(GEN)u1u2[1], met=(GEN)u1u2[3];
  long c,i,j, l = lg(met);

  u1 = reducemodmatrix(ginv(u1), W);
  for (c=1; c<l; c++)
    if (gcmp1(gcoeff(met,c,c))) break;
  if (DEBUGLEVEL) msgtimer("smith/class group");
  res=cgetg(c,t_VEC); init = (GEN*)cgetg(l,t_VEC);
  for (i=1; i<l; i++)
    init[i] = primeform(Disc,stoi(labs(vectbase[vperm[i]])),prec);
  for (j=1; j<c; j++)
  {
    p1 = NULL;
    for (i=1; i<l; i++)
    {
      p2 = gpui(init[i], gcoeff(u1,i,j), prec);
      p1 = comp(p1,p2);
    }
    res[j] = (long)p1;
  }
  if (DEBUGLEVEL) msgtimer("generators");
  *ptmet = met; return res;
}

static GEN
extra_relations(long LIMC, long *ex, long nlze, GEN extramatc)
{
  long av,fpc,p,ep,i,j,k,nlze2, *col, *colg, s = 0, extrarel = nlze+2;
  long MAXRELSUP = min(50,4*KC);
  GEN p1,form, extramat = cgetg(extrarel+1,t_MAT);

  if (DEBUGLEVEL)
  {
    fprintferr("looking for %ld extra relations\n",extrarel);
    flusherr();
  }
  for (j=1; j<=extrarel; j++) extramat[j]=lgetg(KC+1,t_COL);
  nlze2 = PRECREG? max(nlze,lgsub): min(nlze+1,KC);
  if (nlze2 < 3 && KC > 2) nlze2 = 3;
  av = avma;
  while (s<extrarel)
  {
    form = NULL;
    for (i=1; i<=nlze2; i++)
    {
      ex[i]=mymyrand()>>randshift;
      if (ex[i])
      {
        p1 = primeform(Disc,stoi(factorbase[vperm[i]]),PRECREG);
        p1 = gpuigs(p1,ex[i]); form = comp(form,p1);
      }
    }
    if (!form) continue;

    fpc = factorisequad(form,KC,LIMC);
    if (fpc==1)
    {
      s++; col = (GEN)extramat[s];
      for (i=1; i<=nlze2; i++) col[vperm[i]] = -ex[i];
      for (   ; i<=KC; i++) col[vperm[i]]= 0;
      for (j=1; j<=primfact[0]; j++)
      {
        p=primfact[j]; ep=exprimfact[j];
        if (smodis((GEN)form[2], p<<1) > p) ep = -ep;
        col[numfactorbase[p]] += ep;
      }
      for (i=1; i<=KC; i++)
        if (col[i]) break;
      if (i>KC)
      {
        s--; avma = av;
        if (--MAXRELSUP == 0) return NULL;
      }
      else { av = avma; if (PRECREG) coeff(extramatc,1,s) = form[4]; }
    }
    else avma = av;
    if (DEBUGLEVEL)
    {
      if (fpc == 1) fprintferr(" %ld",s);
      else if (DEBUGLEVEL>1) fprintferr(".");
      flusherr();
    }
  }
  for (j=1; j<=extrarel; j++)
  {
    colg = cgetg(KC+1,t_COL);
    col = (GEN)extramat[j]; extramat[j] = (long) colg;
    for (k=1; k<=KC; k++)
      colg[k] = lstoi(col[vperm[k]]);
  }
  if (DEBUGLEVEL)
  {
    fprintferr("\n");
    msgtimer("extra relations");
  }
  return extramat;
}
#undef comp

/*******************************************************************/
/*                                                                 */
/*                    Imaginary Quadratic fields                   */
/*                                                                 */
/*******************************************************************/

static GEN
imag_random_form(long current, long *ex)
{
  long av = avma,i;
  GEN form,pc;

  for(;;)
  {
    form = pc = primeform(Disc,stoi(factorbase[current]),PRECREG);
    for (i=1; i<=lgsub; i++)
    {
      ex[i] = mymyrand()>>randshift;
      if (ex[i])
        form = compimag(form,powsubfactorbase[i][ex[i]]);
    }
    if (form != pc) return form;
    avma = av; /* ex = 0, try again */
  }
}

static void
imag_relations(long lim, long s, long LIMC, long *ex, long **mat)
{
  static long nbtest;
  long av = avma, i,j,pp,fpc,b1,b2,ep,current, first = (s==0);
  long *col,*fpd,*oldfact,*oldexp;
  GEN pc,form,form1;

  if (first) nbtest = 0 ;
  while (s<lim)
  {
    avma=av; nbtest++; current = first? 1+(s%KC): 1+s-RELSUP;
    form = imag_random_form(current,ex);
    fpc = factorisequad(form,KC,LIMC);
    if (!fpc)
    {
      if (DEBUGLEVEL>1) { fprintferr("."); flusherr(); }
      continue;
    }
    if (fpc > 1)
    {
      fpd = largeprime(fpc,ex,current,0);
      if (!fpd)
      {
        if (DEBUGLEVEL>1) { fprintferr("."); flusherr(); }
        continue;
      }
      form1 = powsubfactorbase[1][fpd[1]];
      for (i=2; i<=lgsub; i++)
        form1 = compimag(form1,powsubfactorbase[i][fpd[i]]);
      pc=primeform(Disc,stoi(factorbase[fpd[-2]]),0);
      form1=compimag(form1,pc);
      pp = fpc << 1;
      b1=smodis((GEN)form1[2], pp);
      b2=smodis((GEN)form[2],  pp);
      if (b1 != b2 && b1+b2 != pp) continue;

      s++; col = mat[s];
      if (DEBUGLEVEL) { fprintferr(" %ld",s); flusherr(); }
      oldfact = primfact; oldexp = exprimfact;
      primfact = primfact1; exprimfact = exprimfact1;
      factorisequad(form1,KC,LIMC);

      if (b1==b2)
      {
        for (i=1; i<=lgsub; i++)
          col[numfactorbase[subbase[i]]] = fpd[i]-ex[i];
        col[fpd[-2]]++;
        for (j=1; j<=primfact[0]; j++)
        {
          pp=primfact[j]; ep=exprimfact[j];
          if (smodis((GEN)form1[2], pp<<1) > pp) ep = -ep;
          col[numfactorbase[pp]] -= ep;
        }
      }
      else
      {
        for (i=1; i<=lgsub; i++)
          col[numfactorbase[subbase[i]]] = -fpd[i]-ex[i];
        col[fpd[-2]]--;
        for (j=1; j<=primfact[0]; j++)
        {
          pp=primfact[j]; ep=exprimfact[j];
          if (smodis((GEN)form1[2], pp<<1) > pp) ep = -ep;
          col[numfactorbase[pp]] += ep;
        }
      }
      primfact = oldfact; exprimfact = oldexp;
    }	
    else
    {
      s++; col = mat[s];
      if (DEBUGLEVEL) { fprintferr(" %ld",s); flusherr(); }
      for (i=1; i<=lgsub; i++)
        col[numfactorbase[subbase[i]]] = -ex[i];
    }
    for (j=1; j<=primfact[0]; j++)
    {
      pp=primfact[j]; ep=exprimfact[j];
      if (smodis((GEN)form[2], pp<<1) > pp) ep = -ep;
      col[numfactorbase[pp]] += ep;
    }
    col[current]--;
    if (!first && fpc == 1 && col[current] == 0)
    {
      s--; for (i=1; i<=KC; i++) col[i]=0;
    }
  }
  if (DEBUGLEVEL)
  {
    fprintferr("\nnbrelations/nbtest = %ld/%ld\n",s,nbtest);
    msgtimer("%s relations", first? "initial": "random");
  }
}

static int
imag_be_honest(long *ex)
{
  long p,fpc, s = KC, nbtest = 0, av = avma;
  GEN form;

  while (s<KC2)
  {
    p = factorbase[s+1];
    if (DEBUGLEVEL) { fprintferr(" %ld",p); flusherr(); }
    form = imag_random_form(s+1,ex);
    fpc = factorisequad(form,s,p-1);
    if (fpc == 1) { nbtest=0; s++; }
    else
      if (++nbtest > 20) return 0;
    avma = av;
  }
  return 1;
}

/*******************************************************************/
/*                                                                 */
/*                      Real Quadratic fields                      */
/*                                                                 */
/*******************************************************************/

static GEN
real_random_form(long *ex)
{
  long av = avma, i;
  GEN p1,form = NULL;

  for(;;)
  {
    for (i=1; i<=lgsub; i++)
    {
      ex[i] = mymyrand()>>randshift;
/*    if (ex[i]) KB: less efficient if I put this in. Why ??? */
      {
        p1 = powsubfactorbase[i][ex[i]];
        form = form? comprealform3(form,p1): p1;
      }
    }
    if (form) return form;
    avma = av;
  }
}

static void
real_relations(long lim, long s, long LIMC, long *ex, long **mat, GEN glog2,
               GEN vecexpo)
{
  static long nbtest;
  long av = avma,av1,i,j,p,fpc,b1,b2,ep,current, first = (s==0);
  long *col,*fpd,*oldfact,*oldexp,limstack;
  long findecycle,nbrhocumule,nbrho;
  GEN p1,p2,form,form0,form1,form2;

  limstack=stack_lim(av,1);
  if (first) nbtest = 0;
  current = 0;
  p1 = NULL; /* gcc -Wall */
  while (s<lim)
  {
    form = real_random_form(ex);
    if (!first)
    {
      current = 1+s-RELSUP;
      p1 = redrealprimeform(Disc, factorbase[current]);
      form = comprealform3(form,p1);
    }
    form0 = form; form1 = NULL;
    findecycle = nbrhocumule = 0;
    nbrho = -1; av1 = avma;
    while (s<lim)
    {
      if (low_stack(limstack, stack_lim(av,1)))
      {
        GEN *gptr[2];
        int c = 1;
	if(DEBUGMEM>1) err(warnmem,"real_relations");	
        gptr[0]=&form; if (form1) gptr[c++]=&form1;
        gerepilemany(av1,gptr,c);
      }
      if (nbrho < 0) nbrho = 0; /* first time in */
      else
      {
        if (findecycle) break;
        form = rhorealform(form);
        nbrho++; nbrhocumule++;
        if (first)
        {
          if (absi_equal((GEN)form[1],(GEN)form0[1])
               && egalii((GEN)form[2],(GEN)form0[2])
               && (!narrow || signe(form0[1])==signe(form[1]))) findecycle=1;
        }
        else
        {
          if (narrow)
            { form=rhorealform(form); nbrho++; }
          else if (absi_equal((GEN)form[1], (GEN)form[3])) /* a = -c */
          {
            if (absi_equal((GEN)form[1],(GEN)form0[1]) &&
                    egalii((GEN)form[2],(GEN)form0[2])) break;
            form=rhorealform(form); nbrho++;
          }
          else
            { setsigne(form[1],1); setsigne(form[3],-1); }
          if (egalii((GEN)form[1],(GEN)form0[1]) &&
              egalii((GEN)form[2],(GEN)form0[2])) break;
        }
      }
      nbtest++; fpc = factorisequad(form,KC,LIMC);
      if (!fpc)
      {
        if (DEBUGLEVEL>1) { fprintferr("."); flusherr(); }
        continue;
      }
      if (fpc > 1)
      {
	fpd = largeprime(fpc,ex,current,nbrhocumule);
        if (!fpd)
        {
          if (DEBUGLEVEL>1) { fprintferr("."); flusherr(); }
          continue;
        }
        if (!form1) form1 = initrealform5(ex);
        if (!first)
        {
          p1 = redrealprimeform5(Disc, factorbase[current]);
          form1=comprealform(form1,p1);
        }
        form1 = rhoreal_pow(form1, nbrho); nbrho = 0;
        form2 = initrealform5(fpd);
        if (fpd[-2])
        {
          p1 = redrealprimeform5(Disc, factorbase[fpd[-2]]);
          form2=comprealform(form2,p1);
        }
        form2 = rhoreal_pow(form2, fpd[-3]);
        if (!narrow && !absi_equal((GEN)form2[1],(GEN)form2[3]))
        {
          setsigne(form2[1],1);
          setsigne(form2[3],-1);
        }
        p = fpc << 1;
        b1=smodis((GEN)form2[2], p);
        b2=smodis((GEN)form1[2], p);
        if (b1 != b2 && b1+b2 != p) continue;

        s++; col = mat[s]; if (DEBUGLEVEL) fprintferr(" %ld",s);
        oldfact = primfact; oldexp = exprimfact;
        primfact = primfact1; exprimfact = exprimfact1;
        factorisequad(form2,KC,LIMC);
        if (b1==b2)
        {
          for (i=1; i<=lgsub; i++)
            col[numfactorbase[subbase[i]]] = fpd[i]-ex[i];
          for (j=1; j<=primfact[0]; j++)
          {
            p=primfact[j]; ep=exprimfact[j];
            if (smodis((GEN)form2[2], p<<1) > p) ep = -ep;
            col[numfactorbase[p]] -= ep;
          }
          if (fpd[-2]) col[fpd[-2]]++; /* implies !first */
          p1 = subii((GEN)form1[4],(GEN)form2[4]);
          p2 = divrr((GEN)form1[5],(GEN)form2[5]);
        }
        else
        {
          for (i=1; i<=lgsub; i++)
            col[numfactorbase[subbase[i]]] = -fpd[i]-ex[i];
          for (j=1; j<=primfact[0]; j++)
          {
            p=primfact[j]; ep=exprimfact[j];
            if (smodis((GEN)form2[2], p<<1) > p) ep = -ep;
            col[numfactorbase[p]] += ep;
          }
          if (fpd[-2]) col[fpd[-2]]--;
          p1 = addii((GEN)form1[4],(GEN)form2[4]);
          p2 = mulrr((GEN)form1[5],(GEN)form2[5]);
        }
        primfact = oldfact; exprimfact = oldexp;
      }
      else
      {
	if (!form1) form1 = initrealform5(ex);
        if (!first)
        {
          p1 = redrealprimeform5(Disc, factorbase[current]);
          form1=comprealform(form1,p1);
        }
        form1 = rhoreal_pow(form1,nbrho); nbrho = 0;

	s++; col = mat[s]; if (DEBUGLEVEL) fprintferr(" %ld",s);
	for (i=1; i<=lgsub; i++)
          col[numfactorbase[subbase[i]]] = -ex[i];
        p1 = (GEN) form1[4];
        p2 = (GEN) form1[5];
      }
      for (j=1; j<=primfact[0]; j++)
      {
        p=primfact[j]; ep=exprimfact[j];
        if (smodis((GEN)form1[2], p<<1) > p) ep = -ep;
        col[numfactorbase[p]] += ep;
      }
      p1 = mpadd(mulir(mulsi(EXP220,p1), glog2), mplog(absr(p2)));
      affrr(shiftr(p1,-1), (GEN)vecexpo[s]);
      if (!first)
      {
        col[current]--;
        if (fpc == 1 && col[current] == 0)
          { s--; for (i=1; i<=KC; i++) col[i]=0; }
        break;
      }
    }
    avma = av;
  }
  if (DEBUGLEVEL)
  {
    fprintferr("\nnbrelations/nbtest = %ld/%ld\n",s,nbtest);
    msgtimer("%s relations", first? "initial": "random");
  }
}

static int
real_be_honest(long *ex)
{
  long p,fpc,s = KC,nbtest = 0,av = avma;
  GEN p1,form,form0;

  while (s<KC2)
  {
    p = factorbase[s+1];
    if (DEBUGLEVEL) { fprintferr(" %ld",p); flusherr(); }
    form = real_random_form(ex);
    p1 = redrealprimeform(Disc, p);
    form0 = form = comprealform3(form,p1);
    for(;;)
    {
      fpc = factorisequad(form,s,p-1);
      if (fpc == 1) { nbtest=0; s++; break; }
      form = rhorealform(form);
      if (++nbtest > 20) return 0;
      if (narrow || absi_equal((GEN)form[1],(GEN)form[3]))
	form = rhorealform(form);
      else
      {
	setsigne(form[1],1);
	setsigne(form[3],-1);
      }
      if (egalii((GEN)form[1],(GEN)form0[1])
       && egalii((GEN)form[2],(GEN)form0[2])) break;
    }
    avma = av;
  }
  return 1;
}

static GEN
gcdrealnoer(GEN a,GEN b,long *pte)
{
  long e;
  GEN k1,r;

  if (typ(a)==t_INT)
  {
    if (typ(b)==t_INT) return mppgcd(a,b);
    k1=cgetr(lg(b)); affir(a,k1); a=k1;
  }
  else if (typ(b)==t_INT)
    { k1=cgetr(lg(a)); affir(b,k1); b=k1; }
  if (expo(a)<-5) return absr(b);
  if (expo(b)<-5) return absr(a);
  a=absr(a); b=absr(b);
  while (expo(b) >= -5  && signe(b))
  {
    k1=gcvtoi(divrr(a,b),&e);
    if (e > 0) return NULL;
    r=subrr(a,mulir(k1,b)); a=b; b=r;
  }
  *pte=expo(b); return absr(a);
}

static GEN
get_reg(GEN matc, long sreg)
{
  long i,e,maxe;
  GEN reg = mpabs(gcoeff(matc,1,1));

  e = maxe = 0;
  for (i=2; i<=sreg; i++)
  {
    reg = gcdrealnoer(gcoeff(matc,1,i),reg,&e);
    if (!reg) return NULL;
    maxe = maxe? max(maxe,e): e;
  }
  if (DEBUGLEVEL)
  {
    if (DEBUGLEVEL>7) { fprintferr("reg = "); outerr(reg); }
    msgtimer("regulator");
  }
  return reg;
}

GEN
buchquad(GEN D, double cbach, double cbach2, long RELSUP0, long flag, long prec)
{
  long av0 = avma,av,tetpil,KCCO,KCCOPRO,i,j,s, *ex,**mat;
  long extrarel,nrelsup,nreldep,LIMC,LIMC2,cp,nbram,nlze;
  GEN p1,h,W,met,res,basecl,dr,c_1,pdep,C,B,extramat,extraC;
  GEN reg,vecexpo,glog2,cst;
  double drc,lim,LOGD;

  Disc = D; if (typ(Disc)!=t_INT) err(typeer,"buchquad");
  s=mod4(Disc);
  glog2 = vecexpo = NULL; /* gcc -Wall */
  switch(signe(Disc))
  {
    case -1:
      if (cmpis(Disc,-4) >= 0)
      {
        p1=cgetg(6,t_VEC); p1[1]=p1[4]=p1[5]=un;
        p1[2]=p1[3]=lgetg(1,t_VEC); return p1;
      }
      if (s==2 || s==1) err(funder2,"buchquad");
      PRECREG=0; break;

    case 1:
      if (s==2 || s==3) err(funder2,"buchquad");
      if (flag)
        err(talker,"sorry, narrow class group not implemented. Use bnfnarrow");
      PRECREG=1; break;

    default: err(talker,"zero discriminant in quadclassunit");
  }
  if (carreparfait(Disc)) err(talker,"square argument in quadclassunit");
  if (!isfundamental(Disc))
    err(warner,"not a fundamental discriminant in quadclassunit");
  buch_init(); RELSUP = RELSUP0;
  dr=cgetr(3); affir(Disc,dr); drc=fabs(rtodbl(dr)); LOGD=log(drc);
  lim=sqrt(drc); cst = mulrr(lfunc(Disc), dbltor(lim));
  if (!PRECREG) lim /= sqrt(3.);
  cp = (long)exp(sqrt(LOGD*log(LOGD)/8.0));
  if (cp < 13) cp = 13;
  av = avma; cbach /= 2;

INCREASE: avma = av; cbach = check_bach(cbach,6.);
  nreldep = nrelsup = 0;
  LIMC = (long)(cbach*LOGD*LOGD);
  if (LIMC < cp) LIMC=cp;
  LIMC2 = max(20, (long)(max(cbach,cbach2)*LOGD*LOGD));
  if (LIMC2 < LIMC) LIMC2 = LIMC;
  if (PRECREG)
  {
    PRECREG = max(prec+1, MEDDEFAULTPREC + 2*(expi(Disc)>>TWOPOTBITS_IN_LONG));
    glog2  = glog(gdeux,PRECREG);
    sqrtD  = gsqrt(Disc,PRECREG);
    isqrtD = gfloor(sqrtD);
  }
  factorbasequad(Disc,LIMC2,LIMC);
  if (!KC) goto INCREASE;

  vperm = cgetg(KC+1,t_VECSMALL); for (i=1; i<=KC; i++) vperm[i]=i;
  nbram = subfactorbasequad(lim,KC);
  if (nbram == -1) { desalloc(NULL); goto INCREASE; }
  KCCO = KC + RELSUP;
  if (DEBUGLEVEL) { fprintferr("KC = %ld, KCCO = %ld\n",KC,KCCO); flusherr(); }
  powsubfact(lgsub,CBUCH+7);

  mat = (long**) gpmalloc((KCCO+1)*sizeof(long*));
  setlg(mat, KCCO+1);
  for (i=1; i<=KCCO; i++)
  {
    mat[i] = (long*) gpmalloc((KC+1)*sizeof(long));
    for (j=1; j<=KC; j++) mat[i][j]=0;
  }
  ex = new_chunk(lgsub+1);
  limhash = (LIMC<(MAXHALFULONG>>1))? LIMC*LIMC: HIGHBIT>>1;
  for (i=0; i<HASHT; i++) hashtab[i]=NULL;

  s = lgsub+nbram+RELSUP;
  if (PRECREG)
  {
    vecexpo=cgetg(KCCO+1,t_VEC);
    for (i=1; i<=KCCO; i++) vecexpo[i]=lgetr(PRECREG);
    real_relations(s,0,LIMC,ex,mat,glog2,vecexpo);
    real_relations(KCCO,s,LIMC,ex,mat,glog2,vecexpo);
  }
  else
  {
    imag_relations(s,0,LIMC,ex,mat);
    imag_relations(KCCO,s,LIMC,ex,mat);
  }
  if (KC2 > KC)
  {
    if (DEBUGLEVEL)
      fprintferr("be honest for primes from %ld to %ld\n",
                  factorbase[KC+1],factorbase[KC2]);
    s = PRECREG? real_be_honest(ex): imag_be_honest(ex);
    if (DEBUGLEVEL)
    {
      fprintferr("\n");
      msgtimer("be honest");
    }
    if (!s) { desalloc(mat); goto INCREASE; }
  }
  C=cgetg(KCCO+1,t_MAT);
  if (PRECREG)
  {
    for (i=1; i<=KCCO; i++)
    {
      C[i]=lgetg(2,t_COL); coeff(C,1,i)=vecexpo[i];
    }
    if (DEBUGLEVEL>7) { fprintferr("C: "); outerr(C); flusherr(); }
  }
  else
    for (i=1; i<=KCCO; i++) C[i]=lgetg(1,t_COL);
  W = hnfspec(mat,vperm,&pdep,&B,&C,lgsub);
  nlze = lg(pdep)>1? lg(pdep[1])-1: lg(B[1])-1;

  KCCOPRO=KCCO;
  if (nlze)
  {
EXTRAREL:
    s = PRECREG? 2: 1; extrarel=nlze+2;
    extraC=cgetg(extrarel+1,t_MAT);
    for (i=1; i<=extrarel; i++) extraC[i]=lgetg(s,t_COL);
    extramat = extra_relations(LIMC,ex,nlze,extraC);
    if (!extramat) { desalloc(mat); goto INCREASE; }
    W = hnfadd(W,vperm,&pdep,&B,&C, extramat,extraC);
    nlze = lg(pdep)>1? lg(pdep[1])-1: lg(B[1])-1;
    KCCOPRO += extrarel;
    if (nlze)
    {
      if (++nreldep > 5) { desalloc(mat); goto INCREASE; }
      goto EXTRAREL;
    }
  }
  /* tentative class number */
  h=gun; for (i=1; i<lg(W); i++) h=mulii(h,gcoeff(W,i,i));
  if (PRECREG)
  {
    /* tentative regulator */
    reg = get_reg(C, KCCOPRO - (lg(B)-1) - (lg(W)-1));
    if (!reg)
    {
      desalloc(mat);
      prec = (PRECREG<<1)-2; goto INCREASE;
    }
    if (gexpo(reg)<=-3)
    {
      if (++nrelsup <= 7)
      {
        if (DEBUGLEVEL) { fprintferr("regulateur nul\n"); flusherr(); }
        nlze=min(KC,nrelsup); goto EXTRAREL;
      }
      desalloc(mat); goto INCREASE;
    }
    c_1 = divrr(gmul2n(gmul(h,reg),1), cst);
  }
  else
  {
    reg = gun;
    c_1 = divrr(gmul(h,mppi(DEFAULTPREC)), cst);
  }

  if (gcmpgs(gmul2n(c_1,2),3)<0) { c_1=stoi(10); nrelsup=7; }
  if (gcmpgs(gmul2n(c_1,1),3)>0)
  {
    nrelsup++;
    if (nrelsup<=7)
    {
      if (DEBUGLEVEL)
        { fprintferr("***** check = %f\n\n",gtodouble(c_1)); flusherr(); }
      nlze=min(KC,nrelsup); goto EXTRAREL;
    }
    if (cbach < 5.99) { desalloc(mat); goto INCREASE; }
    err(warner,"suspicious check. Suggest increasing extra relations.");
  }
  basecl = get_clgp(Disc,W,&met,PRECREG);
  s = lg(basecl); desalloc(mat); tetpil=avma;

  res=cgetg(6,t_VEC);
  res[1]=lcopy(h); p1=cgetg(s,t_VEC);
  for (i=1; i<s; i++) p1[i] = (long)icopy(gcoeff(met,i,i));
  res[2]=(long)p1;
  res[3]=lcopy(basecl);
  res[4]=lcopy(reg);
  res[5]=lcopy(c_1); return gerepile(av0,tetpil,res);
}

GEN
buchimag(GEN D, GEN c, GEN c2, GEN REL)
{
  return buchquad(D,gtodouble(c),gtodouble(c2),itos(REL), 0,0);
}

GEN
buchreal(GEN D, GEN sens0, GEN c, GEN c2, GEN REL, long prec)
{
  return buchquad(D,gtodouble(c),gtodouble(c2),itos(REL), itos(sens0),prec);
}
