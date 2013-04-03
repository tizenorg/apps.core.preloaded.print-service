
#include <stdlib.h>
#include <unistd.h>
#include <getppd.h>

static char **gargv;
static pt_db *database;

void help(void)
{
	fprintf(stderr, "Extract ppd file by model name:\n");
	fprintf(stderr, "Usage: %s -m <model> -i <database>\n\n", gargv[0]);
	fprintf(stderr, "Extract ppd file by product name:\n");
	fprintf(stderr, "Usage: %s -p <product> -i <database>\n\n", gargv[0]);
	fprintf(stderr, "Extract ppd file either by model name or product name:\n");
	fprintf(stderr, "Usage: %s -a <key> -i <database>\n", gargv[0]);
}

void clean_all_resources(void)
{
	pt_free_db(database);
}

config *initialization(int argc, char **argv)
{
	int opt;
	bool product = false;
	bool model = false;
	bool drvfile = false;

	int res = atexit(clean_all_resources);
	if(res) {
		perror("atexit() failed");
	}

	config *cfg = malloc(sizeof(config));
	if (!cfg) {
		fprintf(stderr, "Can't allocate memory\n");
		exit(EXIT_FAILURE);
	}
	bzero(cfg, sizeof(config));

	while ((opt = getopt(argc, argv, "i:m:p:a:r:")) != -1) {
		switch (opt) {
		case 'i':
			if (!drvfile) {
				cfg->drvstr = strdup(optarg);
				if (!cfg->drvstr) {
					fprintf(stderr, "Can't duplicate string with strdup()\n");
					exit(EXIT_FAILURE);
				}
				drvfile = true;
			} else {
				fprintf(stderr, "Don't support extraction from multiple databases\n");
				fprintf(stderr, "Do not provide some \'-i\'' options\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'p':
			if (!product) {
				cfg->product = strdup(optarg);
				if (!cfg->product)
				{
					fprintf(stderr, "Can't duplicate string with strdup()\n");
					exit(EXIT_FAILURE);
				}
				product = true;
			} else {
				fprintf(stderr, "Supported extraction only of one ppd file by product\n");
				fprintf(stderr, "Do not provide some \'-p\'' options\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'm':
			if (!model) {
				cfg->model = strdup(optarg);
				if (!cfg->model)
				{
					fprintf(stderr, "Can't duplicate string with strdup()\n");
					exit(EXIT_FAILURE);
				}
				model = true;
			} else {
				fprintf(stderr, "Supported extraction only of one ppd file by modelname\n");
				fprintf(stderr, "Do not provide some \'-m\'' options\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'r':
		case 'a':
			if (!model) {
				cfg->model = strdup(optarg);
				if (!cfg->model) {
					fprintf(stderr, "Can't allocate memory\n");
					exit(EXIT_FAILURE);
				}
				model = true;
			} else {
				fprintf(stderr, "Supported extraction only of one ppd file by modelname\n");
				fprintf(stderr, "Do not provide some \'-m\'' options\n");
				exit(EXIT_FAILURE);
			}

			if (!product) {
				cfg->product = strdup(optarg);
				if (!cfg->product) {
					fprintf(stderr, "Can't duplicate string with strdup()\n");
					exit(EXIT_FAILURE);
				}
				product = true;
			} else {
				fprintf(stderr, "Supported extraction only of one ppd file by product\n");
				fprintf(stderr, "Do not provide some \'-p\'' options\n");
				exit(EXIT_FAILURE);
			}
			break;
		default:				/* '?' */
			help();
			exit(EXIT_FAILURE);
		}
	}

	cfg->find_product = product;
	cfg->find_model = model;

	if (!cfg->drvstr) {
		fprintf(stderr, "drv file is not provided\n");
		exit(EXIT_FAILURE);
	}

	return cfg;
}

int main(int argc, char **argv)
{
	gargv = argv;
	config *cfg = initialization(argc, argv);
	if (!cfg) {
		fprintf(stderr, "Unexpected error\n");
		exit(EXIT_FAILURE);
	}

	if(!cfg->drvstr) {
		fprintf(stderr, "drvstr is NULL\n");
		exit(EXIT_FAILURE);
	}

	database = pt_create_db(cfg->drvstr);
	if(!database) {
		fprintf(stderr, "database is NULL\n");
		exit(EXIT_FAILURE);
	}

	char *output = NULL;

	if (cfg->find_model && !cfg->find_product) {
		output = pt_extract_ppd(database, cfg->model, PT_SEARCH_MODEL);
		if(!output) {
			fprintf(stderr, "Can't find suitable ppd file by model\n");
			exit(EXIT_FAILURE);
		}
	} else if (cfg->find_product && !cfg->find_model) {
		output = pt_extract_ppd(database, cfg->product, PT_SEARCH_PROD);
		if(!output) {
			fprintf(stderr, "Can't find suitable ppd file by product\n");
			exit(EXIT_FAILURE);
		}
	} else if (cfg->find_model && cfg->find_product) {
		if (strcmp(cfg->product, cfg->model)) {
			fprintf(stderr, "Supported extraction only of one ppd file\n");
			fprintf(stderr, "Do not provide mixed \'-m\',\'-p\',\'-a\' with different names\n");
			exit(EXIT_FAILURE);
		}

		output = pt_extract_ppd(database, cfg->model, PT_SEARCH_MODEL);
		if (!output) {
			output = pt_extract_ppd(database, cfg->product, PT_SEARCH_PROD);
			if(!output) {
				fprintf(stderr, "Can't find suitable ppd file\n");
				exit(EXIT_FAILURE);
			}
		}
	} else {
		help();
		exit(EXIT_FAILURE);
	}

	printf("%s", output);
	exit(EXIT_SUCCESS);
}