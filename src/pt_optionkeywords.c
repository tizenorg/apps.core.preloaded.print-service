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

#include "pt_optionmapping.h"

static const pt_choice_keyword pt_quality_words[] = {
	{"Best",                  PT_QUALITY_HIGH,     PT_PAPER_ANY,    PT_GRAYSCALE_ANY}, // - 12
	{"Normal",                PT_QUALITY_STANDARD, PT_PAPER_ANY,    PT_GRAYSCALE_ANY}, // - 13
	{"Draft",                 PT_QUALITY_DRAFT,    PT_PAPER_ANY,    PT_GRAYSCALE_ANY}, // - 12
	{"NormalBest",            PT_QUALITY_HIGH,     PT_PAPER_ANY,    PT_GRAYSCALE_ANY}, // - 13
	{"PMPHOTO_NORMAL",        PT_QUALITY_STANDARD, PT_PAPER_PHOTO,  PT_GRAYSCALE_ANY}, // - 290
	{"PMPHOTO_HIGH",          PT_QUALITY_HIGH,     PT_PAPER_PHOTO,  PT_GRAYSCALE_ANY}, // - 95
	{"PMPHOTO_DRAFT",         PT_QUALITY_DRAFT,    PT_PAPER_PHOTO,  PT_GRAYSCALE_ANY}, // - 90
	{"PLATINA_NORMAL",        PT_QUALITY_STANDARD, PT_PAPER_GLOSSY, PT_GRAYSCALE_ANY}, // - 204
	{"PLATINA_HIGH",          PT_QUALITY_HIGH,     PT_PAPER_GLOSSY, PT_GRAYSCALE_ANY}, // - 81
	{"GLOSSYPHOTO_NORMAL",    PT_QUALITY_STANDARD, PT_PAPER_GLOSSY, PT_GRAYSCALE_ANY}, // - 196
	{"GLOSSYPHOTO_HIGH",      PT_QUALITY_HIGH,     PT_PAPER_GLOSSY, PT_GRAYSCALE_ANY}, // - 70
	{"GLOSSYPHOTO_DRAFT",     PT_QUALITY_DRAFT,    PT_PAPER_GLOSSY, PT_GRAYSCALE_ANY}, // - 57
	{"PMMATT_NORMAL",         PT_QUALITY_STANDARD, PT_PAPER_GLOSSY, PT_GRAYSCALE_ANY}, // - 193
	{"PMMATT_HIGH",           PT_QUALITY_HIGH,     PT_PAPER_GLOSSY, PT_GRAYSCALE_ANY}, // - 73
	{"PLAIN_NORMAL",          PT_QUALITY_STANDARD, PT_PAPER_NORMAL, PT_GRAYSCALE_ANY}, // - 263
	{"PLAIN_HIGH",            PT_QUALITY_HIGH,     PT_PAPER_NORMAL, PT_GRAYSCALE_ANY}, // - 164
//	{"CDDVD_HIGH", },   // - 67
	{"MINIPHOTO_NORMAL",      PT_QUALITY_STANDARD, PT_PAPER_PHOTO,  PT_GRAYSCALE_ANY}, // - 79
	{"IJPC_NORMAL",           PT_QUALITY_STANDARD, PT_PAPER_GLOSSY, PT_GRAYSCALE_ANY}, // - 53
	{"GLOSSYHAGAKI_NORMAL",   PT_QUALITY_STANDARD, PT_PAPER_GLOSSY, PT_GRAYSCALE_ANY}, // - 38
	{"GLOSSYHAGAKI_HIGH",     PT_QUALITY_HIGH,     PT_PAPER_GLOSSY, PT_GRAYSCALE_ANY}, // - 26
	{"RCPC_NORMAL",           PT_QUALITY_STANDARD, PT_PAPER_NORMAL, PT_GRAYSCALE_ANY}, // - 40
	{"RCPC_HIGH",             PT_QUALITY_HIGH,     PT_PAPER_NORMAL, PT_GRAYSCALE_ANY}, // - 34
	{"IRON_NORMAL",           PT_QUALITY_STANDARD, PT_PAPER_PHOTO,  PT_GRAYSCALE_ANY}, // - 27
	{"GLOSSYCAST_NORMAL",     PT_QUALITY_STANDARD, PT_PAPER_PHOTO,  PT_GRAYSCALE_ANY}, // - 107
	{"GLOSSYCAST_HIGH",       PT_QUALITY_HIGH,     PT_PAPER_PHOTO,  PT_GRAYSCALE_ANY}, // - 27
	{"GLOSSYCAST_DRAFT",      PT_QUALITY_DRAFT,    PT_PAPER_PHOTO,  PT_GRAYSCALE_ANY}, // - 27
	{"PLAIN_DRAFT",           PT_QUALITY_DRAFT,    PT_PAPER_NORMAL, PT_GRAYSCALE_ANY}, // - 33
	{"PMMATT_DRAFT",          PT_QUALITY_DRAFT,    PT_PAPER_GLOSSY, PT_GRAYSCALE_ANY}, // - 17
	{"MMEISHI_NORMAL",        PT_QUALITY_STANDARD, PT_PAPER_GLOSSY, PT_GRAYSCALE_ANY}, // - 11
	{"PLATINA_DRAFT",         PT_QUALITY_DRAFT,    PT_PAPER_GLOSSY, PT_GRAYSCALE_ANY}, // - 9
	{"GLOSSYHAGAKI_DRAFT",    PT_QUALITY_DRAFT,    PT_PAPER_GLOSSY, PT_GRAYSCALE_ANY}, // - 3
	{"IJPC_HIGH",             PT_QUALITY_HIGH,     PT_PAPER_GLOSSY, PT_GRAYSCALE_ANY}, // - 3
	{"IJPC_DRAFT",            PT_QUALITY_DRAFT,    PT_PAPER_GLOSSY, PT_GRAYSCALE_ANY}, // - 3
	{"RCPC_DRAFT",            PT_QUALITY_DRAFT,    PT_PAPER_NORMAL, PT_GRAYSCALE_ANY}, // - 2
	{"MINIPHOTO_HIGH",        PT_QUALITY_HIGH,     PT_PAPER_PHOTO,  PT_GRAYSCALE_ANY}, // - 4
	{"MINIPHOTO_DRAFT",       PT_QUALITY_DRAFT,    PT_PAPER_PHOTO,  PT_GRAYSCALE_ANY}, // - 4
	{"SFINE_NORMAL",          PT_QUALITY_STANDARD, PT_PAPER_PHOTO,  PT_GRAYSCALE_ANY}, // - 5
	{"SFINE_HIGH",            PT_QUALITY_HIGH,     PT_PAPER_PHOTO,  PT_GRAYSCALE_ANY}, // - 5
	{"SFINE_DRAFT",           PT_QUALITY_DRAFT,    PT_PAPER_PHOTO,  PT_GRAYSCALE_ANY}, // - 5
//	{"CDDVD_NORMAL", }, // - 7
//	{"CDDVD_DRAFT", },  // - 4
	{"IRON_HIGH",             PT_QUALITY_HIGH,     PT_PAPER_PHOTO,  PT_GRAYSCALE_ANY}, // - 2
	{"IRON_DRAFT",            PT_QUALITY_DRAFT,    PT_PAPER_PHOTO,  PT_GRAYSCALE_ANY}, // - 2
	{"MMEISHI_HIGH",          PT_QUALITY_HIGH,     PT_PAPER_GLOSSY, PT_GRAYSCALE_ANY}, // - 2
	{"MMEISHI_DRAFT",         PT_QUALITY_DRAFT,    PT_PAPER_GLOSSY, PT_GRAYSCALE_ANY}, // - 2
	{"PSGLOS_NORMAL",         PT_QUALITY_STANDARD, PT_PAPER_GLOSSY, PT_GRAYSCALE_ANY}, // - 4
	{"PSGLOS_HIGH",           PT_QUALITY_HIGH,     PT_PAPER_GLOSSY, PT_GRAYSCALE_ANY}, // - 4
	{"PSGLOS_DRAFT",          PT_QUALITY_DRAFT,    PT_PAPER_GLOSSY, PT_GRAYSCALE_ANY}, // - 4
	{"MCLP_NORMAL",           PT_QUALITY_STANDARD, PT_PAPER_PHOTO,  PT_GRAYSCALE_ANY}, // - 1
	{"MCLP_HIGH",             PT_QUALITY_HIGH,     PT_PAPER_PHOTO,  PT_GRAYSCALE_ANY}, // - 1
	{"MCLP_DRAFT",            PT_QUALITY_DRAFT,    PT_PAPER_PHOTO,  PT_GRAYSCALE_ANY}, // - 1
	{"EPHOTO_NORMAL",         PT_QUALITY_STANDARD, PT_PAPER_GLOSSY, PT_GRAYSCALE_ANY}, // - 2
	{"EPHOTO_HIGH",           PT_QUALITY_HIGH,     PT_PAPER_GLOSSY, PT_GRAYSCALE_ANY}, // - 2
	{"EPHOTO_DRAFT",          PT_QUALITY_DRAFT,    PT_PAPER_GLOSSY, PT_GRAYSCALE_ANY}, // - 2
	{"GPPAPER_NORMAL",        PT_QUALITY_STANDARD, PT_PAPER_PHOTO,  PT_GRAYSCALE_ANY}, // - 2
	{"GPPAPER_HIGH",          PT_QUALITY_HIGH,     PT_PAPER_PHOTO,  PT_GRAYSCALE_ANY}, // - 2
	{"GPPAPER_DRAFT",         PT_QUALITY_DRAFT,    PT_PAPER_PHOTO,  PT_GRAYSCALE_ANY}, // - 2
	{"300x300dpi",            PT_QUALITY_DRAFT,    PT_PAPER_ANY,    PT_GRAYSCALE_ANY}, // - 37
	{"600x600dpi",            PT_QUALITY_STANDARD, PT_PAPER_ANY,    PT_GRAYSCALE_ANY}, // - 37
	{"1200x1200dpi",          PT_QUALITY_HIGH,     PT_PAPER_ANY,    PT_GRAYSCALE_ANY}, // - 12
	{ NULL,                   PT_QUALITY_ANY,      PT_PAPER_ANY,    PT_GRAYSCALE_ANY}
};

static const pt_choice_keyword pt_mediatype_words[] = {
	/* ** Epson new keywords ** */
	{"EULTRAGLOSSY",                  PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 1
	{"EPREMGLOSS",                    PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 1
	{"EPSGLOS",                       PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 1
	{"EPENTRY",                       PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 1
	{"EGCP",                          PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1

	/* ** HP MediaType keywords ** */
	{"Plain",                         PT_QUALITY_ANY,     PT_PAPER_NORMAL,  PT_GRAYSCALE_ANY}, // - 676
	{"Glossy",                        PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 441
	{"Automatic",                     PT_QUALITY_ANY,     PT_PAPER_NORMAL,  PT_GRAYSCALE_ANY}, // - 173
//	{"TransparencyFilm",              PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 200
//	{"CDDVDMedia",                    PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 111
	{"Photo",                         PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 9
	{"Unspecified",                   PT_QUALITY_ANY,     PT_PAPER_NORMAL,  PT_GRAYSCALE_ANY}, // - 1
	{"HPLaserJet90",                  PT_QUALITY_ANY,     PT_PAPER_NORMAL,  PT_GRAYSCALE_ANY}, // - 1
//	{"HPColorLaserMatte105",          PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1
//	{"HPPremiumChoiceMatte120",       PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1
//	{"HPColorLaserBrochureMatte160",  PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1
//	{"HPSuperiorLaserMatte160",       PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1
//	{"HPCoverMatte200",               PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1
//	{"HPMattePhoto200",               PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1
	{"HPPresentationGlossy130",       PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 1
	{"HPProfessionalLaserGlossy130",  PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 1
	{"HPColorLaserBrochureGlossy160", PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 1
	{"HPSuperiorLaserGlossy160",      PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 1
	{"HPTriFoldColorLaserBrochure160",PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 1
	{"HPColorLaserPhotoGlossy220",    PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1
	{"HPColorLaserPhotoGlossyFast220",PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1
	{"HPColorLaserPhotoGlossyHigh220",PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1
	{"Light6074",                     PT_QUALITY_ANY,     PT_PAPER_NORMAL,  PT_GRAYSCALE_ANY}, // - 1
	{"MidWeight96110",                PT_QUALITY_ANY,     PT_PAPER_NORMAL,  PT_GRAYSCALE_ANY}, // - 1
	{"Heavy111130",                   PT_QUALITY_ANY,     PT_PAPER_NORMAL,  PT_GRAYSCALE_ANY}, // - 1
	{"ExtraHeavy131175",              PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1
	{"HeavyGlossy111130",             PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 1
	{"ExtraHeavyGlossy131175",        PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 1
	{"CardGlossy176220",              PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 1
//	{"ColorLaserTransparency",        PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1
//	{"Labels",                        PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1
//	{"Letterhead",                    PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1
//	{"Envelope",                      PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1
//	{"HeavyEnvelope",                 PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1
//	{"Preprinted",                    PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1
//	{"Prepunched",                    PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1
//	{"Colored",                       PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1
//	{"Bond",                          PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1
//	{"Recycled",                      PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1
//	{"Rough",                         PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1
//	{"HeavyRough",                    PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1
//	{"HPToughPaper",                  PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1
	{"APhoto",                        PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 3
	{"PPhoto",                        PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 3
	{"OPhoto",                        PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 3
	{"HPPresentationSoftGloss120",    PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 2
	{"HPProfessionalSoftGloss120",    PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 2
	{"HPPresentationGlossy130g",      PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 1
	{"HPProfessionalLaserGlossy130g", PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 1
	{"HPCLaserPhotoGlossy220",        PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 1
	{"Intermediate8595",              PT_QUALITY_ANY,     PT_PAPER_NORMAL,  PT_GRAYSCALE_ANY}, // - 4
	{"Cardstock176220",               PT_QUALITY_ANY,     PT_PAPER_NORMAL,  PT_GRAYSCALE_ANY}, // - 3
	{"MidWTGlossy96110",              PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 1
//	{"OpaqueFilm",                    PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 4
	{"MidWeightGlossy96110",          PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 2
	{"HPEcoSMARTLite",                PT_QUALITY_ANY,     PT_PAPER_NORMAL,  PT_GRAYSCALE_ANY}, // - 4
//	{"HPBrochureMatte150",            PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 3
	{"HPPremiumPresentationGlossy120",PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 3
	{"HPBrochureGlossy150",           PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 3
	{"HPTrifoldBrochureGlossy150",    PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 3
	{"HPBrochureGlossy200",           PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 3
	{"Light",                         PT_QUALITY_ANY,     PT_PAPER_NORMAL,  PT_GRAYSCALE_ANY}, // - 1
//	{"Color",                         PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 1
	{"Card_Stock",                    PT_QUALITY_ANY,     PT_PAPER_NORMAL,  PT_GRAYSCALE_ANY}, // - 1
//	{"MonochromeLaserTransparency",   PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 1
//	{"ShelfEdgeLabels",               PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 1
	{"Card_Stock176220",              PT_QUALITY_ANY,     PT_PAPER_NORMAL,  PT_GRAYSCALE_ANY}, // - 1
//	{"Monotransparency",              PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 1
//	{"HPPremiumPresentationMatte120", PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 2

	/* ** Samsung MediaType keywords ** */
	{"Normal",                        PT_QUALITY_ANY,     PT_PAPER_NORMAL,  PT_GRAYSCALE_ANY}, // - 33
	{"Photo160",                      PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 2
	{"Photo111-130",                  PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 8
	{"Photo131-175",                  PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 8
	{"Photo176-220",                  PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 8
	{"MattePhoto111-130",             PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 4
	{"MattePhoto131-175",             PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 4
	{"MattePhoto176-220",             PT_QUALITY_ANY,     PT_PAPER_PHOTO,   PT_GRAYSCALE_ANY}, // - 4
	{"ThinGlossy",                    PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 5
	{"ThickGlossy",                   PT_QUALITY_ANY,     PT_PAPER_GLOSSY,  PT_GRAYSCALE_ANY}, // - 3
	{ NULL,                           PT_QUALITY_ANY,     PT_PAPER_ANY,     PT_GRAYSCALE_ANY}
};
