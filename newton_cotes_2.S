.intel_syntax noprefix
.global newton_cotes_2

.data
result: .global double

.text

# Registeranfangsbelegung laut Calling Convention
# 	rdi = Zeiger auf dynamisch gebundene Funktion
# 	esi = integer a
# 	edx = integer b
# 	rax = Rueckgaberegister, enthaelt Zeiger auf Ergebnis im Speicher

# Bemerkung: "double" steht hier immer fuer double precision float (64 Bits) und "float" steht fuer single precision float (32 Bits)

# *~*~*~ Programmanfang ~*~*~*

newton_cotes_2:
	# Sichere alle Eingabevariablen in callee-save Register damit sie bei spaeteren Funktionsaufrufen nicht verloren gehen
	push r12
	push r13
	push r14
	mov r12, rdi
	mov r13d, esi
	mov r14d, edx

	cvtsi2sd xmm0, r13d								# xmm0 = double a							-convert und move 
	cvtsi2sd xmm1, r14d								# xmm1 = double b							-convert und move
	addsd xmm0, xmm1								# xmm0 = double (a+b)						-calc

	mov r10d, 2										# r10d = integer 2							-move
	cvtsi2sd xmm1, r10d								# xmm1 = double 2.0							-convert und move
	divsd xmm0, xmm1								# xmm0 = double ((a+b) / 2.0)				-calc

	call r12										# xmm0 = double p((a+b)/2)					-calc p((a+b)/2)
	movsd xmm1, xmm0                                # xmm1 = double p((a+b)/2)					-move
	mov r10d, 4										# r10d = integer 4							-move
	cvtsi2sd xmm2, r10d								# xmm2 = double 4.0							-convert und move
	mulsd xmm1, xmm2								# xmm1 = p((a+b)/2) * 4.0					-calc
	movq r8, xmm1 									# r8 = untere 64 Bits xmm1 = p((a+b)/2)*4.0	-move
	
	cvtsi2sd xmm0, r13d								# xmm0 = double a							-convert und move
	push r8											# r8 ist caller-saved, wird deshalb gesichert
	call r12										# xmm0 = double p(a)						-calc p(a)
	pop r8											# r8 wiederhergestellt
	movsd xmm2, xmm0								# xmm2 = double p(a)						-calc
	
	movq r9, xmm2									# r9 = untere 64 Bits xmm2 = double p(a)	-move
	cvtsi2sd xmm0, r14d								# xmm0 = double b							-convert und move
	push r8											# r8, r9 sind caller-saved, koennten also	-save
	push r9											# im Funktionsaufruf geaendert werden!		-save
	call r12                                      	# double p(b) in xmm0						-calc p(b)
	pop r9											# wiederherstellen von r9 = p(a)			-restore
	pop r8											# wiederherstellen von r8 = p((a+b)/2) * 4.0 -restore
	
	movq xmm1, r8									# xmm1 = r8 = p((a+b)/2) * 4.0				-move
	movq xmm2, r9									# xmm2 = r9 = p(a)							-move
	addsd xmm1, xmm2								# xmm1 = p((a+b)/2) * 4.0 + p(a)			-calc
	addsd xmm1, xmm0								# xmm1 = p((a+b)/2) * 4.0 + p(a) + p(b)		-calc

	mov r10d, 6										# r10d = integer 6							-move
	cvtsi2sd xmm2, r10d								# xmm2 = double 6.0							-move
	divsd xmm1, xmm2								# xmm1 = (p((a+b)/2)*4.0 + p(a) + p(b))/6	-calc
	sub r14d, r13d									# r14d = signed (b-a)						-calc
	cvtsi2sd xmm2, r14d								# xmm2 = signed double (b-a)				-calc
	mulsd xmm1, xmm2								# xmm1 = ((p((a+b)/2)*4.0+p(a)+p(b))/6)*(b-a) -calc = Keplersche Fassregel

	movsd qword ptr ds:[result], xmm1				# [result] = xmm1							-move in den Speicher
	mov rax, result									# rax = Zeiger auf result					-move Speicheradresse

	# Wiederherstellen der genutzten callee-save register
	pop r14
	pop r13
	pop r12
ret