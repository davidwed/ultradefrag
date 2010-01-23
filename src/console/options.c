/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007-2010 by Dmitri Arkhangelski (dmitriar@gmail.com).
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
* UltraDefrag console interface - command line parsing code.
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "../include/udefrag.h"
#include "../include/ultradfgver.h"

#include "../include/getopt.h"

extern int a_flag;
extern int o_flag;
extern int b_flag;
extern int h_flag;
extern int l_flag;
extern int la_flag;
extern int p_flag;
extern int v_flag;
extern int m_flag;
extern int obsolete_option;
extern char letter;

extern short map_border_color;
extern char map_symbol;
extern int map_rows;
extern int map_symbols_per_line;

extern int screensaver_mode;

void show_help(void)
{
	printf(
		"===============================================================================\n"
		VERSIONINTITLE " - Powerful disk defragmentation tool for Windows NT\n"
		"Copyright (c) Dmitri Arkhangelski, 2007-2010.\n"
		"\n"
		"===============================================================================\n"
		"This program is free software; you can redistribute it and/or\n"
		"modify it under the terms of the GNU General Public License\n"
		"as published by the Free Software Foundation; either version 2\n"
		"of the License, or (at your option) any later version.\n"
		"\n"
		"This program is distributed in the hope that it will be useful,\n"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		"GNU General Public License for more details.\n"
		"\n"
		"You should have received a copy of the GNU General Public License\n"
		"along with this program; if not, write to the Free Software\n"
		"Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.\n"
		"===============================================================================\n"
		"\n"
		"Usage: udefrag [command] [options] [volumeletter:]\n"
		"\n"
		"  The default action is to display this help message.\n"
		"\n"
		"Commands:\n"
		"  -a,  --analyze                      Analyze volume\n"
		"       --defragment                   Defragment volume\n"
		"  -o,  --optimize                     Optimize volume space\n"
		"  -l,  --list-available-volumes       List volumes available\n"
		"                                      for defragmentation,\n"
		"                                      except removable media\n"
		"  -la, --list-available-volumes=all   List all available volumes\n"
		"  -h,  --help                         Show this help screen\n"
		"  -?                                  Show this help screen\n"
		"\n"
		"  If command is not specified it will defragment volume.\n"
		"\n"
		"Options:\n"
		"  -b,  --use-system-color-scheme      Use system (usually black/white)\n"
		"                                      color scheme instead of the green color.\n"
		"  -p,  --suppress-progress-indicator  Hide progress indicator.\n"
		"  -v,  --show-volume-information      Show volume information after a job.\n"
		"  -m,  --show-cluster-map             Show map representing clusters\n"
		"                                      on the volume.\n"
		"       --map-border-color=color       Set cluster map border color.\n"
		"                                      Available color values: black, white,\n"
		"                                      red, green, blue, yellow, magenta, cyan,\n"
		"                                      darkred, darkgreen, darkblue, darkyellow,\n"
		"                                      darkmagenta, darkcyan, gray.\n"
		"                                      Yellow color is used by default.\n"
		"       --map-symbol=x                 Set a character used for the map drawing.\n"
		"                                      There are two accepted formats:\n"
		"                                      a character may be typed directly,\n"
		"                                      or its hexadecimal number may be used.\n"
		"                                      For example, --map-symbol=0x1 forces\n"
		"                                      UltraDefrag to use a smile character\n"
		"                                      for the map drawing.\n"
		"                                      Valid numbers are in range: 0x1 - 0xFF\n"
		"                                      \'%%\' symbol is used by default.\n"
		"       --map-rows=n                   Number of rows in cluster map.\n"
		"                                      Default value is 10.\n"
		"       --map-symbols-per-line=n       Number of map symbols\n"
		"                                      containing in each row of the map.\n"
		"                                      Default value is 68.\n"
		"\n"
		"Accepted environment variables:\n"
		"\n"
		"  UD_IN_FILTER                        List of files to be included\n"
		"                                      in defragmentation process. File names\n"
		"                                      must be separated by semicolons.\n"
		"\n"
		"  UD_EX_FILTER                        List of files to be excluded from\n"
		"                                      defragmentation process. File names\n"
		"                                      must be separated by semicolons.\n"
		"\n"
		"  UD_SIZELIMIT                        Exclude all files larger than specified.\n"
		"                                      The following size suffixes are accepted:\n"
		"                                      Kb, Mb, Gb, Tb, Pb, Eb.\n"
		"\n"
		"  UD_FRAGMENTS_THRESHOLD              Exclude all files which have less number\n"
		"                                      of fragments than specified.\n"
		"\n"
		"  UD_TIME_LIMIT                       When the specified time interval elapses\n"
		"                                      the job will be stopped automatically.\n"
		"                                      The following time format is accepted:\n"
		"                                      Ay Bd Ch Dm Es. Here A,B,C,D,E represent\n"
		"                                      any integer numbers, y,d,h,m,s - suffixes\n"
		"                                      used for years, days, hours, minutes\n"
		"                                      and seconds.\n"
		"\n"
		"  UD_REFRESH_INTERVAL                 Progress refresh interval,\n"
		"                                      in milliseconds.\n"
		"                                      The default value is 500.\n"
		"\n"
		"  UD_DISABLE_REPORTS                  If this environment variable is set\n"
		"                                      to 1 (one), no file fragmentation\n"
		"                                      reports will be generated.\n"
		"\n"
		"  UD_DBGPRINT_LEVEL                   Control amount of the debugging output.\n"
		"                                      NORMAL is used by default, DETAILED\n"
		"                                      may be used to collect information for\n"
		"                                      the bug report, PARANOID turns on\n"
		"                                      a really huge amount of debugging\n"
		"                                      information.\n"
		);
}

static struct option long_options_[] = {
	/*
	* Disk defragmenting options.
	*/
	{ "analyze",                     no_argument,       0, 'a' },
	{ "defragment",                  no_argument,       0,  0  },
	{ "optimize",                    no_argument,       0, 'o' },
	
	/*
	* Volume listing options.
	*/
	{ "list-available-volumes",      optional_argument, 0, 'l' },
	
	/*
	* Progress indicators options.
	*/
	{ "suppress-progress-indicator", no_argument,       0, 'p' },
	{ "show-volume-information",     no_argument,       0, 'v' },
	{ "show-cluster-map",            no_argument,       0, 'm' },
	
	/*
	* Colors and decoration.
	*/
	{ "use-system-color-scheme",     no_argument,       0, 'b' },
	{ "map-border-color",            required_argument, 0,  0  },
	{ "map-symbol",                  required_argument, 0,  0  },
	{ "map-rows",                    required_argument, 0,  0  },
	{ "map-symbols-per-line",        required_argument, 0,  0  },
	
	/*
	* Help.
	*/
	{ "help",                        no_argument,       0, 'h' },
	
	/*
	* Screensaver options.
	*/
	{ "screensaver",                 no_argument,       0,  0  },
	
	{ 0,                             0,                 0,  0  }
};

char short_options_[] = "aol::pvmbh?iesd";

/* new code based on GNU getopt() function */
void parse_cmdline(int argc, char **argv)
{
	int c;
	int option_index = 0;
	const char *long_option_name;
	int dark_color_flag = 0;
	int map_symbol_number = 0;
	int rows = 0, symbols_per_line = 0;
	
	if(argc < 2) h_flag = 1;
	while(1){
		option_index = 0;
		c = getopt_long(argc,argv,short_options_,
			long_options_,&option_index);
		if(c == -1) break;
		switch(c){
		case 0:
			//printf("option %s", long_options_[option_index].name);
			//if(optarg) printf(" with arg %s", optarg);
			//printf("\n");
			long_option_name = long_options_[option_index].name;
			if(!strcmp(long_option_name,"defragment")) { /* do nothing here */ }
			else if(!strcmp(long_option_name,"map-border-color")){
				if(!optarg) break;
				if(!strcmp(optarg,"black")){
					map_border_color = 0x0; break;
				}
				if(!strcmp(optarg,"white")){
					map_border_color = FOREGROUND_RED | FOREGROUND_GREEN | \
						FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
				}
				if(!strcmp(optarg,"gray")){
					map_border_color = FOREGROUND_RED | FOREGROUND_GREEN | \
						FOREGROUND_BLUE; break;
				}
				if(strstr(optarg,"dark")) dark_color_flag = 1;

				if(strstr(optarg,"red")){
					map_border_color = FOREGROUND_RED;
				}
				else if(strstr(optarg,"green")){
					map_border_color = FOREGROUND_GREEN;
				}
				else if(strstr(optarg,"blue")){
					map_border_color = FOREGROUND_BLUE;
				}
				else if(strstr(optarg,"yellow")){
					map_border_color = FOREGROUND_RED | FOREGROUND_GREEN;
				}
				else if(strstr(optarg,"magenta")){
					map_border_color = FOREGROUND_RED | FOREGROUND_BLUE;
				}
				else if(strstr(optarg,"cyan")){
					map_border_color = FOREGROUND_GREEN | FOREGROUND_BLUE;
				}
				
				if(!dark_color_flag) map_border_color |= FOREGROUND_INTENSITY;
			}
			else if(!strcmp(long_option_name,"map-symbol")){
				if(!optarg) break;
				if(strstr(optarg,"0x") == optarg){
					/* decode hexadecimal number */
					(void)sscanf(optarg,"%x",&map_symbol_number);
					if(map_symbol_number > 0 && map_symbol_number < 256)
						map_symbol = (char)map_symbol_number;
				} else {
					if(optarg[0]) map_symbol = optarg[0];
				}
			}
			else if(!strcmp(long_option_name,"screensaver")){
				screensaver_mode = 1;
			}
			else if(!strcmp(long_option_name,"map-rows")){
				if(!optarg) break;
				rows = atoi(optarg);
				if(rows > 0) map_rows = rows;
			}
			else if(!strcmp(long_option_name,"map-symbols-per-line")){
				if(!optarg) break;
				symbols_per_line = atoi(optarg);
				if(symbols_per_line > 0) map_symbols_per_line = symbols_per_line;
			}
			break;
		case 'a':
			a_flag = 1;
			break;
		case 'o':
			o_flag = 1;
			break;
		case 'l':
			l_flag = 1;
			if(optarg){
				if(!strcmp(optarg,"a")) la_flag = 1;
				if(!strcmp(optarg,"all")) la_flag = 1;
			}
			break;
		case 'p':
			p_flag = 1;
			break;
		case 'v':
			v_flag = 1;
			break;
		case 'm':
			m_flag = 1;
			break;
		case 'b':
			b_flag = 1;
			break;
		case 'h':
			h_flag = 1;
			break;
		case 'i':
		case 'e':
		case 's':
		case 'd':
			obsolete_option = 1;
			break;
		case '?': /* invalid option or -? option */
			if(optopt == '?') h_flag = 1;
			break;
		default:
			printf("?? getopt returned character code 0%o ??\n", c);
		}
	}
	
	if(optind < argc){ /* scan for volume letters */
		//printf("non-option ARGV-elements: ");
		while(optind < argc){
			//printf("%s ", argv[optind]);
			if(argv[optind][0]){
				/* next check supports UltraDefrag context menu handler */
				if(argv[optind][1] == ':')
					letter = argv[optind][0];
			}
			optind++;
		}
		//printf("\n");
	}
	
	if(!l_flag && !letter) h_flag = 1;
}
