/* $Id: subgroup.c,v 1.6 2000/11/03 21:00:23 karim Exp $

Copyright (C) 2000  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

#include "pari.h"
extern GEN hnf0(GEN x, long r);
void push_val(entree *ep, GEN a);
void pop_val(entree *ep);

/* SUBGROUPS
 * Assume: G = Gp x I, with Gp a p-group and (|I|,p)=1, and I small.
 * Compute subgroups of I by recursive calls
 * Loop through subgroups Hp of Gp using Birkhoff's algorithm.
 * If (I is non trivial)
 *   lift Hp to G (mul by exponent of I)
 *   for each subgp of I, lift it to G (mult by exponent of Gp)
 *   consider the group generated by the two subgroups (concat)
 */
static long *powerlist, *mmu, *lam, *c, *maxc, *a, *maxa, **g, **maxg;
static GEN **H, subq, subqpart, hnfgroup;
static GEN BINMAT;
static long countsub, expoI;
static long *available, indexbound, lsubq, lsubqpart;
static char *gpch;
static entree *ep;
static void(*treatsub_fun)(GEN);
typedef struct slist {
  struct slist *next;
  long *data;
} slist;

static slist *sublist;

void 
printtyp(long *typ)
{
  long i;
  for (i=1; i<=typ[0]; i++) fprintferr(" %ld ",typ[i]);
  fprintferr("\n");
}

/* compute conjugate partition of typ */
static long*
conjugate(long *typ)
{
  long *t, i, k = typ[0], l, last;

  if (!k) { t = new_chunk(1); t[0]=0; return t; }
  l = typ[1]; t = new_chunk(l+2);
  t[1] = k; last = k;
  for (i=2; i<=l; i++)
  {
    while (typ[last] < i) last--;
    t[i] = last;
  }
  t[i] = 0; t[0] = l;
  return t;
}

static void
std_fun(GEN x)
{
  ep->value = (void*)x;
  lisseq(gpch); countsub++;
}

void
addcell(GEN H)
{
  long *pt,i,j, k = 0, n = lg(H)-1;
  slist *cell = (slist*) gpmalloc(sizeof(slist) + n*(n+1)/2 * sizeof(long));

  sublist->next = cell; cell->data = pt = (long*) (cell + 1);
  for (j=1; j<=n; j++)
    for(i=1; i<=j; i++) pt[k++] = itos(gcoeff(H,i,j));
  sublist = cell;
}

static void
list_fun(GEN x)
{
  addcell(hnf(concatsp(hnfgroup,x))); countsub++;
}

/* treat subgroup Hp (not in HNF, treatsub_fun should do it if desired) */
static void
treatsub(GEN Hp)
{
  long i;
  if (!subq) treatsub_fun(Hp);
  else
  { /* not a p group, add the trivial part */
    Hp = gmulsg(expoI,Hp); /* lift Hp to G */
    for (i=1; i<lsubqpart; i++)
      treatsub_fun(concatsp(Hp, (GEN)subqpart[i]));
  }
}

/* assume t>0 and l>1 */
static void
dogroup(void)
{
  long av = avma,av1, e,i,j,k,r,n,t2,ind, t = mmu[0], l = lam[0];

  t2 = (l==t)? t-1: t;
  n = t2 * l - (t2*(t2+1))/2; /* number of gamma_ij */
  for (i=1, r=t+1; ; i++)
  {
    if (available[i]) c[r++] = i;
    if (r > l) break;
  }
  if (DEBUGLEVEL>2) { fprintferr("    column selection:"); printtyp(c); }
  /* a/g and maxa/maxg access the same data indexed differently */
  for (ind=0,i=1; i<=t; ind+=(l-i),i++)
  {
    maxg[i] = maxa + (ind - (i+1)); /* only access maxg[i][i+1..l] */
    g[i] = a + (ind - (i+1));
    for (r=i+1; r<=l; r++)
      if (c[r] < c[i])             maxg[i][r] = powerlist[mmu[i]-mmu[r]-1];
      else if (lam[c[r]] < mmu[i]) maxg[i][r] = powerlist[lam[c[r]]-mmu[r]];
      else                         maxg[i][r] = powerlist[mmu[i]-mmu[r]];
  }
  av1=avma; a[n-1]=0; for (i=0; i<n-1; i++) a[i]=1; 
  for(;;)
  {
    a[n-1]++;
    if (a[n-1] > maxa[n-1])
    {
      j=n-2; while (j>=0 && a[j]==maxa[j]) j--;
      if (j<0) { avma=av; return; }
      a[j]++; for (k=j+1; k<n; k++) a[k]=1;
    }
    for (i=1; i<=t; i++)
    {
      for (r=1; r<i; r++) affsi(0, H[i][c[r]]);
      affsi(powerlist[lam[c[r]]-mmu[r]], H[r][c[r]]);
      for (r=i+1; r<=l; r++)
      {
        if (c[r] < c[i]) 
          e = g[i][r]*powerlist[lam[c[r]]-mmu[i]+1];
        else
          if (lam[c[r]] < mmu[i]) e = g[i][r];
          else e = g[i][r]*powerlist[lam[c[r]]-mmu[i]];
        affsi(e, H[i][c[r]]);
      }
    }
    treatsub((GEN)H); avma=av1;
  }
}

/* c[1],...,c[r-1] filled */
static void
loop(long r)
{
  long j;

  if (r > mmu[0]) { dogroup(); return; }

  if (r!=1 && (mmu[r-1] == mmu[r])) j = c[r-1]+1; else j = 1;
  for (  ; j<=maxc[r]; j++)
    if (available[j])
    {
      c[r] = j;  available[j] = 0;
      loop(r+1); available[j] = 1;
    }
}

static void
dopsubtyp(void)
{
  long av = avma, i,r, l = lam[0], t = mmu[0]; 

  if (!t)
  {
    GEN p1 = cgetg(2,t_MAT);
    p1[1] = (long)zerocol(l); 
    treatsub(p1); avma=av; return;
  }
  if (l==1) /* imply t = 1 */
  {
    GEN p1 = gtomat(stoi(powerlist[lam[1]-mmu[1]]));
    treatsub(p1); avma=av; return;
  }
  c = new_chunk(l+1); c[0] = l;
  maxc = new_chunk(l+1);
  available = new_chunk(l+1);
  a  = new_chunk(l*(t+1));
  maxa=new_chunk(l*(t+1));
  g = (long**)new_chunk(t+1);
  maxg = (long**)new_chunk(t+1);

  if (DEBUGLEVEL) { fprintferr("  subgroup:"); printtyp(mmu); }
  for (i=1; i<=t; i++)
  {
    for (r=1; r<=l; r++)
      if (mmu[i] > lam[r]) break;
    maxc[i] = r-1;
  }
  H = (GEN**)cgetg(t+1, t_MAT);
  for (i=1; i<=t; i++)
  {
    H[i] = (GEN*)cgetg(l+1, t_COL);
    for (r=1; r<=l; r++) H[i][r] = cgeti(3);
  }
  for (i=1; i<=l; i++) available[i]=1;
  for (i=1; i<=t; i++) c[i]=0;
  /* go through all column selections */
  loop(1); avma=av; return;
}

static long
weight(long *typ)
{
  long i,w = 0;
  for (i=1; i<=typ[0]; i++) w += typ[i];
  return w;
}

static long
dopsub(long p, long *gtyp, long *indexsubq)
{
  long w,i,j,k,n, wg = 0, wmin = 0, count = 0;

  if (DEBUGLEVEL) { fprintferr("\ngroup:"); printtyp(gtyp); }
  if (indexbound)
  {
    wg = weight(gtyp);
    wmin = (long) (wg - (log((double)indexbound) / log((double)p)));
    if (cmpis(gpuigs(stoi(p), wg - wmin), indexbound) > 0) wmin++;
  }
  lam = gtyp; n = lam[0]; mmu = new_chunk(n+1);
  mmu[1] = -1; for (i=2; i<=n; i++) mmu[i]=0;
  for(;;) /* go through all vectors mu_{i+1} <= mu_i <= lam_i */
  {
    mmu[1]++;
    if (mmu[1] > lam[1])
    {
      for (j=2; j<=n; j++)
        if (mmu[j] < lam[j] && mmu[j] < mmu[j-1]) break;
      if (j>n) return count;
      mmu[j]++; for (k=1; k<j; k++) mmu[k]=mmu[j];
    }
    for (j=1; j<=n; j++)
      if (!mmu[j]) break;
    mmu[0] = j-1; w = weight(mmu);
    if (w >= wmin)
    { 
      GEN p1 = gun;

      if (subq) /* G not a p-group */
      {
        if (indexbound)
        {
          long indexH = itos(gpuigs(stoi(p), wg - w));
          long bound = indexbound / indexH;
          subqpart = cgetg(lsubq, t_VEC);
          lsubqpart = 1;
          for (i=1; i<lsubq; i++)
            if (indexsubq[i] <= bound) subqpart[lsubqpart++] = subq[i];
        }
        else { subqpart = subq; lsubqpart = lsubq; }
      }
      if (DEBUGLEVEL)
      {
        long *lp = conjugate(lam);
        long *mp = conjugate(mmu);

        if (DEBUGLEVEL > 3)
        {
          fprintferr("    lambda = "); printtyp(lam);
          fprintferr("    lambda'= "); printtyp(lp);
          fprintferr("    mu = "); printtyp(mmu);
          fprintferr("    mu'= "); printtyp(mp);
        }
        for (j=1; j<=mp[0]; j++)
        {
          p1 = mulii(p1, gpuigs(stoi(p), mp[j+1]*(lp[j]-mp[j])));
          p1 = mulii(p1, gcoeff(BINMAT, lp[j]-mp[j+1]+1, mp[j]-mp[j+1]+1));
        }
        fprintferr("  alpha_lambda(mu,p) = %Z\n",p1);
      }
      countsub = 0;
      dopsubtyp();
      count += countsub;
      if (DEBUGLEVEL)
      {
        fprintferr("  countsub = %ld\n", countsub);
        msgtimer("for this type");
        if (subq) p1 = mulis(p1,lsubqpart-1);
        if (cmpis(p1,countsub))
        {
          fprintferr("  alpha = %Z\n",p1);
          err(bugparier,"forsubgroup (alpha != countsub)");
        }
      }
    }
  }
}

static GEN
expand_sub(GEN x, long n)
{
  long i,j, m = lg(x);
  GEN p = idmat(n-1), q,c;

  for (i=1; i<m; i++)
  {
    q = (GEN)p[i]; c = (GEN)x[i];
    for (j=1; j<m; j++) q[j] = c[j];
    for (   ; j<n; j++) q[j] = zero;
  }
  return p;
}

extern GEN matqpascal(long n, GEN q);

static long
subgroup_engine(GEN cyc, long bound)
{
  long av=avma,i,j,k,imax,nbprim,count, n = lg(cyc);
  GEN gtyp,fa,junk,primlist,p,listgtyp,indexsubq = NULL;
  long oindexbound = indexbound;
  long oexpoI      = expoI;
  long *opowerlist = powerlist;
  GEN osubq        = subq;
  GEN oBINMAT      = BINMAT;
  long olsubq      = lsubq;

  long *ommu=mmu, *olam=lam, *oc=c, *omaxc=maxc, *oa=a, *omaxa=maxa, **og=g, **omaxg=maxg;
  GEN **oH=H, osubqpart=subqpart;
  long ocountsub=countsub;
  long *oavailable=available, olsubqpart=lsubqpart;
  
  if (typ(cyc) != t_VEC) 
  {
    if (typ(cyc) != t_MAT) err(typeer,"forsubgroup");
    cyc = mattodiagonal(cyc);
  }
  for (i=1; i<n-1; i++)
    if (!divise((GEN)cyc[i], (GEN)cyc[i+1]))
      err(talker,"not a group in forsubgroup");
  if (n == 1 || gcmp1((GEN)cyc[1])) { treatsub(cyc); return 1; }
  if (!signe(cyc[1]))
    err(talker,"infinite group in forsubgroup");
  if (DEBUGLEVEL) timer2();
  indexbound = bound;
  fa = factor((GEN)cyc[1]); primlist = (GEN)fa[1];
  nbprim = lg(primlist);
  listgtyp = new_chunk(n); imax = k = 0;
  for (i=1; i<nbprim; i++)
  {
    gtyp = new_chunk(n); p = (GEN)primlist[i];
    for (j=1; j<n; j++)
    {
      gtyp[j] = pvaluation((GEN)cyc[j], p, &junk);
      if (!gtyp[j]) break;
    }
    j--; gtyp[0] = j;
    if (j > k) { k = j; imax = i; }
    listgtyp[i] = (long)gtyp;
  }
  gtyp = (GEN)listgtyp[imax]; p = (GEN)primlist[imax];
  k = gtyp[1];
  powerlist = new_chunk(k+1); powerlist[0] = 1;
  powerlist[1] = itos(p);
  for (j=1; j<=k; j++) powerlist[j] = powerlist[1] * powerlist[j-1];

  if (DEBUGLEVEL) BINMAT = matqpascal(gtyp[0]+1, p);
  if (nbprim == 2) subq = NULL;
  else
  { /* not a p-group */
    GEN cyc2 = dummycopy(cyc);
    GEN ohnfgroup = hnfgroup;
    for (i=1; i<n; i++)
    {
      cyc2[i] = ldivis((GEN)cyc2[i], powerlist[gtyp[i]]);
      if (gcmp1((GEN)cyc2[i])) break;
    }
    setlg(cyc2, i);
    if (is_bigint(cyc[1]))
      err(impl,"subgrouplist for large cyclic factors");
    expoI = itos((GEN)cyc2[1]);
    hnfgroup = diagonal(cyc2);
    subq = subgrouplist(cyc2, bound);
    hnfgroup = ohnfgroup;
    lsubq = lg(subq);
    for (i=1; i<lsubq; i++) 
      subq[i] = (long)expand_sub((GEN)subq[i], n);
    if (indexbound)
    {
      indexsubq = new_chunk(lsubq);
      for (i=1; i<lsubq; i++)
        indexsubq[i] = itos(dethnf((GEN)subq[i]));
    }
    /* lift subgroups of I to G */
    for (i=1; i<lsubq; i++) 
      subq[i] = lmulsg(powerlist[k],(GEN)subq[i]);
    if (DEBUGLEVEL>2)
    {
      fprintferr("(lifted) subgp of prime to %Z part:\n",p);
      outbeaut(subq);
    }
  }
  count = dopsub(powerlist[1],gtyp,indexsubq); 
  if (DEBUGLEVEL) fprintferr("nb subgroup = %ld\n",count);

  mmu=ommu; lam=olam; c=oc; maxc=omaxc; a=oa; maxa=omaxa; g=og; maxg=omaxg;
  H=oH; subqpart=osubqpart,
  countsub=ocountsub;
  available=oavailable; lsubqpart=olsubqpart;

  indexbound = oindexbound;
  expoI    = oexpoI;
  powerlist = opowerlist;
  subq      = osubq;
  BINMAT    = oBINMAT;
  lsubq     = olsubq;
  avma=av; return count;
}

void
forsubgroup(entree *oep, GEN cyc, long bound, char *och)
{
  entree *saveep = ep;
  char *savech = gpch;
  void(*savefun)(GEN) = treatsub_fun;
  long n, N = lg(cyc)-1;

  treatsub_fun = std_fun;
  cyc = dummycopy(cyc);
  for (n = N; n > 1; n--) /* take care of trailing 1s */
    if (!gcmp1((GEN)cyc[n])) break;
  setlg(cyc, n+1);
  gpch = och;
  ep = oep;
  push_val(ep, gzero);
  (void)subgroup_engine(cyc,bound);
  pop_val(ep);
  treatsub_fun = savefun;
  gpch = savech;
  ep = saveep;
}

GEN
subgrouplist(GEN cyc, long bound)
{
  void(*savefun)(GEN) = treatsub_fun;
  slist *olist = sublist, *list; 
  long ii,i,j,k,nbsub,n, N = lg(cyc)-1, av = avma;
  GEN z,H;
  GEN ohnfgroup = hnfgroup;

  sublist = list = (slist*) gpmalloc(sizeof(slist));
  treatsub_fun = list_fun;
  cyc = dummycopy(cyc);
  for (n = N; n > 1; n--) /* take care of trailing 1s */
    if (!gcmp1((GEN)cyc[n])) break;
  setlg(cyc, n+1);
  hnfgroup = diagonal(cyc);
  nbsub = subgroup_engine(cyc,bound);
  hnfgroup = ohnfgroup; avma = av;
  z = cgetg(nbsub+1,t_VEC); sublist = list;
  for (ii=1; ii<=nbsub; ii++)
  {
    list = sublist; sublist = list->next; free(list);
    H = cgetg(N+1,t_MAT); z[ii]=(long)H; k=0;
    for (j=1; j<=n; j++)
    {
      H[j] = lgetg(N+1, t_COL);
      for (i=1; i<=j; i++) coeff(H,i,j) = lstoi(sublist->data[k++]);
      for (   ; i<=N; i++) coeff(H,i,j) = zero;
    }
    for (   ; j<=N; j++)
    {
      H[j] = lgetg(N+1, t_COL);
      for (i=1; i<=N; i++) coeff(H,i,j) = (i==j)? un: zero;
    }
  }
  free(sublist); sublist = olist;
  treatsub_fun = savefun; return z;
}
