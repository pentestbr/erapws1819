#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

extern float *newton_cotes_1(float (*p)(float), int a, int b);
extern double *newton_cotes_2(double (*p)(double), int a, int b);

// Zeit in nanosekunden
static inline double curtime(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e9 + t.tv_nsec;
}

int main(int argc, char *argv[]) {
	// Eingabeueberpruefung
	if (argc != 4 && argc != 5){
		printf("Incorrect amount of arguments entered. Expected 3 or 4, got %d\n", argc-1);
		return EXIT_FAILURE;
	}
	
	/* Integrationsgrenzen a und b werden mit strol konvertiert statt atoi damit man zwischen einer gueltigen 0
	und ein geworfenen Fehler von atoi unterscheiden kann (atoi wirft 0 bei falsche Eingaben) */
	const char *a_ptr = argv[2];
	char *a_end_ptr = NULL;
	
	const char *b_ptr = argv[3];
	char *b_end_ptr = NULL;
	
	errno = 0;
	long a_long = strtol (a_ptr, &a_end_ptr, 10);
	
	/* Test fuer (in der Reihenfolge): ungueltige Nummern, Nummern mit nachfolgenden Zeichen, long over- oder underflows, unbekannte Fehler. Analog unten fuer die Integrations-
	grenze b. Die zweite Pr�fung ist nicht erforderlich, da durch das Ignorieren der ungueltigen Zeichen am Ende der Zeichenfolge immer noch ein g�ltiges long zur�ckgegeben wird. 
	W�rden wir das zulassen, so k�nnte jedoch den Eindruck entstehen, dass Eingaben in anderen Basen wie "1001b" oder "42h" korrekt interpretiert werden. Strtol hat die
	Funktionalitaet mit anderen Basen zu arbeiten im 3. Argument, wir beschraenken uns jedoch auf der Basis 10 in diesen Rahmenprogramm.*/	
	if (a_ptr == a_end_ptr) { printf ("Integrationsgrenze 'a' hat ungueltige Zeichen\n"); return EXIT_FAILURE; }
	if (errno == 0 && a_ptr && *a_end_ptr != 0) { printf("Ungueltige Zeichen nach Integrationsgrenze 'a'\n"); return EXIT_FAILURE; }
	if ((errno == ERANGE && a_long == LONG_MAX) || (errno == ERANGE && a_long == LONG_MIN)) { printf("Integrationsgrenze 'a' zu gross fuer ein long\n"); return EXIT_FAILURE; }
	if (errno != 0 && a_long == 0) { printf ("Ungueltige Eingabe fuer Integrationsgrenze 'a'\n"); return EXIT_FAILURE; }
	
	errno = 0;
	long b_long = strtol (b_ptr, &b_end_ptr, 10);
	
	if (b_ptr == b_end_ptr) { printf ("Integrationsgrenze 'b' hat ungueltige Zeichen\n"); return EXIT_FAILURE; }
	if (errno == 0 && b_ptr && *b_end_ptr != 0) { printf("Ungueltige Zeichen nach Integrationsgrenze 'b'\n"); return EXIT_FAILURE; }
	if ((errno == ERANGE && b_long == LONG_MAX) || (errno == ERANGE && b_long == LONG_MIN)) { printf("Integrationsgrenze 'b' zu gross fuer ein long\n"); return EXIT_FAILURE; }
	if (errno != 0 && b_long == 0) { printf ("Ungueltige Eingabe fuer Integrationsgrenze 'b'\n"); return EXIT_FAILURE; }
	if (argc == 5 && strcmp(argv[4],"-t") != 0){printf("Ungueltiges Flag\n"); return EXIT_FAILURE; }

	// Pruefen ob gueltiges long in ein integer passt oder over- oder underflows
	if(a_long > INT_MAX || a_long < INT_MIN || b_long > INT_MAX || b_long < INT_MIN) {
		printf("Integrationsgrenzen besitzen Werte ausserhalb des Darstellungsbereichs eines integers\n");
		return EXIT_FAILURE;
	}
	
	int a = (int) a_long;
	int b = (int) b_long;
	
	// Zeigerdeklarations
	float (*p_nc1)(float);
	double (*p_nc2)(double);
	void * handle;
	char * error;
	
	// Gewuenschte Dynamische Bibliothek oeffnen
	if (strcmp(argv[1], "42") == 0){
		handle = dlopen("./functions/42.so", RTLD_LAZY);
	}
	else if (strcmp(argv[1], "min2x") == 0){
		handle = dlopen("./functions/min2x.so", RTLD_LAZY);
	}
	else if (strcmp(argv[1], "3x-1") == 0){
		handle = dlopen("./functions/3x-1.so", RTLD_LAZY);
	}
	else if (strcmp(argv[1], "x^2+1") == 0){
		handle = dlopen("./functions/x^2+1.so", RTLD_LAZY);
	}
	else if (strcmp(argv[1], "x^3-25x") == 0){
		handle = dlopen("./functions/x^3-25x.so", RTLD_LAZY);
	}
	else if (strcmp(argv[1], "x^4-21x^3+52x^2+480x-512") == 0){
		handle = dlopen("./functions/x^4-21x^3+52x^2+480x-512.so", RTLD_LAZY);
	}
	else if (strcmp(argv[1], "x^5-21x^4+52x^3+480x^2-512x") == 0){
		handle = dlopen("./functions/x^5-21x^4+52x^3+480x^2-512x.so", RTLD_LAZY);
	}
	else if (strcmp(argv[1], "e^x") == 0){
		handle = dlopen("./functions/e^x.so", RTLD_LAZY);
	}
	else {
		printf("The entered function does not exist\n");
		return EXIT_FAILURE;
	}
	
	// Pruefen, ob Bindung erfogreich lief. Fehlermeldung wird automatisch auf der Konsole geprintet
	if(!handle){
		fputs(dlerror(), stderr);
		printf("\n");
		return EXIT_FAILURE;
	}
	
    if ((error = dlerror()) != NULL) {
        fputs(error, stderr);
		printf("\n");
        return EXIT_FAILURE;
    }
	
	// Zeiger werden Adresse der Funktion zugewiesen
	p_nc1 = dlsym(handle, "f");
	p_nc2 = dlsym(handle, "f");	
	
	// Assemblycode ausfuehren
	float * result_asm_1 = newton_cotes_1(p_nc1, a, b);
	double * result_asm_2 = newton_cotes_2(p_nc2, a, b);

	// Ergebnisse bekommen durch deferenzierung der zurueckgegebenen Adresse
	float * num_qdrt1_ptr = ((float *) &result_asm_1);
	float num_qdrt1_result = *num_qdrt1_ptr; 
	
	double * num_qdrt2_ptr = ((double *) &result_asm_2);
	double num_qdrt2_result = *num_qdrt2_ptr;
	
	printf("Ergebnis mit Newton Cotes 1 = %f\n", num_qdrt1_result);
	printf("Ergebnis mit Newton Cotes 2 = %f\n", num_qdrt2_result);
	
	/** Summierte Numerische Quadratur **/
	float superPrecise_nc1 = 0;
	double superPrecise_nc2 = 0;
	
	float * sp_asm_1;
	double * sp_asm_2;
	int swapped = false;

	// es wird immer von der kleineren zur groesseren Variable Integriert
	// und eventuell das Ergebnis negiert
	if (a > b) {
		swapped = true;
		int swap = b;
		b = a;
		a = swap;
	}

	// fuehre die Quadratur fuer jedes Teilinterval durch
	// und summiere die Ergebnisse auf
	for (int i = a; i < b; i++) {
		sp_asm_1 = newton_cotes_1(p_nc1, i, (i+1));
		sp_asm_2 = newton_cotes_2(p_nc2, i, (i+1));

		float * num_qdrt1_ptr_sp = ((float *) &sp_asm_1);
		superPrecise_nc1 += *num_qdrt1_ptr_sp; 
	
		double * num_qdrt2_ptr_sp = ((double *) &sp_asm_2);
		superPrecise_nc2 += *num_qdrt2_ptr_sp; 
	}

	// evtl negieren
	if (swapped == false) {
		printf("Ergebnis der summierten Numerischen Quadratur mit Newton Cotes 1 = %f\n", superPrecise_nc1);
		printf("Ergebnis der summierten Numerischen Quadratur mit Newton Cotes 2 = %f\n", superPrecise_nc2);
	} else {
		printf("Ergebnis der summierten Numerischen Quadratur mit Newton Cotes 1 = %f\n", -1*superPrecise_nc1);
		printf("Ergebnis der summierten Numerischen Quadratur mit Newton Cotes 2 = %f\n", -1*superPrecise_nc2);
	}
	/** Zeitmessung und Tests fuer Flag -t**/
	if(argc == 5){
		printf("\n");
		// variablen fuer Zeitmessung
		double start_time;
		double end_time;
		double newton_cotes_1_time = 0;
		double newton_cotes_2_time = 0;
		
		// Zeitmessung:
		// 5 durchlaeufe mit je 1000000 Aufrufen und 1s Pause dazwischen 
		for(int i = 0; i < 5; i++){
			start_time = curtime();
			for(int i = 0; i < 1000000; i++){
				newton_cotes_1(p_nc1, a, b);
			}
			end_time = curtime();
			newton_cotes_1_time += end_time - start_time;
			start_time = curtime();
			for(int i = 0; i < 1000000; i++){
				newton_cotes_2(p_nc2, a, b);
			}
			end_time = curtime();
			newton_cotes_2_time += end_time - start_time;
			// 1 Sekunde Abstand zwischen Zeitmessungen
			sleep(1);
		}

		// 5000000 mal ausgefuehrt -> fuer einen Durchlauf / 5000000
		newton_cotes_1_time /= 5000000;
		newton_cotes_2_time /= 5000000;

		printf("Berechnungsdauer fuer Newton Cotes 1 = %f Nanosekunden\n", newton_cotes_1_time);
		printf("Berechnungsdauer fuer Newton Cotes 2 = %f Nanosekunden\n", newton_cotes_2_time);
		printf("\n");
		
		/** Tests **/
		/*Hier wird fuer jeden Testfall die Numerische Quadratur berechnet*/
		float (*p_nc1_test)(float);
		float * nc1_testing;
		float * nc1_testing_ptr;
		float nc1_testing_res;
		
		double (*p_nc2_test)(double);
		double * nc2_testing;
		double * nc2_testing_ptr;
		double nc2_testing_res;
		// korrekter Wert des Integrals
		double area;
		
		/*
		f(x) = 42
		*/
		
		handle = dlopen("./functions/42.so", RTLD_LAZY);
		
		p_nc1_test = dlsym(handle, "f");
		p_nc2_test = dlsym(handle, "f");
		
		a = -31;
		b = 50;
		area = 3402;
		
		nc1_testing = newton_cotes_1(p_nc1_test, a, b);
		nc2_testing = newton_cotes_2(p_nc2_test, a, b);
		
		nc1_testing_ptr = ((float *) &nc1_testing);
		nc2_testing_ptr = ((double *) &nc2_testing);
		
		nc1_testing_res = * nc1_testing_ptr;
		nc2_testing_res = * nc2_testing_ptr;
		
		printf("Integral 			f(x) = 42 von -31 bis 50 mit Newton Cotes 1 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", nc1_testing_res, area, (nc1_testing_res-area),((nc1_testing_res/area)*100)-100);
		printf("Integral 			f(x) = 42 von -31 bis 50 mit Newton Cotes 2 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", nc2_testing_res, area, (nc2_testing_res-area), ((nc2_testing_res/area)*100)-100);
		
		nc1_testing_res = 0;
		nc2_testing_res = 0;
		
		for (int i = a; i < b; i++) {
			nc1_testing = newton_cotes_1(p_nc1_test, i, (i+1));
			nc2_testing = newton_cotes_2(p_nc2_test, i, (i+1));
		
			float * nc1_nqtest_ptr = ((float *) &nc1_testing);
			nc1_testing_res += * nc1_nqtest_ptr; 
	
			double * nc2_nqtest_ptr = ((double *) &nc2_testing);
			nc2_testing_res += * nc2_nqtest_ptr; 
		}
		
		printf("Summierte Numerische Quadratur	f(x) = 42 von %d bis %d mit Newton Cotes 1 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc1_testing_res, area, (nc1_testing_res-area), ((nc1_testing_res/area)*100)-100);
		printf("Summierte Numerische Quadratur	f(x) = 42 von %d bis %d mit Newton Cotes 2 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc2_testing_res, area, (nc2_testing_res-area), ((nc2_testing_res/area)*100)-100);
		
		nc1_testing_res = 0;
		nc2_testing_res = 0;
		
		printf("\n");
		
		/*
		f(x) = -2x
		*/
		
		handle = dlopen("./functions/min2x.so", RTLD_LAZY);
		
		p_nc1_test = dlsym(handle, "f");
		p_nc2_test = dlsym(handle, "f");
		
		a = -2000;
		b = 2019;
		area = -76361;
		
		nc1_testing = newton_cotes_1(p_nc1_test, a, b);
		nc2_testing = newton_cotes_2(p_nc2_test, a, b);
		
		nc1_testing_ptr = ((float *) &nc1_testing);
		nc2_testing_ptr = ((double *) &nc2_testing);
		
		nc1_testing_res = * nc1_testing_ptr;
		nc2_testing_res = * nc2_testing_ptr;
		
		printf("Integral 			f(x) = -2x von %d bis %d mit Newton Cotes 1 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc1_testing_res, area, (nc1_testing_res-area),((nc1_testing_res/area)*100)-100);
		printf("Integral 			f(x) = -2x von %d bis %d mit Newton Cotes 2 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc2_testing_res, area, (nc2_testing_res-area), ((nc2_testing_res/area)*100)-100);
		
		nc1_testing_res = 0;
		nc2_testing_res = 0;
		
		for (int i = a; i < b; i++) {
			nc1_testing = newton_cotes_1(p_nc1_test, i, (i+1));
			nc2_testing = newton_cotes_2(p_nc2_test, i, (i+1));
		
			float * nc1_nqtest_ptr = ((float *) &nc1_testing);
			nc1_testing_res += * nc1_nqtest_ptr; 
	
			double * nc2_nqtest_ptr = ((double *) &nc2_testing);
			nc2_testing_res += * nc2_nqtest_ptr; 
		}
		
		printf("Summierte Numerische Quadratur	f(x) = -2x von %d bis %d mit Newton Cotes 1 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc1_testing_res, area, (nc1_testing_res-area), ((nc1_testing_res/area)*100)-100);
		printf("Summierte Numerische Quadratur	f(x) = -2x von %d bis %d mit Newton Cotes 2 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc2_testing_res, area, (nc2_testing_res-area), ((nc2_testing_res/area)*100)-100);
		
		nc1_testing_res = 0;
		nc2_testing_res = 0;
		
		printf("\n");
		
		/*
		f(x) = 3x-1
		*/
		
		handle = dlopen("./functions/3x-1.so", RTLD_LAZY);
		
		p_nc1_test = dlsym(handle, "f");
		p_nc2_test = dlsym(handle, "f");
		
		a = -10;
		b = -5;
		area = -117.5;
		
		nc1_testing = newton_cotes_1(p_nc1_test, a, b);
		nc2_testing = newton_cotes_2(p_nc2_test, a, b);
		
		nc1_testing_ptr = ((float *) &nc1_testing);
		nc2_testing_ptr = ((double *) &nc2_testing);
		
		nc1_testing_res = * nc1_testing_ptr;
		nc2_testing_res = * nc2_testing_ptr;
		
		printf("Integral 			f(x) = 3x-1 von %d bis %d mit Newton Cotes 1 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc1_testing_res, area, (nc1_testing_res-area),((nc1_testing_res/area)*100)-100);
		printf("Integral 			f(x) = 3x-1 von %d bis %d mit Newton Cotes 2 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc2_testing_res, area, (nc2_testing_res-area), ((nc2_testing_res/area)*100)-100);
		
		nc1_testing_res = 0;
		nc2_testing_res = 0;
		
		for (int i = a; i < b; i++) {
			nc1_testing = newton_cotes_1(p_nc1_test, i, (i+1));
			nc2_testing = newton_cotes_2(p_nc2_test, i, (i+1));
		
			float * nc1_nqtest_ptr = ((float *) &nc1_testing);
			nc1_testing_res += * nc1_nqtest_ptr; 
	
			double * nc2_nqtest_ptr = ((double *) &nc2_testing);
			nc2_testing_res += * nc2_nqtest_ptr; 
		}
		
		printf("Summierte Numerische Quadratur	f(x) = 3x-1 von %d bis %d mit Newton Cotes 1 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc1_testing_res, area, (nc1_testing_res-area), ((nc1_testing_res/area)*100)-100);
		printf("Summierte Numerische Quadratur	f(x) = 3x-1 von %d bis %d mit Newton Cotes 2 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc2_testing_res, area, (nc2_testing_res-area), ((nc2_testing_res/area)*100)-100);
		
		nc1_testing_res = 0;
		nc2_testing_res = 0;
		
		printf("\n");
		
		/*
		f(x) = x^2 + 1
		*/
		
		handle = dlopen("./functions/x^2+1.so", RTLD_LAZY);
		
		p_nc1_test = dlsym(handle, "f");
		p_nc2_test = dlsym(handle, "f");
		
		a = -42;
		b = 0;
		area = 24738;
		
		nc1_testing = newton_cotes_1(p_nc1_test, a, b);
		nc2_testing = newton_cotes_2(p_nc2_test, a, b);
		
		nc1_testing_ptr = ((float *) &nc1_testing);
		nc2_testing_ptr = ((double *) &nc2_testing);
		
		nc1_testing_res = * nc1_testing_ptr;
		nc2_testing_res = * nc2_testing_ptr;
		
		printf("Integral 			f(x) = x^2+1 von %d bis %d mit Newton Cotes 1 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc1_testing_res, area, (nc1_testing_res-area),((nc1_testing_res/area)*100)-100);
		printf("Integral 			f(x) = x^2+1 von %d bis %d mit Newton Cotes 2 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc2_testing_res, area, (nc2_testing_res-area), ((nc2_testing_res/area)*100)-100);
		
		nc1_testing_res = 0;
		nc2_testing_res = 0;
		
		for (int i = a; i < b; i++) {
			nc1_testing = newton_cotes_1(p_nc1_test, i, (i+1));
			nc2_testing = newton_cotes_2(p_nc2_test, i, (i+1));
		
			float * nc1_nqtest_ptr = ((float *) &nc1_testing);
			nc1_testing_res += * nc1_nqtest_ptr; 
	
			double * nc2_nqtest_ptr = ((double *) &nc2_testing);
			nc2_testing_res += * nc2_nqtest_ptr; 
		}
		
		printf("Summierte Numerische Quadratur	f(x) = x^2+1 von %d bis %d mit Newton Cotes 1 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc1_testing_res, area, (nc1_testing_res-area), ((nc1_testing_res/area)*100)-100);
		printf("Summierte Numerische Quadratur	f(x) = x^2+1 von %d bis %d mit Newton Cotes 2 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc2_testing_res, area, (nc2_testing_res-area), ((nc2_testing_res/area)*100)-100);
		
		nc1_testing_res = 0;
		nc2_testing_res = 0;
		
		printf("\n");
		
		/*
		f(x) = x^3 - 25x
		*/
		
		handle = dlopen("./functions/x^3-25x.so", RTLD_LAZY);
		
		p_nc1_test = dlsym(handle, "f");
		p_nc2_test = dlsym(handle, "f");
		
		a = -11;
		b = 314;
		area = 2429058206.25;
		
		nc1_testing = newton_cotes_1(p_nc1_test, a, b);
		nc2_testing = newton_cotes_2(p_nc2_test, a, b);
		
		nc1_testing_ptr = ((float *) &nc1_testing);
		nc2_testing_ptr = ((double *) &nc2_testing);
		
		nc1_testing_res = * nc1_testing_ptr;
		nc2_testing_res = * nc2_testing_ptr;
		
		printf("Integral 			f(x) = x^3-25x von %d bis %d mit Newton Cotes 1 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc1_testing_res, area, (nc1_testing_res-area),((nc1_testing_res/area)*100)-100);
		printf("Integral 			f(x) = x^3-25x von %d bis %d mit Newton Cotes 2 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc2_testing_res, area, (nc2_testing_res-area), ((nc2_testing_res/area)*100)-100);
		
		nc1_testing_res = 0;
		nc2_testing_res = 0;
		
		for (int i = a; i < b; i++) {
			nc1_testing = newton_cotes_1(p_nc1_test, i, (i+1));
			nc2_testing = newton_cotes_2(p_nc2_test, i, (i+1));
		
			float * nc1_nqtest_ptr = ((float *) &nc1_testing);
			nc1_testing_res += * nc1_nqtest_ptr; 
	
			double * nc2_nqtest_ptr = ((double *) &nc2_testing);
			nc2_testing_res += * nc2_nqtest_ptr; 
		}
		
		printf("Summierte Numerische Quadratur	f(x) = x^3-25x von %d bis %d mit Newton Cotes 1 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc1_testing_res, area, (nc1_testing_res-area), ((nc1_testing_res/area)*100)-100);
		printf("Summierte Numerische Quadratur	f(x) = x^3-25x von %d bis %d mit Newton Cotes 2 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc2_testing_res, area, (nc2_testing_res-area), ((nc2_testing_res/area)*100)-100);
		
		nc1_testing_res = 0;
		nc2_testing_res = 0;
		
		printf("\n");
		
		/*
		f(x) = x^4-21x^3+52x^2+480x-512
		*/
		
		handle = dlopen("./functions/x^4-21x^3+52x^2+480x-512.so", RTLD_LAZY);
		
		p_nc1_test = dlsym(handle, "f");
		p_nc2_test = dlsym(handle, "f");
		
		a = -10;
		b = 10;
		area = 64426.67;
		
		nc1_testing = newton_cotes_1(p_nc1_test, a, b);
		nc2_testing = newton_cotes_2(p_nc2_test, a, b);
		
		nc1_testing_ptr = ((float *) &nc1_testing);
		nc2_testing_ptr = ((double *) &nc2_testing);
		
		nc1_testing_res = * nc1_testing_ptr;
		nc2_testing_res = * nc2_testing_ptr;
		
		printf("Integral 			f(x) = x^4-21x^3+52x^2+480x-512 von %d bis %d mit Newton Cotes 1 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc1_testing_res, area, (nc1_testing_res-area), ((nc1_testing_res/area)*100)-100);
		printf("Integral 			f(x) = x^4-21x^3+52x^2+480x-512 von %d bis %d mit Newton Cotes 2 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc2_testing_res, area, (nc2_testing_res-area), ((nc2_testing_res/area)*100)-100);
		
		nc1_testing_res = 0;
		nc2_testing_res = 0;
		
		for (int i = a; i < b; i++) {
			nc1_testing = newton_cotes_1(p_nc1_test, i, (i+1));
			nc2_testing = newton_cotes_2(p_nc2_test, i, (i+1));
		
			float * nc1_nqtest_ptr = ((float *) &nc1_testing);
			nc1_testing_res += * nc1_nqtest_ptr; 
	
			double * nc2_nqtest_ptr = ((double *) &nc2_testing);
			nc2_testing_res += * nc2_nqtest_ptr; 
		}
		
		printf("Summierte Numerische Quadratur	f(x) = x^4-21x^3+52x^2+480x-512 von %d bis %d mit Newton Cotes 1 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc1_testing_res, area, (nc1_testing_res-area), ((nc1_testing_res/area)*100)-100);
		printf("Summierte Numerische Quadratur	f(x) = x^4-21x^3+52x^2+480x-512 von %d bis %d mit Newton Cotes 2 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc2_testing_res, area, (nc2_testing_res-area), ((nc2_testing_res/area)*100)-100);
		
		nc1_testing_res = 0;
		nc2_testing_res = 0;
		
		printf("\n");
		
		/*
		f(x) = x^5-21x^4+52x^3+480x^2-512x
		*/
		
		handle = dlopen("./functions/x^5-21x^4+52x^3+480x^2-512x.so", RTLD_LAZY);
		
		p_nc1_test = dlsym(handle, "f");
		p_nc2_test = dlsym(handle, "f");
		
		a = -10;
		b = 10;
		area = -520000;
		
		nc1_testing = newton_cotes_1(p_nc1_test, a, b);
		nc2_testing = newton_cotes_2(p_nc2_test, a, b);
		
		nc1_testing_ptr = ((float *) &nc1_testing);
		nc2_testing_ptr = ((double *) &nc2_testing);
		
		nc1_testing_res = * nc1_testing_ptr;
		nc2_testing_res = * nc2_testing_ptr;
		
		printf("Integral 			f(x) = x^5-21x^4+52x^3+480x^2-512x von %d bis %d mit Newton Cotes 1 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc1_testing_res, area, (nc1_testing_res-area),((nc1_testing_res/area)*100)-100);
		printf("Integral 			f(x) = x^5-21x^4+52x^3+480x^2-512x von %d bis %d mit Newton Cotes 2 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc2_testing_res, area, (nc2_testing_res-area), ((nc2_testing_res/area)*100)-100);
		
		nc1_testing_res = 0;
		nc2_testing_res = 0;
		
		for (int i = a; i < b; i++) {
			nc1_testing = newton_cotes_1(p_nc1_test, i, (i+1));
			nc2_testing = newton_cotes_2(p_nc2_test, i, (i+1));
		
			float * nc1_nqtest_ptr = ((float *) &nc1_testing);
			nc1_testing_res += * nc1_nqtest_ptr; 
	
			double * nc2_nqtest_ptr = ((double *) &nc2_testing);
			nc2_testing_res += * nc2_nqtest_ptr; 
		}
		
		printf("Summierte Numerische Quadratur	f(x) = x^5-21x^4+52x^3+480x^2-512x von %d bis %d mit Newton Cotes 1 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc1_testing_res, area, (nc1_testing_res-area), ((nc1_testing_res/area)*100)-100);
		printf("Summierte Numerische Quadratur	f(x) = x^5-21x^4+52x^3+480x^2-512x von %d bis %d mit Newton Cotes 2 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc2_testing_res, area, (nc2_testing_res-area), ((nc2_testing_res/area)*100)-100);
		
		nc1_testing_res = 0;
		nc2_testing_res = 0;
		
		printf("\n");
		
		/*
		f(x) = e^x
		*/
		
		handle = dlopen("./functions/e^x.so", RTLD_LAZY);
		
		p_nc1_test = dlsym(handle, "f");
		p_nc2_test = dlsym(handle, "f");
		
		a = -1;
		b = 1;
		area = 2.3504;
		
		nc1_testing = newton_cotes_1(p_nc1_test, a, b);
		nc2_testing = newton_cotes_2(p_nc2_test, a, b);
		
		nc1_testing_ptr = ((float *) &nc1_testing);
		nc2_testing_ptr = ((double *) &nc2_testing);
		
		nc1_testing_res = * nc1_testing_ptr;
		nc2_testing_res = * nc2_testing_ptr;
		
		printf("Integral 			f(x) = e^x von %d bis %d mit Newton Cotes 1 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc1_testing_res, area, (nc1_testing_res-area),((nc1_testing_res/area)*100)-100);
		printf("Integral 			f(x) = e^x von %d bis %d mit Newton Cotes 2 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc2_testing_res, area, (nc2_testing_res-area), ((nc2_testing_res/area)*100)-100);
		
		nc1_testing_res = 0;
		nc2_testing_res = 0;
		
		for (int i = a; i < b; i++) {
			nc1_testing = newton_cotes_1(p_nc1_test, i, (i+1));
			nc2_testing = newton_cotes_2(p_nc2_test, i, (i+1));
		
			float * nc1_nqtest_ptr = ((float *) &nc1_testing);
			nc1_testing_res += * nc1_nqtest_ptr; 
	
			double * nc2_nqtest_ptr = ((double *) &nc2_testing);
			nc2_testing_res += * nc2_nqtest_ptr; 
		}
		
		printf("Summierte Numerische Quadratur	f(x) = e^x von %d bis %d mit Newton Cotes 1 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc1_testing_res, area, (nc1_testing_res-area), ((nc1_testing_res/area)*100)-100);
		printf("Summierte Numerische Quadratur	f(x) = e^x von %d bis %d mit Newton Cotes 2 = %f, mathematische Flaeche: %f, Differenz: %f, Fehler: %f%%\n", a, b, nc2_testing_res, area, (nc2_testing_res-area), ((nc2_testing_res/area)*100)-100);
	}

	return EXIT_SUCCESS;
}