#include <stdlib.h>
#include <stdio.h>

int testc_faux_ini_good(void) {

	char *path = NULL;

	path = getenv("FAUX_INI_PATH");
	if (path)
		printf("Env var is [%s]\n", path);
	return 0;
}


int testc_faux_ini_bad(void) {

	printf("Some debug information here\n");
	return -1;
}

int testc_faux_ini_signal(void) {

	char *p = NULL;

	printf("%s\n", p);
	return -1;
}
