/* $Id: anal.h,v 1.15.2.1 2005/09/15 14:55:22 bill Exp $

Copyright (C) 2000  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/*************************************************************************/
/*                                                                       */
/*                 Declarations specific to the analyzer                 */
/*                                                                       */
/*************************************************************************/

typedef struct default_type {
  char *name;
  void *fun;
} default_type;

typedef struct gp_args {
  int nloc, narg;
  GEN *arg;
} gp_args;

typedef struct module {
  entree *func;
  char **help;
} module;

typedef struct stack {
  struct stack *prev;
  void *value;
} stack;

extern void push_stack(stack **pts, void *a);
extern void *pop_stack(stack **pts);

entree *do_alias(entree *ep);
int    is_identifier(char *s);
entree *is_entry_intern(char *s, entree **table, long *hash);
long   is_keyword_char(char c);
char   *readstring(char *src, char *s);
long   loop_break();
long   did_break();
extern void   print_prefixed_text(char *s, char *prefix, char *str);

extern void term_color(int c);
extern char *term_get_color(int c);
extern void hit_return();

extern void push_val(entree *ep, GEN a);
extern void pop_val(entree *ep);

extern long prec, secure;
extern GEN gnil;

extern char *current_function;
extern GEN  (*gp_history_fun)(long, long, char *, char *);
extern int  (*whatnow_fun)(char *, int);
extern void (*output_fun)(GEN);
extern void *foreignHandler;
extern GEN  (*foreignExprHandler)(char*);
extern char foreignExprSwitch;
extern entree * (*foreignAutoload)(char*, long);
extern void (*foreignFuncFree)(entree *);
extern int (*default_exception_handler)(long);

/* Variables containing the list of PARI functions */
extern int    functions_tblsz;     /* hashcodes table size */
extern module *pari_modules;       /* list of functions modules */
extern entree **functions_hash;    /* functions hashtable */
extern entree **members_hash;      /* members hashtable */
extern char   *helpmessages_basic[];
extern entree functions_basic[];

/* Variables containing the list of specific GP functions */
extern char   *helpmessages_gp[];
extern entree  functions_gp[];
extern entree  gp_member_list[];
extern char   *helpmessages_highlevel[];
extern entree  functions_highlevel[];
extern int     gp_colors[];
extern int     disable_color,added_newline;

/* Variables containing the list of old PARI fonctions (up to 1.39.15) */
extern module *pari_oldmodules;    /* list of functions modules */
extern entree **funct_old_hash;    /* hashtable */
extern char   *oldhelpmessage[], *helpmessages_oldgp[];
extern entree  oldfonctions[], functions_oldgp[];

/* backward compatibility */
extern long compatible;
enum { NONE, WARN, OLDFUN, OLDALL };
#define new_fun_set (compatible == NONE || compatible == WARN)

/* return type for GP functions */
enum { RET_GEN, RET_LONG, RET_INT, RET_VOID };

/* emacs/texmacs interface */
extern int under_emacs;
extern int under_texmacs;

#ifdef STACK_CHECK
extern void *PARI_stack_limit;
#endif

/* entrees */
#define Epstatic 0x100
#define EpVALENCE(ep) ((ep)->valence & 0xFF)
#define EpSTATIC(ep) ((ep)->valence & 0x100)
#define EpSETSTATIC(ep) ((ep)->valence |= 0x100)
#define PARAMSHIFT 9
#define EpNPARAM(ep) ((ep)->valence >> PARAMSHIFT)
#define EpPREDEFINED(ep) (EpVALENCE(ep) < EpUSER)

#define EpINSTALL 200
#define EpMEMBER  105
#define EpGVAR    104
#define EpVAR     103
#define EpALIAS   102
#define EpNEW     101
#define EpUSER    100

#define NOT_CREATED_YET ((entree *)0x1L) /* for check_new_fun */
#define initial_value(ep) ((ep)+1)

/* blocs */
#define BL_HEAD 3
#define bl_base(x) ((x) - BL_HEAD)
#define bl_next(x) (((GEN)x)[-3])
#define bl_prev(x) (((GEN)x)[-2])
#define bl_num(x)  (((GEN)x)[-1])

/* break */
enum { br_NONE, br_BREAK, br_NEXT, br_MULTINEXT, br_RETURN }; /* break status */

/* formatted printing */
#ifndef LONG_IS_64BIT
#  define VOIR_STRING1 "[&=%08lx] "
#  define VOIR_STRING2 "%08lx  "
#  define VOIR_STRING3 "  %08lx  :  "
#else
#  define VOIR_STRING1 "[&=%08x%08x] "
#  define VOIR_STRING2 "%08x%08x  "
#  define VOIR_STRING3 "  %08x%08x  :  "
#endif

/* signals */
#define INIT_JMPm 1
#define INIT_SIGm 2
#define INIT_JMP     (init_opts & INIT_JMPm)
#define INIT_SIG     (init_opts & INIT_SIGm)
#define INIT_JMP_on  (init_opts |= INIT_JMPm)
#define INIT_SIG_on  (init_opts |= INIT_SIGm)
#define INIT_JMP_off (init_opts &= ~INIT_JMPm)
#define INIT_SIG_off (init_opts &= ~INIT_SIGm)

/* gp_colors */
extern void decode_color(int n, int *c);
#define c_NONE 0xffffUL
enum { c_ERR, c_HIST, c_PROMPT, c_INPUT, c_OUTPUT, c_HELP, c_TIME, c_LAST };

/* general printing */
#define print_text(s) print_prefixed_text((s),NULL,NULL);

/* infiles */
#define MAX_BUFFER 64
#define mf_IN    1
#define mf_PIPE  2
#define mf_FALSE 4
#define mf_OUT   8
#define mf_PERM 16
#define mf_TEST 32

/* for filtre */
#define f_COMMENT  0
#define f_INIT     1
#define f_KEEPCASE 2
#define f_READL    4
#define f_REG      8
#define f_ENDFILE 16
