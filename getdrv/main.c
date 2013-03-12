/*
*	Printservice
*
* Copyright 2012  Samsung Electronics Co., Ltd

* Licensed under the Flora License, Version 1.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at

* http://floralicense.org/license/

* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

#include "main.h"

static FILE *FD = NULL;
static FILE *WR = NULL;
static char *ofile = NULL;
static char *ifile = "stripped.drv";
static char *glob = NULL;

typedef enum _pt_drv_stringtype_e {
	PT_DRVSTRING_COMMENT = 0,
	PT_DRVSTRING_BLOCKSTART,
	PT_DRVSTRING_BLOCKEND,
	PT_DRVSTRING_CONTENT,
} pt_drv_stringtype_e;

__attribute__((weak)) void init_signals()
{
	return;
}

bool init(void)
{
	init_signals();

	list = stackinit();

	if (glob == NULL) {
		fprintf(stderr, "error: please provide model name!\n");
		return false;
	} else {
		char *sentinel = "\"";
		char *temp = calloc(1,strlen(glob)+3);
		memcpy(temp,sentinel,1);
		memcpy(temp+1,glob,strlen(glob));
		memcpy(temp+strlen(glob)+1,sentinel,1);
		glob = strdup(temp);
		PT_IF_FREE_MEM(temp);
	}

	FD = fopen(ifile, "r");
	if (FD == NULL) {
		fprintf(stderr, "cannot open %s: %s\n", ifile, strerror(errno));
		return false;
	}

	char temp_file[] = "tmpfileXXXXXX";
	int fd = -1;
	umask(022);
	fd = mkstemp(temp_file);
	if (fd == -1) {
		fprintf(stderr,"Cannot creat temp file");
		return false;
	}
	close(fd);
	ofile = strdup(temp_file);
	WR = fopen(ofile, "w");
	if (WR == NULL) {
		fprintf(stderr, "cannot open %s: %s\n", ofile, strerror(errno));
		return false;
	}

	return true;
}

void cleanup(void)
{
	if (unlink(ofile) == -1) {
		perror("Cannot remove temporary drv file");
		exit(EXIT_FAILURE);
	}

	if (fclose(WR) != 0) {
		perror("Cannot close temporary drv file");
		exit(EXIT_FAILURE);
	}

	return;
}

pt_drv_stringtype_e get_string_type(const char *line)
{

	pt_drv_stringtype_e ret = PT_DRVSTRING_CONTENT;

	if (strncmp(line, "//",2) == 0) {
		ret = PT_DRVSTRING_COMMENT;
	} else if (strncmp(line, "{\n", 2) == 0) {
		ret = PT_DRVSTRING_BLOCKSTART;
	} else if (strncmp(line, "}\n", 2) == 0) {
		ret = PT_DRVSTRING_BLOCKEND;
	} else {
		ret = PT_DRVSTRING_CONTENT;
	}

	return ret;
}

bool execute_ppdc(void)
{
	int status;

	switch (fork()) {
	case -1:// -1, fork failure
		perror("Failed to fork");
		return false;
	case 0:// 0, this is the child process
		fprintf(stderr,"ppdc execute\n");
		if ((execlp("ppdc", "ppdc", "-v", "-d", ".", ofile, NULL)) == -1) {
			perror("Cannot exec ppdc compiler");
			return false;
		}
		break;
	default:// > 0, parent process and the PID is the child's PID
		wait(&status);
		break;
	}

	return true;
}

static bool enlarge_templine(char **temp, int *psize, int *plen)
{
	int size = *psize;
	int len = *plen;

	if (size < len) {
		len--;
		for (uint i = 1; i <= sizeof(int); i <<= 1) {
			len |= len >> i;
		}
		len++;
		size = 2 * len;
		void *t = realloc(*temp, size);
		PT_RETV_IF(t == NULL, false, "Cannot reallocate memory\n");
		*psize = size;
		*plen = len;
		*temp = t;
	}

	return true;
}

static bool extract_model(void)
{
	int len = 0;
	int size = -1;
	char *line = NULL;
	char *tmpline = NULL;
	size_t linelen = 0;
	bool ret = false;

	tmpline = calloc(1, 1);
	PT_RETV_IF(tmpline == NULL, false, "Cannot allocate memory\n");

	while (getline(&line, &linelen, FD) != -1) {
		len = linelen + strlen(tmpline);
		ret = enlarge_templine(&tmpline, &size, &len);
		if (ret == false) {
			PT_IF_FREE_MEM(tmpline);
			return false;
		}

		pt_drv_stringtype_e ret = PT_DRVSTRING_COMMENT;
		ret = get_string_type(line);

		switch (ret) {
		case PT_DRVSTRING_COMMENT:
			continue;
			break;
		case PT_DRVSTRING_BLOCKSTART:
			if (tmpline[0] != 0) {
				list->head->next->data = strdup(tmpline);
				tmpline[0] = 0;
			}
			list->push(" ");
			break;
		case PT_DRVSTRING_BLOCKEND:
			list->head->next->data = strdup(tmpline);
			if (strstr(tmpline, "ModelName ") != NULL || strstr(tmpline, "Attribute \"Product\"") != NULL) {
				if (strcasestr(tmpline, glob) != NULL) {
					char *t = NULL;
					while ((t = list->upsh())) {
						fprintf(WR, "%s", t);
					}

					if (fflush(WR) == EOF) {
						perror("Cannot flush temporary file");
						PT_IF_FREE_MEM(tmpline);
						return false;
					}
					ret = execute_ppdc();
					PT_IF_FREE_MEM(tmpline);
					return ret;
				}
			}
			tmpline[0] = 0;
			list->pop();
			break;
		case PT_DRVSTRING_CONTENT:
			strcat(tmpline, line);
			break;
		}
	}
	PT_IF_FREE_MEM(tmpline);

	return false;
}

int main(int argc, char **argv)
{
	int opt;

	while ((opt = getopt(argc, argv, "i:r:")) != -1) {
		switch (opt) {
		case 'i':
			ifile = optarg;
			break;
		case 'r':
			glob = optarg;
			break;
		default:				/* '?' */
			fprintf(stderr, "Usage: %s -r model name [-i input file]\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if (!init()) {
		return EXIT_FAILURE;
	}

	if (!extract_model()) {
		fprintf(stderr, "model %s not found\n", glob);
		cleanup();

		return EXIT_FAILURE;
	}

	cleanup();
	return EXIT_SUCCESS;
}
