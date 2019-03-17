.intel_syntax noprefix
.global newton_cotes_1

.data
result: .global float

.text

# Registeranfangsbelegung laut Calling Convention
# 	rdi = Zeiger auf dynamisch gebundene Funktion
# 	esi = integer a
# 	edx = integer b
# 	rax = Rueckgaberegister, enthaelt Zeiger auf Ergebnis im Speicher

# Bemerkung: "double" steht hier immer fuer double precision float (64 Bits) und "float" steht fuer single precision float (32 Bits)

# *~*~*~ Programmanfang ~*~*~*

newton_cotes_1:
	# Sichere alle Eingabevariablen in callee-saved Register damit sie bei spaeteren Funktionsaufrufen nicht verloren gehen
	push r12
	push r13
	push r14
	mov r12, rdi
	mov r13d, esi
	mov r14d, edx
	
	# Berechnung:
	cvtsi2sd xmm0, r13d		# xmm0 = double a		-convert und move
	call r12				# xmm0 = double p(a)	-calc p(a)
	cvtsd2ss xmm1, xmm0		# xmm1 = float p(a)		-convert und move
	
	cvtsi2sd xmm0, r14d		# xmm0 = double b		-convert und move
	sub r14d, r13d			# r14 = int b - a		-calc b - a

	movq r13, xmm1			# r13 = float p(a)		-move fuer Sicherung
	call r12				# xmm0 = double p(b)	-calc p(b)
	movq xmm1, r13			# xmm1 = double p(a)	-move fuer Wiederherstellung
	cvtsd2ss xmm0,xmm0		# xmm0 = float p(b)		-convert

	mov r10d, 2				# r10 = int 2			-move
	cvtsi2ss xmm2, r10d		# xmm2 = float 2.0		-convert und move
	cvtsi2ss xmm3, r14d		# xmm3 = float b - a	-convert und move
	
	addss xmm0, xmm1		# xmm0 = float p(a) + p(b)					-calc 
	divss xmm0, xmm2		# xmm0 = float (p(a) + float p(b))/2.0		-calc 
	mulss xmm0, xmm3		# xmm0 = float (b-a) * (p(a) + p(b))/2.0	-calc = Trapezformel
	
	# Ergebnis abspeichern:
	movss dword ptr ds:[result], xmm0 # *result = flat (b-a) * (p(a) + p(b))/2.0	-move in den Speicher
	mov rax, result					  # rax = result								-move Speicheradresse

	# Genutzte callee-save register wieder herstellen
	pop r14
	pop r13
	pop r12
ret