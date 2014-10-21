/* $Id: gp_rl.c,v 1.5.2.2 2005/02/20 17:03:27 kb Exp $

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
/*                 INTERFACE TO READLINE COMPLETION                */
/*                                                                 */
/*******************************************************************/
#include "pari.h"
#include "../language/anal.h"
#include "gp.h"

#ifdef READLINE
typedef char** (*CF)(char*, char* (*)()); /* completion function */
typedef char* (*GF)(char*, int); /* generator function */
typedef int (*RLCI)(int, int); /* rl_complete and rl_insert functions */

BEGINEXTERN
#ifdef HAS_RL_MESSAGE
#  define USE_VARARGS
#  define PREFER_STDARG
#endif
#ifdef READLINE_LIBRARY
#  include <readline.h>
#else
#  include <readline/readline.h>
#endif
#ifndef HAS_RL_MESSAGE
extern int rl_message (const char *, ...);
extern int rl_clear_message();
extern int rl_begin_undo_group(), rl_end_undo_group();
extern int rl_read_key();
extern int rl_stuff_char();
extern char *filename_completion_function(char *text,int state);
extern char *username_completion_function(char *text,int state);
#endif
char **pari_completion(char *text, int start, int end);
extern int rl_completion_query_items;
extern int rl_bind_key_in_map();
ENDEXTERN

void print_fun_list(char **matches, int nbli);
void aide(char *s, int flag);

extern default_type gp_default_list[];
extern char *keyword_list[];
static int add_help_keywords;
static entree *current_ep = NULL;

static int pari_rl_back;
extern RLCI rl_last_func;
static int do_args_complete = 1;
static int do_matched_insert = 0;
static int did_init_matched = 0;

#ifdef HAS_RL_SAVE_PROMPT
#  define SAVE_PROMPT() rl_save_prompt()
#  define RESTORE_PROMPT() rl_restore_prompt()
#else
#  ifdef HAS_UNDERSCORE_RL_SAVE_PROMPT
#    define SAVE_PROMPT() _rl_save_prompt()
#    define RESTORE_PROMPT() _rl_restore_prompt()
#  else
#    define SAVE_PROMPT()
#    define RESTORE_PROMPT()
#  endif
#endif

#ifdef HAS_RL_COMPLETION_MATCHES
#  define COMPLETION_MATCHES(a,b) (rl_completion_matches((a),(b)))
#  define FILE_COMPLETION ((GF)rl_filename_completion_function)
#  define USER_COMPLETION ((GF)rl_username_completion_function)
#else
#  define COMPLETION_MATCHES(a,b) \
     (completion_matches((char*)(a),(CPFunction*)(b)))
#  define FILE_COMPLETION ((GF)filename_completion_function)
#  define USER_COMPLETION ((GF)username_completion_function)
#endif

#define ELECTRIC_PAREN 1
#define ARGS_COMPLETE  2
static int
change_state(char *msg, int *opt, int count)
{
  int c;

  switch(count)
  {
    default: c = 0; break; /* off */
    case -1: c = 1; break; /* on  */
    case -2: c = 1 - *opt; /* toggle */
  }
  *opt = c;
  SAVE_PROMPT();
  rl_message("%s: %s.", msg, c? "on": "off");
  c = rl_read_key();
  RESTORE_PROMPT();
  rl_clear_message();
  rl_stuff_char(c); return 1;
}

/* Wrapper around rl_complete to allow insertion of () with a point in
   between. */
static int
pari_rl_complete(int count, int key)
{
  int ret;

  pari_rl_back = 0;
  if (count <= 0)
    return change_state("complete args", &do_args_complete, count);

  rl_begin_undo_group();
  if (rl_last_func == pari_rl_complete)
    rl_last_func = (RLCI) rl_complete; /* Make repeated TABs different */
  ret = ((RLCI)rl_complete)(count,key);
  if (pari_rl_back && (pari_rl_back <= rl_point))
    rl_point -= pari_rl_back;
  rl_end_undo_group(); return ret;
}

static const char paropen[] = "([{";
static const char parclose[] = ")]}";

/* To allow insertion of () with a point in between. */
static int
pari_rl_matched_insert(int count, int key)
{
  int i = 0, ret;

  if (count <= 0)
    return change_state("electric parens", &do_matched_insert, count);
  while (paropen[i] && paropen[i] != key) i++;
  if (!paropen[i] || !do_matched_insert || under_emacs)
    return ((RLCI)rl_insert)(count,key);
  rl_begin_undo_group();
  ((RLCI)rl_insert)(count,key);
  ret = ((RLCI)rl_insert)(count,parclose[i]);
  rl_point -= count;
  rl_end_undo_group(); return ret;
}

static int
pari_rl_default_matched_insert(int count, int key)
{
    if (!did_init_matched) {
	did_init_matched = 1;
	do_matched_insert = 1;
    }
    return pari_rl_matched_insert(count, key);
}

static int
pari_rl_forward_sexp(int count, int key)
{
  int deep = 0, dir = 1, move_point = 0, lfail;

  (void)key;
  if (count < 0)
  {
    count = -count; dir = -1;
    if (!rl_point) goto fail;
    rl_point--;
  }
  while (count || deep)
  {
    move_point = 1;	/* Need to move point if moving left. */
    lfail = 0;		/* Do not need to fail left movement yet. */
    while ( !is_keyword_char(rl_line_buffer[rl_point])
            && !strchr("\"([{}])",rl_line_buffer[rl_point])
            && !( (dir == 1)
                  ? (rl_point >= rl_end)
                  : (rl_point <= 0 && (lfail = 1))))
        rl_point += dir;
    if (lfail || !rl_line_buffer[rl_point]) goto fail;

    if (is_keyword_char(rl_line_buffer[rl_point]))
    {
      while ( is_keyword_char(rl_line_buffer[rl_point])
              && (!((dir == 1) ? (rl_point >= rl_end) : (rl_point <= 0))
                  || (move_point = 0)))
        rl_point += dir;
      if (!deep) count--;
    }
    else if (strchr(paropen,rl_line_buffer[rl_point]))
    {
      if (deep == 0 && dir == -1) goto fail; /* We are already out of pars. */
      rl_point += dir;
      deep++; if (!deep) count--;
    }
    else if (strchr(parclose,rl_line_buffer[rl_point]))
    {
      if (deep == 0 && dir == 1)
      {
        rl_point++; goto fail; /* Get out of pars. */
      }
      rl_point += dir;
      deep--; if (!deep) count--;
    }
    else if (rl_line_buffer[rl_point] == '\"')
    {
      int bad = 1;

      rl_point += dir;
      while ( ((rl_line_buffer[rl_point] != '\"') || (bad = 0))
              && (!((dir == 1) ? (rl_point >= rl_end) : (rl_point <= 0))
                  || (move_point = 0)) )
        rl_point += dir;
      if (bad) goto fail;
      rl_point += dir;	/* Skip the other delimiter */
      if (!deep) count--;
    }
    else
    {
      fail: ding(); return 1;
    }
  }
  if (dir != 1 && move_point) rl_point++;
  return 1;
}

static int
pari_rl_backward_sexp(int count, int key)
{
  return pari_rl_forward_sexp(-count, key);
}

/* do we add () at the end of completed word? (is it a function?) */
static int
add_paren(int end)
{
  entree *ep;
  char *s;

  if (end < 0 || rl_line_buffer[end] == '(')
    return 0; /* not from command_generator or already there */
  ep = do_alias(current_ep); /* current_ep set in command_generator */
  if (EpVALENCE(ep) < EpUSER)
  { /* is it a constant masked as a function (e.g Pi)? */
    s = ep->help; if (!s) return 1;
    while (is_keyword_char(*s)) s++;
    return (*s != '=');
  }
  switch(EpVALENCE(ep))
  {
    case EpUSER:
    case EpINSTALL: return 1;
  }
  return 0;
}

static void
match_concat(char **matches, char *s)
{
  int i = strlen(matches[0]) + 1;
  matches[0] = (char*) gprealloc(matches[0], i+strlen(s), i);
  strcat(matches[0],s);
}

static char **
matches_for_emacs(char *text, char **matches)
{
  if (!matches) printf("@");
  else
  {
    int i;
    printf("%s@", matches[0] + strlen(text));
    if (matches[1]) print_fun_list(matches+1,0);

   /* we don't want readline to do anything, but insert some junk
    * which will be erased by emacs.
    */
    for (i=0; matches[i]; i++) free(matches[i]);
    free(matches);
  }
  matches = (char **) gpmalloc(2*sizeof(char *));
  matches[0] = gpmalloc(2); sprintf(matches[0],"_");
  matches[1] = NULL; printf("@E_N_D"); pariflush();
  return matches;
}

#define add_comma(x) (x==-2) /* from default_generator */

/* Attempt to complete on the contents of TEXT. END points to the end of the
 * word to complete. Return the array of matches, or NULL if there aren't any.
 */
static char **
get_matches(int end, char *text, char* f(char*,int))
{
  char **matches;

#ifdef HAS_COMPLETION_APPEND_CHAR
  rl_completion_append_character = ' ';
#endif
  current_ep = NULL;
  matches = COMPLETION_MATCHES(text, (char *(*)())f);
  if (matches && !matches[1]) /* a single match */
  {
    if (add_paren(end))
    {
      match_concat(matches,"()");
      pari_rl_back = 1;
      if (rl_point == rl_end)
#ifdef HAS_COMPLETION_APPEND_CHAR
        rl_completion_append_character = '\0'; /* Do not append space. */
#else
        pari_rl_back = 2;
#endif
    }
    else if (add_comma(end))
      match_concat(matches,",");
  }
  if (under_emacs) matches = matches_for_emacs(text,matches);
  return matches;
}
#undef add_comma

static char *
add_junk(char *name, char *text, long junk)
{
  char *s = strncpy((char*) gpmalloc(strlen(name)+1+junk),text,junk);
  strcpy(s+junk,name); return s;
}

/* Generator function for command completion.  STATE lets us know whether
 * to start from scratch; without any state (i.e. STATE == 0), then we
 * start at the top of the list.
 */
static char *
hashtable_generator (char *text, int state, entree **hash)
{
  static int hashpos, len, junk, n;
  static entree* ep;
  static char *TEXT;

 /* If this is a new word to complete, initialize now:
  *  + indexes hashpos (GP hash list) and n (keywords specific to long help).
  *  + file completion and keyword completion use different word boundaries,
  *    have TEXT point to the keyword start.
  *  + save the length of TEXT for efficiency.
  */
  if (!state)
  {
    n = hashpos = 0; ep=hash[hashpos];
    len=strlen(text); junk=len-1;
    while (junk >= 0 && is_keyword_char(text[junk])) junk--;
    junk++; len -= junk; TEXT = text + junk;
  }

  /* First check the keywords list */
  if (add_help_keywords)
  {
    for ( ; keyword_list[n]; n++)
      if (!strncmp(keyword_list[n],TEXT,len))
      {
        text = add_junk(keyword_list[n],text,junk);
        n++; return text;
      }
  }

  /* Return the next name which partially matches from the command list. */
  for(;;)
    if (!ep)
    {
      if (++hashpos >= functions_tblsz) return NULL; /* no names matched */
      ep = hash[hashpos];
    }
    else if (strncmp(ep->name,TEXT,len))
      ep = ep->next;
    else
      break;
  current_ep = ep;
  text = add_junk(ep->name,text,junk);
  ep=ep->next; return text;
}

static char *
command_generator (char *text, int  state)
{
  return hashtable_generator(text,state, functions_hash);
}
static char *
member_generator (char *text, int  state)
{
  return hashtable_generator(text,state, members_hash);
}

#define DFLT 0
#define ENTREE 1

static char *
generator(void *list, char *text, int *nn, int len, int typ)
{
  char *def = NULL, *name;
  int n = *nn;

  /* Return the next name which partially matches from list.*/
  switch(typ)
  {
    case DFLT :
      do
	def = (((default_type *) list)[n++]).name;
      while (def && strncmp(def,text,len));
      break;

    case ENTREE :
      do
	def = (((entree *) list)[n++]).name;
      while (def && strncmp(def,text,len));
  }

  *nn = n;
  if (def)
  {
    name = strcpy((char*) gpmalloc(strlen(def)+1), def);
    return name;
  }
  return NULL; /* no names matched */
}

static char *
old_generator(char *text,int state)
{
  static int n,len;
  static char *res;

  if (!state) { res = "a"; n=0; len=strlen(text); }
  if (res)
  {
    res = generator((void *)oldfonctions,text,&n,len,ENTREE);
    if (res) return res;
    n=0;
  }
  return generator((void *)functions_oldgp,text,&n,len,ENTREE);
}

static char *
default_generator(char *text,int state)
{
  static int n,len;

  if (!state) { n=0; len=strlen(text); }
  return generator(gp_default_list,text,&n,len,DFLT);
}

char **
pari_completion(char *text, int start, int end)
{
  int i, first=0;

/* If the line does not begin by a backslash, then it is:
 * . an old command ( if preceded by "whatnow(" ).
 * . a default ( if preceded by "default(" ).
 * . a member function ( if preceded by "." within 4 letters )
 * . a file name (in current directory) ( if preceded by "read(" )
 * . a command
 */
  if (start >=1 && rl_line_buffer[start] != '~') start--;
  while (start && is_keyword_char(rl_line_buffer[start])) start--;
  if (rl_line_buffer[start] == '~')
  {
    for(i=start+1;i<=end;i++)
      if (rl_line_buffer[i] == '/')
	return get_matches(-1,text,FILE_COMPLETION);
    return get_matches(-1,text,USER_COMPLETION);
  }

  while (rl_line_buffer[first] && isspace((int)rl_line_buffer[first])) first++;
  switch (rl_line_buffer[first])
  {
    case '\\':
      if (first == start) text = rl_line_buffer+start+1;
      return get_matches(-1,text,FILE_COMPLETION);
    case '?':
      if (rl_line_buffer[first+1] == '?') add_help_keywords = 1;
      return get_matches(-1,text,command_generator);
  }

  while (start && rl_line_buffer[start] != '('
               && rl_line_buffer[start] != ',') start--;
  if (rl_line_buffer[start] == '(' && start)
  {
#define MAX_KEYWORD 200
    int iend, j;
    entree *ep;
    char buf[MAX_KEYWORD];

    i = start;

    while (i && isspace((int)rl_line_buffer[i-1])) i--;
    iend = i;
    while (i && is_keyword_char(rl_line_buffer[i-1])) i--;

    if (iend - i == 7)
    {
      if (strncmp(rl_line_buffer + i,"default",7) == 0)
	return get_matches(-2,text,default_generator);
      if (strncmp(rl_line_buffer + i,"whatnow",7) == 0)
	return get_matches(-1,text,old_generator);
    }
    if (iend - i >= 4)
    {
      if (strncmp(rl_line_buffer + i,"read",4) == 0)
	return get_matches(-1,text,FILE_COMPLETION);
    }

    j = start + 1;
    while (j && isspace((int)rl_line_buffer[j])) j++;
    /* If we are in empty parens, insert arguments for the function: */
    if ( (rl_line_buffer[j] == ')' || !rl_line_buffer[j] )
	 && do_args_complete
	 && (iend - i < MAX_KEYWORD)
	 && ( strncpy(buf, rl_line_buffer + i, iend - i),
	      buf[iend - i] = 0, 1)
	 && (ep = is_entry(buf)) && ep->help)
     {
      char *s = ep->help;

      while (is_keyword_char(*s)) s++;
      if (*s++ == '(')
      { /* Function, print arguments! */
        char *endh = s;
        while (*endh && *endh != ')' && *endh != '(') endh++;
        if (*endh == ')')
        { /* Well-formed help.  */
          char *str = strncpy((char*) gpmalloc(endh-s + 1), s, endh-s);
          char **ret = (char**)gpmalloc(sizeof(char*)*2);
          str[endh-s] = 0;
          ret[0] = str; ret[1] = NULL;
          if (under_emacs) ret = matches_for_emacs("",ret);
          return ret;
        }
      }
    }
  }
  for(i=end-1;i>=start;i--)
    if (!is_keyword_char(rl_line_buffer[i]))
    {
      if (rl_line_buffer[i] == '.')
        return get_matches(-1,text,member_generator);
      break;
    }
  add_help_keywords = 0;
  return get_matches(end,text,command_generator);
}

static int
rl_short_help(int count, int key)
{
  int p=rl_point, off=p, e = rl_end;
  FILE *save = pari_outfile;
  long flag = h_RL;

  while (off && is_keyword_char(rl_line_buffer[off-1])) off--;

  /* Check for \c type situation.  Could check for leading whitespace too... */
  if (off == 1 && rl_line_buffer[off-1] == '\\') off--;
  if (off >= 8) {		/* Check for default(whatever) */
    int t = off - 1;

    while (t >= 7 && (rl_line_buffer[t] == ' ' || rl_line_buffer[t] == '\t'))
      t--;
    if (rl_line_buffer[t--] == '(') {
      while (t >= 6 && (rl_line_buffer[t] == ' ' || rl_line_buffer[t] == '\t'))
	t--;
      if (t >= 6 && rl_line_buffer[t] == 't'
	  && strncmp(rl_line_buffer + t - 6, "default", 7) == 0
	  && (t == 6 || !is_keyword_char(rl_line_buffer[t-7])))
	off = t - 6;		/* All this for that assignment */
    }
  }
  rl_point = 0; rl_end = 0; pari_outfile = rl_outstream;
  if (count < 0) flag |= h_LONG; /* long help */
  SAVE_PROMPT();
  rl_message("%s","");
  aide(rl_line_buffer + off, flag);
  RESTORE_PROMPT();
  rl_point = p; rl_end = e; pari_outfile = save;
  rl_clear_message();
#ifdef RL_REFRESH_LINE_OLDPROTO
  rl_refresh_line();
#else
  rl_refresh_line(count,key);
#endif
  return 0;
}

static int
rl_long_help(int count, int key)
{
  (void)count;
  return rl_short_help(-1,key);
}

void
init_readline()
{
  /* Allow conditional parsing of the ~/.inputrc file. */
  rl_readline_name = "Pari-GP";

  /* added ~, ? and , */
  rl_basic_word_break_characters = " \t\n\"\\'`@$><=;|&{(?,~";
  rl_special_prefixes = "~";

  /* custom completer */
#ifndef HAS_RL_COMPLETION_FUNC_T
# ifndef CPPFunction_defined
#   define CPPFunction Function
# endif
# define rl_completion_func_t CPPFunction
#endif
  rl_attempted_completion_function = (rl_completion_func_t*) pari_completion;

  /* we always want the whole list of completions under emacs */
  if (under_emacs) rl_completion_query_items = 0x8fff;

#define Bind(a,b,c) (rl_bind_key_in_map((a), (Function*)(b), (c)))
#define Defun(a,b,c) (rl_add_defun((char*)(a), (Function*)(b), (c)))

  Defun("short-help", rl_short_help, -1);
  Defun("long-help", rl_long_help, -1);
  Defun("pari-complete", pari_rl_complete, '\t');
  Defun("pari-matched-insert", pari_rl_default_matched_insert, -1);
  Defun("pari-forward-sexp", pari_rl_forward_sexp, -1);
  Defun("pari-backward-sexp", pari_rl_backward_sexp, -1);

  Bind('h', rl_short_help, emacs_meta_keymap);
  Bind('H', rl_long_help,  emacs_meta_keymap);
  Bind('h', rl_short_help, vi_movement_keymap);
  Bind('H', rl_long_help,  vi_movement_keymap);
  Bind('(', pari_rl_matched_insert, emacs_standard_keymap);
  Bind('[', pari_rl_matched_insert, emacs_standard_keymap);
  Bind(6, pari_rl_forward_sexp,  emacs_meta_keymap); /* M-C-f */
  Bind(2, pari_rl_backward_sexp, emacs_meta_keymap); /* M-C-b */

#ifdef EMACS_DOS_KEYMAP
  Bind(';', rl_short_help, emacs_dos_keymap); /* F1 */
  Bind('T', rl_long_help,  emacs_dos_keymap); /* Shift-F1 */
  Bind(155, pari_rl_backward_sexp, emacs_dos_keymap); /* Alt-Left */
  Bind(157, pari_rl_forward_sexp,  emacs_dos_keymap); /* Alt-Right*/
#endif
}
#endif
