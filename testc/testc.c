#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/wait.h>

#if WITH_INTERNAL_GETOPT
#include "libc/getopt.h"
#else
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#endif

#if HAVE_LOCALE_H
#include <locale.h>
#endif
#if HAVE_LANGINFO_CODESET
#include <langinfo.h>
#endif

#include "faux/faux.h"
#include "faux/str.h"
#include "faux/list.h"

#ifndef VERSION
#define VERSION 1.0.0
#endif
#define QUOTE(t) #t
#define version(v) printf("%s\n", v)

// Version of testc API (not version of programm)
#define TESTC_VERSION_MAJOR_DEFAULT 1
#define TESTC_VERSION_MINOR_DEFAULT 0
#define SYM_TESTC_VERSION_MAJOR "testc_version_major"
#define SYM_TESTC_VERSION_MINOR "testc_version_minor"
#define SYM_TESTC_MODULE "testc_module"


// Command line options */
struct opts_s {
	int debug;
	faux_list_t *so_list;
};

typedef struct opts_s opts_t;

static opts_t *opts_parse(int argc, char *argv[]);
static void opts_free(opts_t *opts);
static void help(int status, const char *argv0);
static int exec_test(int (*test_sym)(void));


int main(int argc, char *argv[]) {

	opts_t *opts = NULL;
	faux_list_node_t *iter = NULL;
	char *so = NULL;
	// Return value will be negative on any error or failed test.
	// It doesn't mean that any error will break the processing.
	// The following var is error counter.
	unsigned int total_errors = 0;
	unsigned int total_modules = 0;
	unsigned int total_tests = 0;

#if HAVE_LOCALE_H
	// Set current locale
	setlocale(LC_ALL, "");
#endif

	// Parse command line options
	opts = opts_parse(argc, argv);
	if (!opts) {
		fprintf(stderr, "Error: Can't parse command line options\n");
		return -1;
	}

	iter = faux_list_head(opts->so_list);
	while ((so = faux_list_each(&iter))) {
		void *so_handle = NULL;
		// Module symbols
		unsigned char testc_version_major = TESTC_VERSION_MAJOR_DEFAULT;
		unsigned char testc_version_minor = TESTC_VERSION_MINOR_DEFAULT;
		unsigned char *testc_version = NULL;
		const char *(*testc_module)[2] = NULL;
		// Module counters
		unsigned int module_tests = 0;
		unsigned int module_errors = 0;

		printf("--------------------------------------------------------------------------------\n");

		so_handle = dlopen(so, RTLD_LAZY | RTLD_LOCAL);
		if (!so_handle) {
			fprintf(stderr, "Error: Can't open module \"%s\"... Skipped\n", so);
			total_errors++;
			continue;
		}

		// Get testc API version from module
		testc_version = dlsym(so_handle, SYM_TESTC_VERSION_MAJOR);
		if (!testc_version) {
			fprintf(stderr, "Warning: Can't get API version for module \"%s\"... Use defaults\n", so);
		} else {
			testc_version_major = *testc_version;
			testc_version = dlsym(so_handle, SYM_TESTC_VERSION_MINOR);
			if (!testc_version) {
				fprintf(stderr, "Warning: Can't get API minor version for module \"%s\"... Use '0'\n", so);
				testc_version_minor = 0;
			} else {
				testc_version_minor = *testc_version;
			}
		}
		if ((testc_version_major > TESTC_VERSION_MAJOR_DEFAULT) ||
			((testc_version_major == TESTC_VERSION_MAJOR_DEFAULT) &&
			(testc_version_minor >TESTC_VERSION_MINOR_DEFAULT))) {
			fprintf(stderr, "Error: Unsupported API v%u.%u for module \"%s\"... Skipped\n", 
				testc_version_major, testc_version_minor, so);
			continue;
		}

		testc_module = dlsym(so_handle, SYM_TESTC_MODULE);
		if (!testc_module) {
			fprintf(stderr, "Error: Can't get test list for module \"%s\"... Skipped\n", so);
			total_errors++;
			continue;
		}

		total_modules++;
		printf("Processing module \"%s\" v%u.%u ...\n", so,
			testc_version_major, testc_version_minor);

		while ((*testc_module)[0]) {
			const char *test_name = NULL;
			const char *test_desc = NULL;
			int (*test_sym)(void);
			int wstatus = 0;
			char *result_str = NULL;
			char *attention_str = NULL;

			test_name = (*testc_module)[0];
			test_desc = (*testc_module)[1];
			if (!test_desc)
				test_desc = "";
			testc_module++;
			module_tests++;

			test_sym = (int (*)(void))dlsym(so_handle, test_name);
			if (!test_sym) {
				fprintf(stderr, "Error: Can't find symbol \"%s\"... Skipped\n", test_name);
				module_errors++;
				continue;
			}

			wstatus = exec_test(test_sym);

			if (WIFEXITED(wstatus)) {
				if (WEXITSTATUS(wstatus) == 0) {
					result_str = faux_str_dup("success");
					attention_str = faux_str_dup("");
				} else {
					result_str = faux_str_sprintf("failed (%d)",
						(int)((signed char)((unsigned char)WEXITSTATUS(wstatus))));
					attention_str = faux_str_dup("(!) ");
					module_errors++;
				}
			} else if (WIFSIGNALED(wstatus)) {
				result_str = faux_str_sprintf("terminated (%d)",
					WTERMSIG(wstatus));
				attention_str = faux_str_dup("[!] ");
				module_errors++;
			} else {
				result_str = faux_str_dup("unknown");
				attention_str = faux_str_dup("[!] ");
				module_errors++;
			}

			printf("%sTest #%03u %s() %s: %s\n", attention_str, module_tests, test_name, test_desc, result_str);
			faux_str_free(result_str);
			faux_str_free(attention_str);
		}


		dlclose(so_handle);
		so_handle = NULL;

		printf("Module tests: %u\n", module_tests);
		printf("Module errors: %u\n", module_errors);

		total_tests += module_tests;
		total_errors += module_errors;

	}

	opts_free(opts);

	// Total statistics
	printf("================================================================================\n");
	printf("Total modules: %u\n", total_modules);
	printf("Total tests: %u\n", total_tests);
	printf("Total errors: %u\n", total_errors);

	if (total_errors > 0)
		return -1;
	return 0;
}


static int exec_test(int (*test_sym)(void)) {

	pid_t pid = -1;
	int wstatus = -1;

	pid = fork();
	assert(pid != -1);
	if (pid == -1)
		return -1;

	// Child
	if (pid == 0)
		_exit(test_sym());

	// Parent
	while (waitpid(pid, &wstatus, 0) != pid);

	return wstatus;
}


static void opts_free(opts_t *opts) {

	assert(opts);
	if (!opts)
		return;

	faux_list_free(opts->so_list);
	faux_free(opts);
}


static opts_t *opts_new(void) {

	opts_t *opts = NULL;

	opts = faux_zmalloc(sizeof(*opts));
	assert(opts);
	if (!opts)
		return NULL;

	opts->debug = BOOL_FALSE;

	// Members of list are static strings from argv so don't free() it
	opts->so_list = faux_list_new(BOOL_FALSE, BOOL_TRUE,
		(faux_list_cmp_fn)strcmp, NULL, NULL);
	if (!opts->so_list) {
		opts_free(opts);
		return NULL;
	}

	return opts;
}


static opts_t *opts_parse(int argc, char *argv[]) {

	opts_t *opts = NULL;

	static const char *shortopts = "hvd";
#ifdef HAVE_GETOPT_LONG
	static const struct option longopts[] = {
		{"help",	0, NULL, 'h'},
		{"version",	0, NULL, 'v'},
		{"debug",	0, NULL, 'd'},
		{NULL,		0, NULL, 0}
	};
#endif

	opts = opts_new();
	if (!opts)
		return NULL;

	optind = 1;
	while (1) {
		int opt;
#ifdef HAVE_GETOPT_LONG
		opt = getopt_long(argc, argv, shortopts, longopts, NULL);
#else
		opt = getopt(argc, argv, shortopts);
#endif
		if (-1 == opt)
			break;
		switch (opt) {
		case 'd':
			opts->debug = BOOL_TRUE;
			break;
		case 'h':
			help(0, argv[0]);
			exit(0);
			break;
		case 'v':
			version(VERSION);
			exit(0);
			break;
		default:
			help(-1, argv[0]);
			exit(-1);
			break;
		}
	}

	if (optind < argc) {
		int i = 0;
		for (i = optind; i < argc; i++)
			faux_list_add(opts->so_list, argv[i]);
	} else {
		help(-1, argv[0]);
		exit(-1);
	}

	return opts;
}


static void help(int status, const char *argv0) {

	const char *name = NULL;

	if (!argv0)
		return;

	// Find the basename
	name = strrchr(argv0, '/');
	if (name)
		name++;
	else
		name = argv0;

	if (status != 0) {
		fprintf(stderr, "Try `%s -h' for more information.\n",
			name);
	} else {
		printf("Usage: %s [options] <so_object> [so_object] ...\n", name);
		printf("Unit test helper for C code.\n");
		printf("Options:\n");
		printf("\t-v, --version\tPrint version.\n");
		printf("\t-h, --help\tPrint this help.\n");
		printf("\t-d, --debug\tDebug mode. Don't daemonize.\n");
	}
}
