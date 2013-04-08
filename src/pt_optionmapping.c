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
#include "pt_utils.h"
#include "pt_ppd.h"
#include "pt_optionmapping.h"

typedef ppd_choice_t *OPTIONCUBE[PT_QUALITY_MAX][PT_PAPER_MAX][PT_GRAYSCALE_MAX][PT_DUPLEX_MAX];

static int aQuality;
static int aPaper;
static int aColor;
static int aDuplex;
static int aPaperSize;
static ppd_option_t *paper_sizes;
static OPTIONCUBE *optionCube;
static char *aModelName;

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
				optionCube[0][lookingup->quality][lookingup->papertype][lookingup->grayscale][lookingup->duplex] = &opt->choices[i];
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
			optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY] = &opt->choices[i];
		} else if (strcasecmp("TextImage", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY] = &opt->choices[i];
		} else if (strcasecmp("Draft", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_DRAFT][PT_PAPER_ANY][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY] = &opt->choices[i];
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
			optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY] = &opt->choices[i];
			aQuality = PT_QUALITY_HIGH;
//		} else if (strcasecmp("PrinterDefault", opt->choices[i].choice) == 0) {
//			optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
		} else if (strcasecmp("TRUE", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_DRAFT][PT_PAPER_ANY][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY] = &opt->choices[i];
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
			optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY] = &opt->choices[i];
			aQuality = PT_QUALITY_HIGH;
		} else if (strcasecmp("PrinterDefault", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY] = &opt->choices[i];
		} else if (strcasecmp("True", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_DRAFT][PT_PAPER_ANY][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY] = &opt->choices[i];
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
			optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY] = &opt->choices[i];
			aQuality = PT_QUALITY_HIGH;
//		} else if (strcasecmp("PrinterDefault", opt->choices[i].choice) == 0) {
//			optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_ANY] = &opt->choices[i];
		} else if (strcasecmp("TRUE", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_DRAFT][PT_PAPER_ANY][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY] = &opt->choices[i];
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
			optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY] = &opt->choices[i];
		} else if (strcasecmp("FALSE", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY] = &opt->choices[i];
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
			optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY] = &opt->choices[i];
		} else if (strcasecmp("600dpi", opt->choices[i].choice) == 0 ||
				   strcasecmp("600x600dpi", opt->choices[i].choice) == 0 ||
				   strcasecmp("ImageRet3600", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY] = &opt->choices[i];
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
			optionCube[0][PT_QUALITY_HIGH][PT_PAPER_ANY][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY] = &opt->choices[i];
		} else if (strcasecmp("Normal", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY] = &opt->choices[i];
		} else if (strcasecmp("Fast", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_DRAFT][PT_PAPER_ANY][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY] = &opt->choices[i];
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
				optionCube[0][lookingup->quality][lookingup->papertype][lookingup->grayscale][lookingup->duplex] = &opt->choices[i];
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
	int i;
	int j = PT_RESOLUTION_HIGH;
	ppd_choice_t *resolutions[PT_RESOLUTION_MAX];

	for (i = 0; i < PT_RESOLUTION_MAX; i++) {
		resolutions[i] = NULL;
	}

	for (i = 0; i < opt->num_choices; i++) {
		const pt_resolution_keyword *lookingup = pt_resoultion_words;
		while(lookingup->keyword != NULL) {
			if(strcasecmp(lookingup->keyword, opt->choices[i].choice) == 0) {
				resolutions[lookingup->weight] = &opt->choices[i];
				break;
			} else {
				lookingup++;
			}
		}
	}

	for (i = PT_QUALITY_HIGH; i >= PT_QUALITY_DRAFT; i--) {
		while(resolutions[j] == NULL && j >= 0) {
			j--;
		}
		if (j < 0) {
			break;
		}
		optionCube[0][i][PT_PAPER_ANY][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY] = resolutions[j];
		j--;
	}

	aQuality = PT_QUALITY_ANY;
	for(i = PT_QUALITY_DRAFT; i < PT_QUALITY_ANY; i++) {
		if (optionCube[0][i][PT_PAPER_ANY][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY] != NULL) {
			if ((strcasecmp(optionCube[0][i][PT_PAPER_ANY][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY]->choice, opt->defchoice) == 0)) {
				aQuality = i;
				break;
			}
		}
	}
}

static void parse_ink(ppd_option_t *opt)
{
	int i;
	for (i = 0; i < opt->num_choices; i++) {
		if (strcasecmp("COLOR", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_COLOUR][PT_DUPLEX_ANY] = &opt->choices[i];
		} else if (strcasecmp("MONO", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_GRAYSCALE][PT_DUPLEX_ANY] = &opt->choices[i];
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
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_COLOUR][PT_DUPLEX_ANY] = &opt->choices[i];
		} else if (strcasecmp("Grayscale", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_GRAYSCALE][PT_DUPLEX_ANY] = &opt->choices[i];
		} else if (strcasecmp("Mono", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_GRAYSCALE][PT_DUPLEX_ANY] = &opt->choices[i];
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
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_COLOUR][PT_DUPLEX_ANY] = &opt->choices[i];
		} else if (strcasecmp("GrayscalePrint", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_GRAYSCALE][PT_DUPLEX_ANY] = &opt->choices[i];
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
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_COLOUR][PT_DUPLEX_ANY] = &opt->choices[i];
		} else if (strcasecmp("FALSE", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_GRAYSCALE][PT_DUPLEX_ANY] = &opt->choices[i];
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
			if (optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_GRAYSCALE][PT_DUPLEX_ANY] == NULL) {
				optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_GRAYSCALE][PT_DUPLEX_ANY] = &opt->choices[i];
			}
			if (strcasecmp(opt->choices[i].choice, opt->defchoice) == 0) {
				aColor = PT_GRAYSCALE_GRAYSCALE;
			}
		} else if (strcasecmp("CMYK", opt->choices[i].choice) == 0 ||
				   strcasecmp("Color", opt->choices[i].choice) == 0 ||
				   strcasecmp("RGBA", opt->choices[i].choice) == 0 ||
				   strcasecmp("RGB", opt->choices[i].choice) == 0) {
			if (optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_COLOUR][PT_DUPLEX_ANY] == NULL) {
				optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_COLOUR][PT_DUPLEX_ANY] = &opt->choices[i];
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
		if (strcasecmp("False", opt->choices[i].choice) == 0 ||
			strcasecmp("Off", opt->choices[i].choice) == 0 ) {
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_COLOUR][PT_DUPLEX_ANY] = &opt->choices[i];
		} else if (strcasecmp("True", opt->choices[i].choice) == 0 ||
					strcasecmp("HighQuality", opt->choices[i].choice) == 0 ||
					strcasecmp("BlackInkOnly", opt->choices[i].choice) == 0) {
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_GRAYSCALE][PT_DUPLEX_ANY] = &opt->choices[i];
		}
	}
	// setup the default value:
	if (strcasecmp("False", opt->defchoice) == 0 || strcasecmp("Off", opt->defchoice) == 0) {
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
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_GRAYSCALE][PT_DUPLEX_ANY] = &opt->choices[i];
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
		const pt_choice_keyword *lookingup = pt_cmandresolution_words;
		while (lookingup->keyword != NULL) {
			if (strcasecmp(lookingup->keyword, opt->choices[i].choice) == 0) {
				optionCube[0][lookingup->quality][lookingup->papertype][lookingup->grayscale][lookingup->duplex] = &opt->choices[i];
				break;
			} else {
				lookingup++;
			}
		}
		if ((strcasecmp(opt->choices[i].choice, opt->defchoice) == 0) && (lookingup->keyword != NULL)) {
			aQuality = lookingup->quality;
			aColor = lookingup->grayscale;
		}
	}
	return;
}

static void parse_outputmode(ppd_option_t *opt)
{
	int i;
	for (i=0; i < opt->num_choices; i++) {
		const pt_choice_keyword *lookingup = pt_outputmode_words;
		while (lookingup->keyword != NULL) {
			if (strcasecmp(lookingup->keyword, opt->choices[i].choice) == 0) {
				optionCube[0][lookingup->quality][lookingup->papertype][lookingup->grayscale][lookingup->duplex] = &opt->choices[i];
				break;
			} else {
				lookingup++;
			}
		}
		if ((strcasecmp(opt->choices[i].choice, opt->defchoice) == 0) && (lookingup->keyword != NULL)) {
			aQuality = (lookingup->quality   != PT_QUALITY_ANY)   ? lookingup->quality   : aQuality;
			aPaper =   (lookingup->papertype != PT_PAPER_ANY)     ? lookingup->papertype : aPaper;
			aColor =   (lookingup->grayscale != PT_GRAYSCALE_ANY) ? lookingup->grayscale : aColor;
		}
	}
}

static void parse_duplex(ppd_option_t *opt)
{
	int i;
	for(i=0; i < opt->num_choices; i++) {
		const pt_choice_keyword *lookingup = pt_duplex_words;
		while (lookingup->keyword != NULL) {
			if (strcasecmp(lookingup->keyword, opt->choices[i].choice) == 0) {
				optionCube[0][lookingup->quality][lookingup->papertype][lookingup->grayscale][lookingup->duplex] = &opt->choices[i];
				break;
			} else {
				lookingup++;
			}
		}
		if ((strcasecmp(opt->choices[i].choice, opt->defchoice) == 0) && lookingup->keyword != NULL) {
			aDuplex = lookingup->duplex;
		}
	}
}

/* KA: database with options and apropriate function */
static KeyActions KA[] = {
	{
		"MediaType",
		parse_mediatype,
		pt_mediatype_words
	},
	{
		"HPEconoMode",
		parse_hpeconomode,
		NULL
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
	{
		"Duplex",
		parse_duplex,
		pt_duplex_words
	},
	{	NULL, NULL, NULL}
};


static ppd_choice_t *quality_exists(int q)
{
	ppd_choice_t *choi = NULL;

	if (!optionCube[aPaperSize][q][aPaper][aColor][aDuplex]) {
		choi = optionCube[aPaperSize][q][PT_PAPER_ANY][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY];
		// choi == 1 is a special case: option doesn't exist in ppd file,
		// but it is allowed as a printer default setting, for ex.:
		// Standard quality as the only quality, a printer supports
		if (!choi && choi != (ppd_choice_t *)1) {
			choi = optionCube[aPaperSize][q][aPaper][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY];
		}
		if (!choi && choi != (ppd_choice_t *)1) {
			choi = optionCube[aPaperSize][q][PT_PAPER_ANY][aColor][PT_DUPLEX_ANY];
		}
	}
	return choi;
}

static ppd_choice_t *paper_exists(int p)
{
	ppd_choice_t *choi = NULL;

	if (!optionCube[aPaperSize][aQuality][p][aColor][aDuplex]) {
		choi = optionCube[aPaperSize][PT_QUALITY_ANY][p][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY];
		// choi == 1 is a special case: option doesn't exist in ppd file,
		// but it is allowed as printer default setting, for ex.:
		// Plain paper as the only and default paper type
		if (!choi && choi != (ppd_choice_t *)1) {
			choi = optionCube[aPaperSize][aQuality][p][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY];
		}
		if (!choi && choi != (ppd_choice_t *)1) {
			choi = optionCube[aPaperSize][PT_QUALITY_ANY][p][aColor][PT_DUPLEX_ANY];
		}
	}
	return choi;
}

static ppd_choice_t *color_exists(int c)
{
	ppd_choice_t *choi = NULL;

	if (!optionCube[aPaperSize][aQuality][aPaper][c][aDuplex]) {
		choi = optionCube[aPaperSize][PT_QUALITY_ANY][PT_PAPER_ANY][c][PT_DUPLEX_ANY];
		// choi == 1 is a special case: option doesn't exist in ppd file,
		// but it is allowed as printer default setting, for ex.:
		// Grayscale for black & white printer.
		if (!choi && choi != (ppd_choice_t *)1) {
			choi = optionCube[aPaperSize][aQuality][PT_PAPER_ANY][c][PT_DUPLEX_ANY];
		}
		if (!choi && choi != (ppd_choice_t *)1) {
			choi = optionCube[aPaperSize][PT_QUALITY_ANY][aPaper][c][PT_DUPLEX_ANY];
		}
	}
	return choi;
}

static ppd_choice_t *duplex_exists(int d)
{
	ppd_choice_t *choi = NULL;
	if (!optionCube[aPaperSize][aQuality][aPaper][aColor][d]) {
		choi = optionCube[aPaperSize][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_ANY][d];
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
	aDuplex = PT_DUPLEX_ANY;
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
			optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][aColor][PT_DUPLEX_ANY] = (ppd_choice_t *)1;
		} else {
			PT_DEBUG("Attribute ColorDevice not found! Check ppd file!");
		}
	}
	if (aQuality == PT_QUALITY_ANY) {
		PT_DEBUG("Qualtiy option not found!");
		optionCube[0][PT_QUALITY_STANDARD][PT_PAPER_ANY][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY] = (ppd_choice_t *)1;
		aQuality = PT_QUALITY_STANDARD;
	}
	if (aPaper == PT_PAPER_ANY) {
		PT_DEBUG("MediaType option not found!");
		optionCube[0][PT_QUALITY_ANY][PT_PAPER_NORMAL][PT_GRAYSCALE_ANY][PT_DUPLEX_ANY] = (ppd_choice_t *)1;
		aPaper = PT_PAPER_NORMAL;
	}
	if (aDuplex == PT_DUPLEX_ANY) {
		PT_DEBUG("Duplex option not found!");
		optionCube[0][PT_QUALITY_ANY][PT_PAPER_ANY][PT_GRAYSCALE_ANY][PT_DUPLEX_OFF] = (ppd_choice_t *)1;
		aDuplex = PT_DUPLEX_OFF;
	}
}

static int mark_choice(ppd_file_t *ppd, ppd_choice_t* ch) {
	if (ch && ch != (ppd_choice_t*) 1) {
		ch->marked = 1;
		return ppdConflicts(ppd);
	}
	return -1;
}

static void unmark_choice(ppd_choice_t* ch) {
    if (ch && ch != (ppd_choice_t*) 1) {
        ch->marked = 0;
    }
}

static void check_constraints(ppd_file_t *ppd, int papersize)
{
	int sPaperSize, sQuality, sPaper, sColor, sDuplex;
	ppd_choice_t* ch;
	ppd_choice_t* qualityCh;
	ppd_choice_t* paperCh;
	ppd_choice_t* colorCh;

	// saving default values
	sPaperSize = aPaperSize;
	sQuality = aQuality;
	sPaper = aPaper;
	sColor = aColor;
	sDuplex = aDuplex;
	aPaperSize = papersize;
	ppdMarkDefaults(ppd);
	ppdMarkOption(ppd, "PageSize", paper_sizes->choices[papersize].choice);
	for (aQuality=0; aQuality<PT_QUALITY_ANY; aQuality++) {
		qualityCh = pt_selected_choice(PT_OPTION_ID_QUALITY, PT_ORIENTATION_PORTRAIT);
		mark_choice(ppd, qualityCh);
		for (aPaper=0; aPaper<PT_PAPER_ANY; aPaper++) {
			paperCh = pt_selected_choice(PT_OPTION_ID_PAPER, PT_ORIENTATION_PORTRAIT);
			mark_choice(ppd, paperCh);
			for (aColor=0; aColor<PT_GRAYSCALE_ANY; aColor++) {
				colorCh = pt_selected_choice(PT_OPTION_ID_GRAYSCALE, PT_ORIENTATION_PORTRAIT);
				mark_choice(ppd, colorCh);
				for(aDuplex=0; aDuplex<PT_DUPLEX_ANY; aDuplex++) {
					ch = duplex_exists(aDuplex);
					if(ch && ch != (ppd_choice_t*)1) {
						if (ppdMarkOption(ppd, ch->option->keyword, ch->choice) > 0) {
							optionCube[papersize][aQuality][aPaper][aColor][aDuplex] = (ppd_choice_t *) -1;
						}
						ch->marked = 0;
					} else if(ppdConflicts(ppd) > 0) {
						optionCube[papersize][aQuality][aPaper][aColor][aDuplex] = (ppd_choice_t *) -1;
					}
				}
				unmark_choice(colorCh);
			}
			unmark_choice(paperCh);
		}
		unmark_choice(qualityCh);
	}
	// restoring default values
	aQuality = sQuality;
	aPaper = sPaper;
	aColor = sColor;
	aDuplex = sDuplex;
	aPaperSize = sPaperSize;
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
				choi = optionCube[paper][i][j][k][PT_DUPLEX_ANY];
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

static bool pt_validate_settings(int s, int q, int p, int c, int d)
{
        if (s < 0 || s >= paper_sizes->num_choices) {
                return false;
        }
        if (q < PT_QUALITY_DRAFT || q > PT_QUALITY_HIGH) {
                return false;
        }
        if (p < PT_PAPER_NORMAL || p > PT_PAPER_PHOTO) {
                return false;
        }
        if (c < PT_GRAYSCALE_GRAYSCALE || c > PT_GRAYSCALE_COLOUR) {
                return false;
        }
        if (d < PT_DUPLEX_OFF || d > PT_DUPLEX_TUMBLE) {
                return false;
        }
        return true;
}

static void pt_try_settings(int s, int q, int p, int c, int d)
{
	int sPaperSize = aPaperSize;
	int sQuality = aQuality;
	int sPaper = aPaper;
	int sColor = aColor;
	int sDuplex = aDuplex;

	if(pt_validate_settings(s, q, p, c, d)) {
		if (!pt_is_enabled(PT_OPTION_ID_QUALITY, q) || !pt_is_enabled(PT_OPTION_ID_PAPER, p) ||
			!pt_is_enabled(PT_OPTION_ID_GRAYSCALE, c) || !pt_is_enabled(PT_OPTION_ID_DUPLEX, d)) {
			aPaperSize = sPaperSize;
			aQuality = sQuality;
			aPaper = sPaper;
			aColor = sColor;
			aDuplex = sDuplex;
		} else {
			aPaperSize = s;
			aQuality = q;
			aPaper = p;
			aColor = c;
			aDuplex = d;
		}
	}
}

void pt_load_user_choice(void) {
	FILE *stream;
	char buf[255] = {0,};
	char *args;
	int s;
	int q;
	int p;
	int c;
	int d;
	char *null_check = NULL;

	stream = fopen(PT_USER_OPTION_CONFIG_FILE,"r");
	if(stream == NULL) {
		PT_DEBUG("Can't open settings file");
		return;
	}
	while(fgets(buf, sizeof(buf), stream) != NULL) {
		null_check = strchr(buf, '\n');
		if (null_check) {
			*null_check = '\0';
		}

		gchar **tokens = g_strsplit((gchar *)buf, ",", 0);
		if (g_strv_length(tokens) != 6) {
			g_strfreev(tokens);
			continue;
		}

		if (!strcasecmp(tokens[0], aModelName)) {
			s = atoi(tokens[1]);
			q = atoi(tokens[2]);
			p = atoi(tokens[3]);
			c = atoi(tokens[4]);
			d = atoi(tokens[5]);
			pt_try_settings(s, q, p, c, d);
			g_strfreev(tokens);
			break;
		}
		g_strfreev(tokens);
	}
	fclose(stream);
}

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
		break;
	case PT_OPTION_ID_DUPLEX:
		if (duplex_exists(ch)) {
			aDuplex = ch;
		}
		ret = aDuplex;
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
	case PT_OPTION_ID_DUPLEX:
		choi = aDuplex;
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
		if (optionCube[ch][aQuality][aPaper][aColor][aDuplex] == (ppd_choice_t *)(-1)) {
			enabled = false;
		}
		break;
	case PT_OPTION_ID_DUPLEX:
		if (!duplex_exists(ch)) {
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
	ppd_attr_t *at = NULL;
	KeyActions *ka = NULL;

	opt = ppdFindOption(ppd, "PageSize");
	if (opt != NULL) {
		parse_pagesize(opt);
	}

	at = ppdFindAttr(ppd, "ModelName", NULL);
	if (at != NULL) {
		aModelName = at->value;
		PT_DEBUG("ModelName found, value: %s; spec: %s; text: %s", at->value, at->spec, at->text);
	} else {
		PT_DEBUG("ModelName not found");
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
	pt_load_user_choice();
	return;
}

ppd_choice_t *pt_selected_choice(int op, pt_orientation_e p)
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
		choi = &paper_sizes->choices[aPaperSize];
		break;
	case PT_OPTION_ID_DUPLEX:
		choi = duplex_exists(aDuplex);
		if(aDuplex != PT_DUPLEX_OFF) {
			choi = (p == PT_ORIENTATION_PORTRAIT) ? duplex_exists(PT_DUPLEX_NO_TUMBLE) : duplex_exists(PT_DUPLEX_TUMBLE);
		}
		break;
	default:
		break;
	}
	if(choi == (ppd_choice_t*)1) {
		// Special case. Set by force, no ppd option. Use printer default.
		// No need to send smth. to printer.
		choi = NULL;
	}
	return choi;
}

void pt_save_user_choice(void) {
	FILE* stream;
	fpos_t position;
	char buf[255];
	char *args;
	int ret = -1;

	stream = fopen(PT_USER_OPTION_CONFIG_FILE,"r+");
	if(stream == NULL) {
		//may be file doesn't exist, try to create it...
		stream = fopen(PT_USER_OPTION_CONFIG_FILE,"w");
		if(stream == NULL) {
			PT_DEBUG("Can't create settings file");
			return;
		}
	}
	fgetpos(stream, &position);
	while(fgets(buf, sizeof(buf)-1, stream) != NULL) {
		args = strchr(buf, ',');
		if(args == NULL) {
			continue;
		}
		*args++ = '\0';
		if(!strcasecmp(buf, aModelName)) {
			ret = fsetpos(stream, &position);
			if (ret == -1) {
				fclose(stream);
				return;
			}
			fprintf(stream, "%s,%03d,%03d,%03d,%03d,%03d\n", aModelName, aPaperSize, aQuality, aPaper, aColor, aDuplex);
			fclose(stream);
			return;
		}
		fgetpos(stream, &position);
	}
	ret = fsetpos(stream, &position);
	if (ret == -1) {
		fclose(stream);
		return;
	}
	fprintf(stream, "%s,%03d,%03d,%03d,%03d,%03d\n", aModelName, aPaperSize, aQuality, aPaper, aColor, aDuplex);
	fclose(stream);
}
