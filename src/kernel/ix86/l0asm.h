/* $Id: l0asm.h,v 1.2.2.1 2003/11/03 13:37:09 bill Exp $

Copyright (C) 2000  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

#ifdef _MSC_VER
#  define C(entrypoint) entrypoint
#  define L(label) L##label
#else
#  ifdef ASM_UNDERSCORE
#    if defined(__STDC__) || defined(__cplusplus)
#      define C(entrypoint) _##entrypoint
#      define L(label) L##label
#    else
#      define C(entrypoint) _/**/entrypoint
#      define L(label) L/**/label
#    endif
#  else
#    define C(entrypoint) entrypoint
#    if defined(__STDC__) || defined(__cplusplus)
#      define L(label) .L##label
#    else
#      define L(label) .L/**/label
#    endif
#  endif
#endif

#ifdef _MSC_VER
#  define INTEL_SYNTAX
#else
#  ifdef ASM_UNDERSCORE
#    define BSD_SYNTAX
#  else
#    define ELF_SYNTAX
#  endif
#endif

#if defined (BSD_SYNTAX) || defined (ELF_SYNTAX)
#  define R(r) %r
#  define NUM(n) $##n
#  define ADDR(a) $##a
#  define X1
#  define X2
#  define X4
#  define X8
#  define MEM(base)(R(base))
#  define MEM_DISP(base,displacement)displacement(R(base))
#  define MEM_INDEX(base,index)(R(base),R(index))
#  define MEM_SHINDEX(base,index,size)(R(base),R(index),size)
#  define MEM_DISP_SHINDEX0(displacement,index,size)displacement(,R(index),size)
#  define MEM_DISP_SHINDEX(base,displacement,index,size)displacement(R(base),R(index),size)
#  define INDIR(value)*value
#  define INSNCONC(mnemonic,size_suffix)mnemonic##size_suffix
#  define INSN1(mnemonic,size_suffix,dst)INSNCONC(mnemonic,size_suffix) dst
#  define INSN2(mnemonic,size_suffix,src,dst)INSNCONC(mnemonic,size_suffix) src,dst
#  define INSN2MOVX(mnemonic,size_suffix,src,dst)INSNCONC(INSNCONC(mnemonic,size_suffix),l) src,dst
#  if defined(BSD_SYNTAX) || defined(COHERENT)
#    define INSN2SHCL(mnemonic,size_suffix,src,dst)INSNCONC(mnemonic,size_suffix) R(cl),src,dst
#    define REPZ repe ;
#  else
#    define INSN2SHCL(mnemonic,size_suffix,src,dst)INSNCONC(mnemonic,size_suffix) src,dst
#    define REPZ repz ;
#  endif
#  define REP rep ;
#  if defined(BSD_SYNTAX) && !(defined(__CYGWIN32__) || defined(__MINGW32__))
#    define ALIGN(log) .align log,0x90
#  endif
#  if defined(ELF_SYNTAX) || defined(__CYGWIN32__) || defined(__MINGW32__)
#    define ALIGN(log) .align 1<<(log)
#  endif
#endif

#ifdef INTEL_SYNTAX
#  define R(r) r
#  define NUM(n) n
#  define ADDR(a) OFFSET a
#  define X1 BYTE PTR
#  define X2 WORD PTR
#  define X4 DWORD PTR
#  define X8 QWORD PTR
#  define MEM(base) [base]
#  define MEM_DISP(base,displacement) [base+(displacement)]
#  define MEM_INDEX(base,index) [base+index]
#  define MEM_SHINDEX(base,index,size) [base+index*size]
#  define MEM_DISP_SHINDEX0(displacement,index,size) [(displacement)+index*size]
#  define MEM_DISP_SHINDEX(base,displacement,index,size) [base+(displacement)+index*size]
#  define INDIR(value)value
#  define INSNCONC(mnemonic,suffix)mnemonic##suffix
#  define INSN1(mnemonic,size_suffix,dst)mnemonic dst
#  define INSN2(mnemonic,size_suffix,src,dst)mnemonic dst,src
#  define INSN2MOVX(mnemonic,size_suffix,src,dst)INSNCONC(mnemonic,x) dst,src
#  define INSN2SHCL(mnemonic,size_suffix,src,dst)mnemonic dst,src,R(cl)
#  define REPZ repz
#  define REP rep
#  define movsl  movs R(eax)
#  define stosl  stos R(eax)
#  define scasl  scas R(eax)
#  define cmpsl  cmpsd
#  ifdef _MSC_VER
#    define ALIGN(log)
#  else
#    define ALIGN(log) .align log
#  endif
#endif

#ifdef _MSC_VER
#  define TEXT()
#  define GLOBL(name)
#  define FUNBEGIN(name) __declspec(naked) void name () { __asm {
#  define FUNEND()                                      }       }
#else
#  define TEXT() .text
#  define GLOBL(name) .globl name
#  define FUNBEGIN(name) C(name:)
#  define FUNEND()
#endif

#define _

TEXT()
