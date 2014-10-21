/* $Id: rect.h,v 1.8 2000/11/03 21:00:24 karim Exp $

Copyright (C) 2000  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

#define PLOT_NAME_LEN 20
#define NUMRECT 18

#define DTOL(t) ((long)(t + 0.5))

typedef struct PARI_plot {
  long width;
  long height;
  long hunit;
  long vunit;
  long fwidth;
  long fheight;
  long init;
  char name[PLOT_NAME_LEN+1];
} PARI_plot;

extern PARI_plot pari_plot, pari_psplot;

#define w_height (pari_plot.height)
#define w_width (pari_plot.width)
#define f_height (pari_plot.fheight)
#define f_width (pari_plot.fwidth)
#define h_unit (pari_plot.hunit)
#define v_unit (pari_plot.vunit)
#define lmargin (f_width*10)
#define rmargin (h_unit - 1)
#define tmargin (v_unit - 1)
#define bmargin (v_unit + f_height - 1)

typedef struct dblPointList{
  double *d;                   /* data */
  long nb;                     /* number of elements */
  double xsml,xbig,ysml,ybig;  /* extrema */
} dblPointList;

typedef struct RectObj {
  struct RectObj *next;
  long code,color;
} RectObj;

typedef struct PariRect {
  RectObj *head,*tail;
  long sizex,sizey;
  double cursorx,cursory;
  double xscale,yscale;
  double xshift,yshift;
  long has_graph;	   /* xy-ranges of this rectangle should be used
			      for interactive operations.  */
} PariRect;

/* The structures below are "subclasses" of RectObj. */

typedef struct RectObj1P {
  struct RectObj *next;
  long code,color;
  double x,y;
} RectObj1P;

typedef struct RectObj2P {
  struct RectObj *next;
  long code,color;
  double x1,y1;
  double x2,y2;
} RectObj2P;

typedef struct RectObjMP {
  struct RectObj *next;
  long code,color;
  long count;
  double *xs,*ys;
} RectObjMP;

typedef struct RectObjST {
  struct RectObj *next;
  long code,color;
  long length;
  char *s;
  double x,y;
  long dir;
} RectObjST;

typedef struct RectObjPN {
  struct RectObj *next;
  long code,color;
  long pen;
} RectObjPN;

typedef struct RectObjPS {
  struct RectObj *next;
  long code,color;
  double size;
} RectObjPS;

#define BLACK      1 /* Default */
#define BLUE       2 /* Axes */
#define SIENNA     3 /* Odd numbered curves in ploth */
#define RED        4 /* Curves, or Even numbered curves in ploth */
#define CORNSILK   5
#define GREY       6
#define GAINSBORO  7

#define MAX_COLORS 8
#define DEFAULT_COLOR BLACK

#define ROt_MV 0			/* Move */
#define ROt_PT 1			/* Point */
#define ROt_LN 2			/* Line */
#define ROt_BX 3			/* Box */
#define ROt_MP 4			/* Multiple point */
#define ROt_ML 5			/* Multiple lines */
#define ROt_ST 6			/* String */
#define ROt_PTT 7			/* Point type change */
#define ROt_LNT 8			/* Line type change */
#define ROt_PTS 9			/* Point size change */
#define ROt_NULL 10		/* To be the start of the chain */

#define ROt_MAX 10		/* Maximal type */

/* Pointer conversion. */

#define RoMV(rop) ((RectObj1P*)rop)
#define RoPT(rop) ((RectObj1P*)rop)
#define RoLN(rop) ((RectObj2P*)rop)
#define RoBX(rop) ((RectObj2P*)rop)
#define RoMP(rop) ((RectObjMP*)rop)
#define RoML(rop) ((RectObjMP*)rop)
#define RoST(rop) ((RectObjST*)rop)
#define RoPTT(rop) ((RectObjPN*)rop)
#define RoPTS(rop) ((RectObjPS*)rop)
#define RoLNT(rop) ((RectObjPN*)rop)
#define RoNULL(rop) ((RectObj*)rop)

/* All the access to the rectangle data _should_ go via these macros! */

#define RHead(rp) ((rp)->head)
#define RTail(rp) ((rp)->tail)
#define RXsize(rp) ((rp)->sizex)
#define RYsize(rp) ((rp)->sizey)
#define RXcursor(rp) ((rp)->cursorx)
#define RYcursor(rp) ((rp)->cursory)
#define RXshift(rp) ((rp)->xshift)
#define RYshift(rp) ((rp)->yshift)
#define RXscale(rp) ((rp)->xscale)
#define RYscale(rp) ((rp)->yscale)
#define RHasGraph(rp) ((rp)->has_graph)

#define RoNext(rop) ((rop)->next)
#define RoType(rop) ((rop)->code)
#define RoCol(rop) ((rop)->color)
#define RoMVx(rop) (RoMV(rop)->x)
#define RoMVy(rop) (RoMV(rop)->y)
#define RoPTx(rop) (RoPT(rop)->x)
#define RoPTy(rop) (RoPT(rop)->y)
#define RoLNx1(rop) (RoLN(rop)->x1)
#define RoLNy1(rop) (RoLN(rop)->y1)
#define RoLNx2(rop) (RoLN(rop)->x2)
#define RoLNy2(rop) (RoLN(rop)->y2)
#define RoBXx1(rop) (RoBX(rop)->x1)
#define RoBXy1(rop) (RoBX(rop)->y1)
#define RoBXx2(rop) (RoBX(rop)->x2)
#define RoBXy2(rop) (RoBX(rop)->y2)

#define RoMPcnt(rop) (RoMP(rop)->count)
#define RoMPxs(rop) (RoMP(rop)->xs)
#define RoMPys(rop) (RoMP(rop)->ys)

#define RoMLcnt(rop) (RoML(rop)->count)
#define RoMLxs(rop) (RoML(rop)->xs)
#define RoMLys(rop) (RoML(rop)->ys)

#define RoSTs(rop) (RoST(rop)->s)
#define RoSTl(rop) (RoST(rop)->length)
#define RoSTx(rop) (RoST(rop)->x)
#define RoSTy(rop) (RoST(rop)->y)
#define RoSTdir(rop) (RoST(rop)->dir)

#define RoSTdirLEFT	  0x00
#define RoSTdirCENTER	  0x01
#define RoSTdirRIGHT	  0x02
#define RoSTdirHPOS_mask  0x03

#define RoSTdirBOTTOM	  0x00
#define RoSTdirVCENTER	  0x04
#define RoSTdirTOP	  0x08
#define RoSTdirVPOS_mask  0x0c

#define RoSTdirHGAP	  0x10
#define RoSTdirVGAP	  0x20


#define RoPTTpen(rop) (RoPTT(rop)->pen)
#define RoLNTpen(rop) (RoLNT(rop)->pen)
#define RoPTSsize(rop) (RoPTS(rop)->size)

#define PL_POINTS 1
#define GOODRECT(r) (0 <= r && r < NUMRECT)
#define GOODCOLOR(c) (1 <= c && c < MAX_COLORS)

#define PLOT_PARAMETRIC   0x00001
#define PLOT_RECURSIVE    0x00002
#define PLOT_NO_RESCALE   0x00004
#define PLOT_NO_AXE_X     0x00008
#define PLOT_NO_AXE_Y     0x00010
#define PLOT_NO_FRAME     0x00020
#define PLOT_POINTS       0x00040
#define PLOT_POINTS_LINES 0x00080
#define PLOT_SPLINES      0x00100
#define PLOT_NO_TICK_X    0x00200
#define PLOT_NO_TICK_Y    0x00400
#define PLOT_NODOUBLETICK 0x00800

#define PLOT_POSTSCRIPT   0x80000

#define RECT_CP_RELATIVE  0x1
#define RECT_CP_NW        0x0
#define RECT_CP_SW        0x2
#define RECT_CP_SE        0x4
#define RECT_CP_NE        0x6

#define TICKS_CLOCKW	1		/* Draw in clockwise direction */
#define TICKS_ACLOCKW	2		/* Draw in anticlockwise direction */
#define TICKS_ENDSTOO	4		/* Draw at endspoints if needed */
#define TICKS_NODOUBLE	8		/* Do not draw double-length ticks */

/* Not implemented yet */
#define TICKS_COORD	16		/* Output [x,y,l,isdbl] for each tick */
#define TICKS_RELATIVE	32		/* x,y-coordinates are relative */

extern PariRect  **rectgraph;
extern long  rectpoint_itype;
extern long  rectline_itype;

/* plotport.c */

extern  void    initrect(long ne, long x, long y);
extern  void    initrect_gen(long ne, GEN x, GEN y, long flag);
extern  void    killrect(long ne);
extern  void    plot(entree *ep, GEN a, GEN b, char *ch, GEN ysmlu, GEN ybigu, long prec);
extern  GEN     ploth(entree *ep, GEN a, GEN b, char *ch, long prec, long flag, long numpoints);
extern  GEN     ploth2(entree *ep, GEN a, GEN b, char *ch, long prec);
extern  GEN     plothmult(entree *ep, GEN a, GEN b, char *ch, long prec);
extern  GEN     plothraw(GEN listx, GEN listy, long flag);
extern  GEN     plothsizes();
extern  GEN     plothsizes_flag(long flag);
extern  void    postdraw(GEN list);
extern  void    postdraw_flag(GEN list, long flag);
extern  GEN     postploth(entree *ep,GEN a,GEN b,char *ch,long prec,long flag,long numpoints);
extern  GEN     postploth2(entree *ep,GEN a,GEN b,char *ch,long prec,long numpoints);
extern  GEN     postplothraw(GEN listx, GEN listy, long flag);
extern  void    rectbox(long ne, GEN gx2, GEN gy2);
extern  void    rectcolor(long ne, long color);
extern  void    rectcopy(long source, long dest, long xoff, long yoff);
extern  void    rectcopy_gen(long source, long dest, GEN xoff, GEN yoff, long flag);
extern  GEN     rectcursor(long ne);
extern  void    rectdraw(GEN list);
extern  void    rectdraw_flag(GEN list, long flag);
extern  void    rectline(long ne, GEN gx2, GEN gy2);
extern  void    rectlines(long ne, GEN listx, GEN listy, long flag);
extern  void    rectlinetype(long ne, long t);
extern  void    rectmove(long ne, GEN x, GEN y);
extern  GEN     rectploth(long drawrect,entree *ep, GEN a, GEN b, char *ch, long prec, ulong flags, long testpoints);
extern  GEN     rectplothraw(long drawrect, GEN data, long flags);
extern  void    rectpoint(long ne, GEN x, GEN y);
extern  void    rectpoints(long ne, GEN listx, GEN listy);
extern  void    rectpointtype(long ne, long t);
extern  void	rectpointsize(long ne, GEN size);
extern  void    rectrbox(long ne, GEN gx2, GEN gy2);
extern  void    rectrline(long ne, GEN gx2, GEN gy2);
extern  void    rectrmove(long ne, GEN x, GEN y);
extern  void    rectrpoint(long ne, GEN x, GEN y);
extern  void    rectscale(long ne, GEN x1, GEN x2, GEN y1, GEN y2);
extern  void    rectstring(long ne, char *x);
extern  void    rectstring3(long ne, char *x, long dir);
extern  void    rectclip(long rect);

/* architecture-dependent plot file (plotX.c, plotsun.c, plognuplot.c...) */

extern  void    PARI_get_plot(long fatal);
extern  long    term_set(char *s);
extern  long    plot_outfile_set(char *s);
extern  void	set_pointsize(double d);
