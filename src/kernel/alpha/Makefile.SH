# Level 0 kernel is "asm extern"
# Level 1 kernel is the C generic one

cat >> $file << EOT
pariinl.h: $src/kernel/none/asm0.h $src/kernel/none/level1.h
	cat $src/kernel/none/asm0.h $src/kernel/none/level1.h > \$@
kernel.o:  $src/kernel/$asmarch/level0.s
	\$(AS) \$(ASFLAGS) -o kernel.o $src/kernel/$asmarch/level0.s
mp.o: .headers $src/kernel/none/mp.c
	\$(CC) -c \$(CFLAGS) \$(CPPFLAGS) -o mp.o $src/kernel/none/mp.c
mpinl.o: .headers $src/kernel/none/level1.h
	\$(CC) -c \$(CFLAGS) \$(CPPFLAGS) -o mpinl.o $src/kernel/none/level1.c

EOT
