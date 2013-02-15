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

#include <ctype.h>
#include <stdbool.h>
#include "pt_debug.h"
#include "pt_common.h"
#include "pt_optionmapping.h"

typedef ppd_choice_t *OPTIONCUBE[PT_QUALITY_MAX][PT_PAPER_MAX][PT_GRAYSCALE_MAX];

static int aQuality;
static int aPaper;
static int aColor;
static int aPaperSize;
static ppd_option_t *paper_sizes;
static OPTIONCUBE *optionCube;

int pt_get_print_option_default_papersize(void);

#include "pt_optionkeywords.c"

#ifdef PT_OPTIONCUBE_TEST_PRINT
static const char *papernames[] = {
	"Plain  ",
	"Glossy ",
	"Photo  ",
	"Any    "
};

static const char *qualitynames[] = {
	"Draft    ",
	"Standard ",
	"High     ",
	"Any      "
};

static const char *colornames[] = {
	"Gray     ",
	"Color    ",
	"Any      "
};

#endif

static void parse_pagesize(ppd_option_t *opt)
{
	int index;
	paper_sizes = opt;
	for (index = paper_sizes->num_choices-1; index > 0; --index)
		if (!strcasecmp(paper_sizes->choices[index].choice, paper_sizes->defchoice)) {
			break;
		}
	aPaperSize = pt_get_print_option_default_papersize();
	return;
}

static void parse_quality(ppd_option_t *opt)
{
	int i;
	for (i = opt->num_choices-1; i >= 0; i--) {
		const pt_choice_keyword *lookingup = pt_quality_words;
		while (lookingup->keyword != NULL) {
			if (strcasecmp(lookingup->keyword, opt->choices[i].choice) == 0) {
				optionCube[0][lookingup->quality][lookingup->papertype][lookingup->grayscale] = &opt->choices[i];
				PT_DEBUG("quality choice found: %s - q:%d p:%d c:%d", lookingup->keyword, lookingup->quality, lookingup->papertype, lookingup->grayscale);
				break;
			} else {
				lookingup++;
			}
		}
		if ((strcasecmp(opt->choices[i].choice, opt->defchoice) == 0) && (lookingup->keyword != NULL)) {
			if (lookingup->quality != PT_QUALITY_ANY) {
				aQuality = lookingup->quality;
			}
			if (lookingup->papertype != PT_PAPER_ANY) {
				aPaper = lookingup->papertype;
			}
		}
	}

	return;
}

static void parse_printquality(ppd_option_t *opt)
{
	int i;
	for (i = 0; i < opt->num_choices; i++) {
		if (strcasecmp("BestPhoto", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
		} else if (strcasecmp("TextImage", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
		} else if (strcasecmp("Draft", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_DRAFT][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
		}
	}
	// setup the default value:
	if (strcasecmp("BestPhoto", opt->defchoice) == 0) {
		aQuality = PT_QUALITY_HIGH;
	} else if (strcasecmp("Draft", opt->defchoice) == 0) {
		aQuality = PT_QUALITY_DRAFT;
	} else {
		aQuality = PT_QUALITY_STANDARD;
	}
	return;
}

static void parse_economode(ppd_option_t *opt)
{
	int i;
	for (i = 0; i < opt->num_choices; i++) {
		if (strcasecmp("FALSE", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
			aQuality = PT_QUALITY_HIGH;
//		} else if (strcasecmp("PrinterDefault", opt->choices[i].choice) == 0) {
//			optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
		} else if (strcasecmp("TRUE", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_DRAFT][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
		}
	}
	// setup the default value:
	if (strcasecmp("FALSE", opt->defchoice) == 0) {
		aQuality = PT_QUALITY_HIGH;
	} else {
		aQuality = PT_QUALITY_DRAFT;
	}
	return;
}

static void parse_hpeconomode(ppd_option_t *opt)
{
	int i;
	for (i = 0; i < opt->num_choices; i++) {
		if (strcasecmp("False", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
			aQuality = PT_QUALITY_HIGH;
		} else if (strcasecmp("PrinterDefault", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
		} else if (strcasecmp("True", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_DRAFT][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
		}
	}
	// setup the default value:
	if (strcasecmp("False", opt->defchoice) == 0) {
		aQuality = PT_QUALITY_HIGH;
	} else if (strcasecmp("True", opt->defchoice) == 0) {
		aQuality = PT_QUALITY_DRAFT;
	} else {
		aQuality = PT_QUALITY_STANDARD;
	}
	return;
}

static void parse_hpeconomic(ppd_option_t *opt)
{
	int i;
	for (i = 0; i < opt->num_choices; i++) {
		if (strcasecmp("FALSE", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
			aQuality = PT_QUALITY_HIGH;
//		} else if (strcasecmp("PrinterDefault", opt->choices[i].choice) == 0) {
//			optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
		} else if (strcasecmp("TRUE", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_DRAFT][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
		}
	}
	// setup the default value:
	if (strcasecmp("FALSE", opt->defchoice) == 0) {
		aQuality = PT_QUALITY_HIGH;
	} else {
		aQuality = PT_QUALITY_DRAFT;
	}
	return;
}

static void parse_hpclean(ppd_option_t *opt)
{
	int i;
	for (i = 0; i < opt->num_choices; i++) {
		if (strcasecmp("TRUE", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
		} else if (strcasecmp("FALSE", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
		}
	}
	// setup the default value:
	if (strcasecmp("TRUE", opt->defchoice) == 0) {
		aQuality = PT_QUALITY_HIGH;
	} else {
		aQuality = PT_QUALITY_STANDARD;
	}
	return;
}

static void parse_hpprintquality(ppd_option_t *opt)
{
	int i;
	for (i = 0; i < opt->num_choices; i++) {
		if (strcasecmp("FastRes1200", opt->choices[i].choice) == 0 ||
				strcasecmp("1200x1200dpi", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
		} else if (strcasecmp("600dpi", opt->choices[i].choice) == 0 ||
				   strcasecmp("600x600dpi", opt->choices[i].choice) == 0 ||
				   strcasecmp("ImageRet3600", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
		}
	}
	// setup the default value:
	if (strcasecmp("FastRes1200", opt->defchoice) == 0 || strcasecmp("1200x1200dpi", opt->defchoice)) {
		aQuality = PT_QUALITY_HIGH;
	} else {
		aQuality = PT_QUALITY_STANDARD;
	}
	return;
}

static void parse_hpoutputquality(ppd_option_t *opt)
{
	int i;
	for (i = 0; i < opt->num_choices; i++) {
		if (strcasecmp("Best", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
		} else if (strcasecmp("Normal", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
		} else if (strcasecmp("Fast", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_DRAFT][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
		}
	}
	// setup the default value:
	if (strcasecmp("Best", opt->defchoice) == 0) {
		aQuality = PT_QUALITY_HIGH;
	} else if (strcasecmp("Fast", opt->defchoice) == 0) {
		aQuality = PT_QUALITY_DRAFT;
	} else {
		aQuality = PT_QUALITY_STANDARD;
	}
	return;
}

static void parse_mediatype(ppd_option_t *opt)
{
	int i;
	aPaper = PT_PAPER_NORMAL;
	for (i = opt->num_choices-1; i >= 0; i--) {
		const pt_choice_keyword *lookingup = pt_mediatype_words;
		while (lookingup->keyword != NULL) {
			if (strcasecmp(lookingup->keyword, opt->choices[i].choice) == 0) {
				optionCube[0][lookingup->quality][lookingup->papertype][lookingup->grayscale] = &opt->choices[i];
				PT_DEBUG("mediatype choice found: %s - q:%d p:%d c:%d", lookingup->keyword, lookingup->quality, lookingup->papertype, lookingup->grayscale);
				break;
			} else {
				lookingup++;
			}
		}
		if ((strcasecmp(opt->choices[i].choice, opt->defchoice) == 0) && (lookingup->keyword != NULL)) {
			if (lookingup->papertype != PT_PAPER_ANY) {
				aPaper = lookingup->papertype;
			}
		}
	}
	return;
}

static void parse_resolution(ppd_option_t *opt)
{
	int rv;
	int x;
	int y;
	char qlty[8];

	int i;
	for (i = 0; i < opt->num_choices; i++) {
		rv = sscanf(opt->choices[i].choice, "%d%d_%8s", &x, &y, qlty);
		switch (rv) {
		case 3:
			if (isdigit((int)qlty[0]) != 0) {
				if (strcasecmp("4", qlty) == 0) {
					optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
					aQuality = PT_QUALITY_HIGH;
				} else if (strcasecmp("2", qlty) == 0) {
					optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
				} else if (strcasecmp("1", qlty) == 0) {
					optionCube[0][PT_QUALITY_DRAFT][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
				}
			} else if (isalpha((int)qlty[0]) != 0) {
				if (strcasecmp("Best", qlty) == 0) {
					optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
					aQuality = PT_QUALITY_HIGH;
				} else if (strcasecmp("Normal", qlty) == 0) {
					optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
				} else if (strcasecmp("Draft", qlty) == 0) {
					optionCube[0][PT_QUALITY_DRAFT][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
				}
			}
			break;
		case 2:
			if (x > y) {
				if (y == 600) {
					if (optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY] == NULL) {
						optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
						aQuality = PT_QUALITY_HIGH;
					} else {
						optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
					}
				}
			} else if (x == y) { // it is important to check that all choices is available
				if (x == 1200) {
					if (optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY] != NULL) {
						optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = \
								optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY];
					}
					optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
					aQuality = PT_QUALITY_HIGH;
				} else if (x == 600) {
					optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
				} else if (x == 300) {
					optionCube[0][PT_QUALITY_DRAFT][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
				}
			}
			break;
		case 1:
			if (x == 1200) {
				optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
				aQuality = PT_QUALITY_HIGH;
			} else if (x == 600) {
				optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
			} else if (x == 300) {
				optionCube[0][PT_QUALITY_DRAFT][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
			}
			break;
		case 0:
			rv = sscanf(opt->choices[i].choice, "%8s", qlty);
			if (isalpha(qlty[0]) != 0) {
				if (strcasecmp("Best", qlty) == 0) {
					optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
					aQuality = PT_QUALITY_HIGH;
				} else if (strcasecmp("Normal", qlty) == 0) {
					optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
				} else if (strcasecmp("Draft", qlty) == 0) {
					optionCube[0][PT_QUALITY_DRAFT][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
				}
			}
			break;
		default:
			break;
		}
	}

	return;
}

static void parse_ink(ppd_option_t *opt)
{
	int i;
	for (i = 0; i < opt->num_choices; i++) {
		if (strcasecmp("COLOR", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_COLOUR] = &opt->choices[i];
		} else if (strcasecmp("MONO", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_GRAYSCALE] = &opt->choices[i];
		}
	}
	// setup the default value:
	if (strcasecmp("COLOR", opt->defchoice) == 0) {
		aColor = PT_GRAYSCALE_COLOUR;
	} else {
		aColor = PT_GRAYSCALE_GRAYSCALE;
	}
	return;
}

static void parse_color(ppd_option_t *opt)
{
	int i;
	for (i = 0; i < opt->num_choices; i++) {
		if (strcasecmp("Color", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_COLOUR] = &opt->choices[i];
		} else if (strcasecmp("Grayscale", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_GRAYSCALE] = &opt->choices[i];
		} else if (strcasecmp("Mono", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_GRAYSCALE] = &opt->choices[i];
		}
	}
	// setup the default value:
	if (strcasecmp("Color", opt->defchoice) == 0) {
		aColor = PT_GRAYSCALE_COLOUR;
	} else {
		aColor = PT_GRAYSCALE_GRAYSCALE;
	}
	return;
}

static void parse_hpcoloroutput(ppd_option_t *opt)
{
	int i;
	for (i = 0; i < opt->num_choices; i++) {
		if (strcasecmp("ColorPrint", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_COLOUR] = &opt->choices[i];
		} else if (strcasecmp("GrayscalePrint", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_GRAYSCALE] = &opt->choices[i];
		}
	}
	// setup the default value:
	if (strcasecmp("ColorPrint", opt->defchoice) == 0) {
		aColor = PT_GRAYSCALE_COLOUR;
	} else {
		aColor = PT_GRAYSCALE_GRAYSCALE;
	}
	return;
}

static void parse_colormode(ppd_option_t *opt)
{
	int i;
	for (i = 0; i < opt->num_choices; i++) {
		if (strcasecmp("TRUE", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_COLOUR] = &opt->choices[i];
		} else if (strcasecmp("FALSE", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_GRAYSCALE] = &opt->choices[i];
		}
	}
	// setup the default value:
	if (strcasecmp("TRUE", opt->defchoice) == 0) {
		aColor = PT_GRAYSCALE_COLOUR;
	} else {
		aColor = PT_GRAYSCALE_GRAYSCALE;
	}
	return;
}

static void parse_colormodel(ppd_option_t *opt)
{
	int i;
	for (i = 0; i < opt->num_choices; i++) {
		if (strcasecmp("BlackWhite", opt->choices[i].choice) == 0 ||
				strcasecmp("CMYGray", opt->choices[i].choice) == 0 ||
				strcasecmp("KGray", opt->choices[i].choice) == 0 ||
				strcasecmp("Gray", opt->choices[i].choice) == 0 ||
				strcasecmp("W", opt->choices[i].choice) == 0) {
			if (optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_GRAYSCALE] == NULL) {
				optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_GRAYSCALE] = &opt->choices[i];
			}
			if (strcasecmp(opt->choices[i].choice, opt->defchoice) == 0) {
				aColor = PT_GRAYSCALE_GRAYSCALE;
			}
		} else if (strcasecmp("CMYK", opt->choices[i].choice) == 0 ||
				   strcasecmp("Color", opt->choices[i].choice) == 0 ||
				   strcasecmp("RGBA", opt->choices[i].choice) == 0 ||
				   strcasecmp("RGB", opt->choices[i].choice) == 0) {
			if (optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_COLOUR] == NULL) {
				optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_COLOUR] = &opt->choices[i];
			}
			if (strcasecmp(opt->choices[i].choice, opt->defchoice) == 0) {
				aColor = PT_GRAYSCALE_COLOUR;
			}
		}
	}

	return;
}

static void parse_hpcolorasgray(ppd_option_t *opt)
{
	int i;
	for (i = 0; i < opt->num_choices; i++) {
		if (strcasecmp("False", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_COLOUR] = &opt->choices[i];
		} else if (strcasecmp("True", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_GRAYSCALE] = &opt->choices[i];
		}
	}
	// setup the default value:
	if (strcasecmp("False", opt->defchoice) == 0) {
		aColor = PT_GRAYSCALE_COLOUR;
	} else {
		aColor = PT_GRAYSCALE_GRAYSCALE;
	}
	return;
}

static void parse_hpprintingrayscale(ppd_option_t *opt)
{
	int i;
	// XXX - It's a way to get gray print by black ink or sum of all inks.
	for (i = 0; i < opt->num_choices; i++) {
		if (strcasecmp("GrayscalePrint", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_GRAYSCALE] = &opt->choices[i];
			if (strcasecmp(opt->choices[i].choice, opt->defchoice) == 0) {
				aColor = PT_GRAYSCALE_GRAYSCALE;
			}
		} else if (strcasecmp("FullSetOfInks", opt->choices[i].choice) == 0) {
			// FIXME - Currently GrayscalePrint only used
			//optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_GRAYSCALE] = &opt->choices[i];
		}
	}

	return;
}

static void parse_cmandresolution(ppd_option_t *opt)
{
	int i;
	for (i = 0; i < opt->num_choices; i++) {
		if (strcasecmp("CMYKImageRET3600", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_COLOUR] = &opt->choices[i];
			aQuality = PT_QUALITY_HIGH;
			aColor = PT_GRAYSCALE_COLOUR;
		} else if (strcasecmp("CMYKImageRET2400", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_COLOUR] = &opt->choices[i];
			aColor = PT_GRAYSCALE_COLOUR;
		} else if (strcasecmp("Gray1200x1200dpi", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_GRAYSCALE] = &opt->choices[i];
			aQuality = PT_QUALITY_HIGH;
		} else if (strcasecmp("Gray600x600dpi", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_GRAYSCALE] = &opt->choices[i];
		}
	}

	return;
}

static void parse_outputmode(ppd_option_t *opt)
{
	int i;
	for (i = 0; i < opt->num_choices; i++) {
		if (strcasecmp("Draft", opt->choices[i].choice) == 0 ||
				strcasecmp("FastDraft", opt->choices[i].choice) == 0) {
			if (optionCube[0][PT_QUALITY_DRAFT][PT_PAPER_ANY][PT_GRAYSCALE_ANY] == NULL) {
				optionCube[0][PT_QUALITY_DRAFT][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
			}
		} else if (strcasecmp("DraftGray", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_DRAFT][PT_PAPER_ANY][PT_GRAYSCALE_GRAYSCALE] = &opt->choices[i];
		} else if (strcasecmp("DraftRGB", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_DRAFT][PT_PAPER_ANY][PT_GRAYSCALE_COLOUR] = &opt->choices[i];
			aColor = PT_GRAYSCALE_COLOUR;
		} else if (strcasecmp("Auto", opt->choices[i].choice) == 0 ||
				   strcasecmp("Normal", opt->choices[i].choice) == 0 ||
				   strcasecmp("FastNormal", opt->choices[i].choice) == 0 ||
				   strcasecmp("Good", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
		} else if (strcasecmp("NormalGray", opt->choices[i].choice) == 0 ||
				   strcasecmp("NormaGrayl", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_GRAYSCALE] = &opt->choices[i];
		} else if (strcasecmp("NormalRGB", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_COLOUR] = &opt->choices[i];
			aColor = PT_GRAYSCALE_COLOUR;
		} else if (strcasecmp("aPhoto", opt->choices[i].choice) == 0 ||
				   strcasecmp("Best", opt->choices[i].choice) == 0 ||
				   strcasecmp("Fast", opt->choices[i].choice) == 0) {
			if (optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY] == NULL) {
				optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
			}
			aQuality = PT_QUALITY_HIGH;
		} else if (strcasecmp("BestRGB", opt->choices[i].choice) == 0 ||
				   strcasecmp("Photo", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_COLOUR] = &opt->choices[i];
			aQuality = PT_QUALITY_HIGH;
			aColor = PT_GRAYSCALE_COLOUR;
		}
	}

	return;
}

/* KA: database with options and apropriate function */
static KeyActions KA[] = {
	{
		"MediaType",
		parse_mediatype,
		pt_mediatype_words
	},
	{
		"Quality",
		parse_quality,
		pt_quality_words
	},
	{
		"PrintQuality",
		parse_printquality,
		NULL
	},
	{
		"Resolution",
		parse_resolution,
		NULL
	},
	{
		"OutputMode",
		parse_outputmode,
		NULL
	},
	{
		"EconoMode",
		parse_economode,
		NULL
	},
	{
		"HPEconoMode",
		parse_hpeconomode,
		NULL
	},
	{
		"HPEconomic",
		parse_hpeconomic,
		NULL
	},
	{
		"HPClean",
		parse_hpclean,
		NULL
	},
	{
		"HPPrintQuality",
		parse_hpprintquality,
		NULL
	},
	{
		"HPOutputQuality",
		parse_hpoutputquality,
		NULL
	},
	{
		"HPColorOutput",
		parse_hpcoloroutput,
		NULL
	},
	{
		"ColorModel",
		parse_colormodel,
		NULL
	},
	{
		"ColorMode",
		parse_colormode,
		NULL
	},
	{
		"Color",
		parse_color,
		NULL
	},
	{
		"Ink",
		parse_ink,
		NULL
	},
	{
		"CMAndResolution",
		parse_cmandresolution,
		NULL
	},
	{
		"HPColorAsGray",
		parse_hpcolorasgray,
		NULL
	},
	{
		"HPPrintInGrayscale",
		parse_hpprintingrayscale,
		NULL
	},
	{	NULL, NULL, NULL}
};


static ppd_choice_t *quality_exists(int q)
{
	ppd_choice_t *choi = NULL;

	if (!optionCube[aPaperSize][q][aPaper][aColor]) {
		choi = optionCube[aPaperSize][q][PT_PAPER_ANY][PT_GRAYSCALE_ANY];
		if (!choi) {
			choi = optionCube[aPaperSize][q][aPaper][PT_GRAYSCALE_ANY];
		}
		if (!choi) {
			choi = optionCube[aPaperSize][q][PT_PAPER_ANY][aColor];
		}
	}
	return choi;
}

static ppd_choice_t *paper_exists(int p)
{
	ppd_choice_t *choi = NULL;

	if (!optionCube[aPaperSize][aQuality][p][aColor]) {
		choi = optionCube[aPaperSize][PT_QUALITY_ANY][p][PT_GRAYSCALE_ANY];
		if (!choi) {
			choi = optionCube[aPaperSize][aQuality][p][PT_GRAYSCALE_ANY];
		}
		if (!choi) {
			choi = optionCube[aPaperSize][PT_QUALITY_ANY][p][aColor];
		}
	}
	return choi;
}

static ppd_choice_t *color_exists(int c)
{
	ppd_choice_t *choi = NULL;

	if (!optionCube[aPaperSize][aQuality][aPaper][c]) {
		choi = optionCube[aPaperSize][PT_QUALITY_ANY][PT_PAPER_ANY][c];
		// choi == 1 is a special case: option doesn't exist in ppd file,
		// but it is allowd as printer default setting. (for ex.:
		// Grayscale for black & white printer.
		if (!choi && choi != (ppd_choice_t *)1) {
			choi = optionCube[aPaperSize][aQuality][PT_PAPER_ANY][c];
		}
		if (!choi && choi != (ppd_choice_t *)1) {
			choi = optionCube[aPaperSize][PT_QUALITY_ANY][aPaper][c];
		}
	}
	return choi;
}

static void reset_option_cube(int num_sizes)
{
	if (optionCube != NULL) {
		free(optionCube);
	}
	optionCube = calloc(num_sizes, sizeof(OPTIONCUBE));
	// set current settings to their initial values
	aColor = PT_GRAYSCALE_ANY;
	aQuality = PT_QUALITY_ANY;
	aPaper = PT_PAPER_ANY;
}

static void check_defaults(ppd_file_t *ppd)
{
	if (aColor == PT_GRAYSCALE_ANY) {
		PT_DEBUG("Color option not found!");
		ppd_attr_t *at = ppdFindAttr(ppd, "ColorDevice", NULL);
		if (at != NULL) {
			if (strcasecmp(at->text, "True") == 0) {
				aColor = PT_GRAYSCALE_COLOUR;
			} else {
				aColor = PT_GRAYSCALE_GRAYSCALE;
			}
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][aColor] = (ppd_choice_t *)1;
		} else {
			PT_DEBUG("Attribute ColorDevice not found! Check ppd file!");
		}
	}
	if (aQuality == PT_QUALITY_ANY) {
		PT_DEBUG("Qualtiy option not found!");
	}
	if (aPaper == PT_PAPER_ANY) {
		PT_DEBUG("MediaType option not found!");
	}
}

static ppd_choice_t *find_more(ppd_file_t *ppd, ppd_option_t *opt, int quality, int paper, int color, const pt_choice_keyword *keywords)
{
	int i;
	for (i = 0; i < opt->num_choices; i++) {
		const pt_choice_keyword *lookingup = keywords;
		while (lookingup->keyword != NULL) {
			if (strcasecmp(lookingup->keyword, opt->choices[i].choice) == 0 &&
					lookingup->quality == quality && lookingup->papertype == paper && lookingup->grayscale == color)
				if (ppdMarkOption(ppd, opt->keyword, opt->choices[i].choice) == 0) {
					return &opt->choices[i];
				}
			lookingup++;
		}
	}
	return NULL;
}

static ppd_choice_t *parse_again(ppd_file_t *ppd, ppd_option_t *opt, int quality, int paper, int color)
{
	KeyActions *ka = NULL;
	int i;

	for (i = 0, ka = &KA[i]; ka[i].key != NULL; i++) {
		if (strcmp(ka[i].key, opt->keyword) == 0) {
			if (ka[i].keywords != NULL) {
				return find_more(ppd, opt, quality, paper, color, ka[i].keywords);
			}
			return NULL;
		}
	}
	return NULL;
}

static void check_constraints(ppd_file_t *ppd, int papersize)
{
	int i, j, k, n;
	int quality, paper;
	ppd_choice_t *ch;

	ppdMarkDefaults(ppd);
	ppdMarkOption(ppd, "PageSize", paper_sizes->choices[papersize].choice);

	for (i=0; i<PT_QUALITY_ANY; i++) {
		ch = optionCube[papersize][i][PT_PAPER_ANY][PT_GRAYSCALE_ANY];
		if (ch && ch != (ppd_choice_t *)1) {
			n = ppdMarkOption(ppd, ch->option->keyword, ch->choice);
		}
		for (j=0; j<PT_PAPER_ANY; j++) {
			ch = optionCube[papersize][PT_QUALITY_ANY][j][PT_GRAYSCALE_ANY];
			if (!ch) {
				ch = optionCube[papersize][i][j][PT_GRAYSCALE_ANY];
			}
			if (ch && ch != (ppd_choice_t *)1) {
				ppdMarkDefaults(ppd);
				ppdMarkOption(ppd, "PageSize", paper_sizes->choices[papersize].choice);
				n = ppdMarkOption(ppd, ch->option->keyword, ch->choice);
				for (k=0; k<PT_GRAYSCALE_ANY; k++) {
					ch = optionCube[papersize][PT_QUALITY_ANY][PT_PAPER_ANY][k];
					quality = PT_QUALITY_ANY;
					paper = PT_PAPER_ANY;
					if (!ch) {
						ch = optionCube[papersize][i][PT_PAPER_ANY][k];
						quality = i;
						paper = PT_PAPER_ANY;
					}
					if (!ch) {
						ch = optionCube[papersize][PT_QUALITY_ANY][j][k];
						quality = PT_QUALITY_ANY;
						paper = j;
					}
					if (ch && ch != (ppd_choice_t *)1) {
						n = ppdMarkOption(ppd, ch->option->keyword, ch->choice);
						if (n) {
// -- Dont remove, for future versions use !
//                            (*optionCube)[quality][paper][k] = parse_again(ppd, ch->option, quality, paper, k);
//                            if (!(*optionCube)[quality][paper][k])
							optionCube[papersize][i][j][k] = (ppd_choice_t *)(-1);
#ifdef PT_OPTIONCUBE_TEST_PRINT
							PT_DEBUG(" q:[%d] p:[%d] c:[%d] [%s] [%s] [%s] - ppd conflict", i, j, k, qualitynames[i], papernames[j], colornames[k]);
#endif
						}
					}
				}
			}
		}
	}
}

#ifdef PT_OPTIONCUBE_TEST_PRINT

static void testprint(int paper)
{
	int i, j, k;
	char dbgPrint[256];
	char *dbgPrintPtr;
	ppd_choice_t *choi;

	PT_DEBUG("           Gray         Color         Any         for PaperSize: %s", paper_sizes->choices[paper].choice);
	for (i=0; i<=PT_QUALITY_ANY; i++) {
		PT_DEBUG("----------------- %s ------------------", qualitynames[i]);
		for (j=0; j<=PT_PAPER_ANY; j++) {
			dbgPrintPtr = dbgPrint;
			dbgPrintPtr += sprintf(dbgPrintPtr, "%s", papernames[j]);
			for (k=0; k<=PT_GRAYSCALE_ANY; k++) {
				choi = optionCube[paper][i][j][k];
				if (choi == (ppd_choice_t *)(-1)) {
					dbgPrintPtr += sprintf(dbgPrintPtr, "  disabled   ");
				} else if (choi == (ppd_choice_t *)1) {
					dbgPrintPtr += sprintf(dbgPrintPtr, "   enabled   ");
				} else if (choi) {
					dbgPrintPtr += sprintf(dbgPrintPtr, "    %s  ", choi->choice);
				} else {
					dbgPrintPtr += sprintf(dbgPrintPtr, "    <n/a>    ");
				}
			}
			PT_DEBUG("%s", dbgPrint);
		}
	}
}
#endif

int pt_set_choice(int op, int ch)
{
	int ret = -1;
	switch (op) {
	case PT_OPTION_ID_QUALITY:
		if (quality_exists(ch)) {
			aQuality = ch;
		}
		ret = aQuality;
		break;
	case PT_OPTION_ID_PAPER:
		if (paper_exists(ch)) {
			aPaper = ch;
		}
		ret = aPaper;
		break;
	case PT_OPTION_ID_GRAYSCALE:
		if (color_exists(ch)) {
			aColor = ch;
		}
		ret = aColor;
		break;
	case PT_OPTION_ID_PAPERSIZE:
		if (pt_is_enabled(op, ch)) {
			aPaperSize = ch;
		}
		ret = aPaperSize;
	default:
		break;
	}
	return ret;
}

int pt_get_selected(int op)
{
	int choi = -1;
	switch (op) {
	case PT_OPTION_ID_QUALITY:
		choi = aQuality;
		break;
	case PT_OPTION_ID_PAPER:
		choi = aPaper;
		break;
	case PT_OPTION_ID_GRAYSCALE:
		choi = aColor;
		break;
	case PT_OPTION_ID_PAPERSIZE:
		choi = aPaperSize;
		break;
	default:
		break;
	}
	return choi;
}

int pt_is_enabled(int op, int ch)
{
	int enabled = true;
	switch (op) {
	case PT_OPTION_ID_QUALITY:
		if (!quality_exists(ch)) {
			enabled = false;
		}
		break;
	case PT_OPTION_ID_PAPER:
		if (!paper_exists(ch)) {
			enabled = false;
		}
		break;
	case PT_OPTION_ID_GRAYSCALE:
		if (!color_exists(ch)) {
			enabled = false;
		}
		break;
	case PT_OPTION_ID_PAPERSIZE:
		if (optionCube[ch][aQuality][aPaper][aColor] == (ppd_choice_t *)(-1)) {
			enabled = false;
		}
		break;
	default:
		break;
	}
	return enabled;
}

int pt_get_selected_paper_size_pts(pt_pagesize_t *s)
{
	int ret = 0;
	ppd_size_t *size = pt_utils_paper_size_pts(paper_sizes->choices[aPaperSize].choice);
	if (size) {
		s->x = size->width;
		s->y = size->length;
		strcpy(s->name, size->name);
		PT_DEBUG("%s pagesize selected (%f x %f)", s->name, s->x, s->y);
	} else {
		PT_DEBUG("No pagesize selected!");
		ret = 1;
	}
	return ret;
}


char *pt_get_print_option_papersize(int papersize_num)
{
	PT_RETV_IF(paper_sizes == NULL, NULL, "paper_sizes is NULL");
	return paper_sizes->choices[papersize_num].text;
}

char *pt_get_print_option_papersize_cmd(int papersize_num)
{
	PT_RETV_IF(paper_sizes == NULL, NULL, "paper_sizes is NULL");
	return paper_sizes->choices[papersize_num].choice;
}

int pt_get_print_option_papersize_num(void)
{
	PT_RETV_IF(paper_sizes == NULL, 0, "paper_sizes is NULL");
	return paper_sizes->num_choices;
}

int pt_get_print_option_default_papersize(void)
{
	PT_RETV_IF(paper_sizes == NULL, 0, "paper_sizes is NULL");
	int i;
	for (i=0; i< paper_sizes->num_choices; i++)
		if (!strcasecmp(paper_sizes->choices[i].choice, paper_sizes->defchoice)) {
			return i;
		}
	return 0;
}


/* parse_options: search ppd options and call certain function */
void pt_parse_options(ppd_file_t *ppd)
{
	int i;
	ppd_option_t *opt = NULL;
	KeyActions *ka = NULL;

	opt = ppdFindOption(ppd, "PageSize");
	if (opt != NULL) {
		parse_pagesize(opt);
	}

	reset_option_cube(paper_sizes->num_choices);

	//find all options and call necessary function
	for (i = 0, ka = &KA[i]; ka[i].key != NULL; i++) {
		opt = ppdFindOption(ppd, ka[i].key);
		if (opt != NULL) {
			ka[i].fn(opt);
		}
	}
	// Check, if all options were set otherwise set them "by force"
	check_defaults(ppd);
	// copy found options and check UI constraints for all paper sizes
	for (i=1; i<paper_sizes->num_choices; i++) {
		PT_DEBUG("Copying option cubes %d", i);
		memcpy(optionCube[i], optionCube[0], sizeof(OPTIONCUBE));
		PT_DEBUG("Checking UI constraints");
		check_constraints(ppd, i);
#ifdef PT_OPTIONCUBE_TEST_PRINT
		testprint(i);
#endif
	}

	// check UI Constraints for paper size No 0 too.
	check_constraints(ppd, 0);

	return;
}

ppd_choice_t *pt_selected_choice(int op)
{
	ppd_choice_t *choi = NULL;
	switch (op) {
	case PT_OPTION_ID_QUALITY:
		choi = quality_exists(aQuality);
		break;
	case PT_OPTION_ID_PAPER:
		choi = paper_exists(aPaper);
		break;
	case PT_OPTION_ID_GRAYSCALE:
		choi = color_exists(aColor);
		break;
	case PT_OPTION_ID_PAPERSIZE:
		// FIXME: need to check constraints first!
		choi = &paper_sizes->choices[aPaperSize];
		break;
	default:
		break;
	}
	return choi;
}

