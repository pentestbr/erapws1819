.intel_syntax noprefix
.global f
.text

# Kompilieren mit: gcc -shared min2x.S -o min2x.so
# Eingabe- und Rueckgabewert in xmm0!

f:
	mov r8, -2			# -2 wird in r8 geladen		-move
	cvtsi2sd xmm1, r8	# xmm1 = double -2.0		-convert und move
	mulsd xmm0, xmm1	# xmm0 = -2*x				-calc
ret