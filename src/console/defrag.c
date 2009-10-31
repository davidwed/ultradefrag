/*
 *  UltraDefrag - powerful defragmentation tool for Windows NT.
 *  Copyright (c) 2007,2008 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
* UltraDefrag console interface.
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "../include/udefrag.h"
#include "../include/ultradfgver.h"

#include "../include/getopt.h"

#define settextcolor(c) SetConsoleTextAttribute(hOut,c)

/* global variables */
HANDLE hOut;
WORD console_attr = 0x7;
int a_flag = 0,o_flag = 0;
int b_flag = 0,h_flag = 0;
int l_flag = 0,la_flag = 0;
int p_flag = 0,v_flag = 0;
int m_flag = 0;
int obsolete_option = 0;
char letter = 0;
char unk_opt[] = "Unknown option: x!";
int unknown_option = 0;

int screensaver_mode = 0;

#define BLOCKS_PER_HLINE  68//60//79
#define BLOCKS_PER_VLINE  10//8//16
#define N_BLOCKS          (BLOCKS_PER_HLINE * BLOCKS_PER_VLINE)

char *cluster_map = NULL;

/* internal functions prototypes */
void HandleError(char *err_msg,int exit_code);
BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType);
void __stdcall ErrorHandler(short *msg);

void Exit(int exit_code)
{
	if(!b_flag) settextcolor(console_attr);
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,FALSE);
	udefrag_unload();
	if(cluster_map) free(cluster_map);
	exit(exit_code);
}

void HandleError(char *err_msg,int exit_code)
{
	if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
	printf("%s\n",err_msg);
	Exit(exit_code);
}

/* returns an exit code for console program terminating */
int show_vollist(void)
{
	volume_info *v;
	int n;
	char s[32];
	double d;
	int p;

	if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	printf("Volumes available for defragmentation:\n\n");
	udefrag_set_error_handler(NULL);

	if(udefrag_get_avail_volumes(&v,la_flag ? FALSE : TRUE) < 0){
		if(!b_flag) settextcolor(console_attr);
		udefrag_set_error_handler(ErrorHandler);
		return 1;
	} else {
		for(n = 0;;n++){
			if(v[n].letter == 0) break;
			udefrag_fbsize((ULONGLONG)(v[n].total_space.QuadPart),2,s,sizeof(s));
			d = (double)(signed __int64)(v[n].free_space.QuadPart);
			/* 0.1 constant is used to exclude divide by zero error */
			d /= ((double)(signed __int64)(v[n].total_space.QuadPart) + 0.1);
			p = (int)(100 * d);
			if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			printf("%c:  %8s %12s %8u %%\n",
				v[n].letter,v[n].fsname,s,p);
		}
	}

	if(!b_flag) settextcolor(console_attr);
	udefrag_set_error_handler(ErrorHandler);
	return 0;
}

BOOL stop_flag = FALSE;

BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType)
{
	stop_flag = TRUE;
	udefrag_stop();
	return TRUE;
}

void __stdcall ErrorHandler(short *msg)
{
	char oem_buffer[1024]; /* see zenwinx.h ERR_MSG_SIZE */
	WORD color = FOREGROUND_RED | FOREGROUND_INTENSITY;

	/* ignore notifications */
	if((short *)wcsstr(msg,L"N: ") == msg) return;
	
	if(!b_flag){
		if((short *)wcsstr(msg,L"W: ") == msg)
			color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
		settextcolor(color);
	}
	/* skip W: and E: labels */
	if((short *)wcsstr(msg,L"W: ") == msg || (short *)wcsstr(msg,L"E: ") == msg)
		WideCharToMultiByte(CP_OEMCP,0,msg + 3,-1,oem_buffer,1023,NULL,NULL);
	else
		WideCharToMultiByte(CP_OEMCP,0,msg,-1,oem_buffer,1023,NULL,NULL);
	printf("%s\n",oem_buffer);
	if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

void RunScreenSaver(void)
{
	printf("Hello!\n");
}

BOOL first_run = TRUE;
BOOL err_flag = FALSE;
BOOL err_flag2 = FALSE;
//char last_op = 0;

WORD colors[NUM_OF_SPACE_STATES] = {
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	FOREGROUND_GREEN,
	FOREGROUND_RED | FOREGROUND_INTENSITY,
	FOREGROUND_RED,
	FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	FOREGROUND_BLUE,
	FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	FOREGROUND_RED | FOREGROUND_GREEN,
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	FOREGROUND_RED | FOREGROUND_GREEN,
	FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
};

WORD bcolors[NUM_OF_SPACE_STATES] = {
	BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY,
	BACKGROUND_GREEN | BACKGROUND_INTENSITY,
	BACKGROUND_GREEN,
	BACKGROUND_RED | BACKGROUND_INTENSITY,
	BACKGROUND_RED,
	BACKGROUND_BLUE | BACKGROUND_INTENSITY,
	BACKGROUND_BLUE,
	BACKGROUND_RED | BACKGROUND_BLUE | FOREGROUND_INTENSITY,
	BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY,
	BACKGROUND_RED | BACKGROUND_GREEN,
	BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY,
	BACKGROUND_RED | BACKGROUND_GREEN,
	BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY,
};

#define MAP_SYMBOL '%'
BOOL map_completed = FALSE;

#define BORDER_COLOR (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)

WORD map_border_color = BORDER_COLOR;
char map_symbol = MAP_SYMBOL;
int map_rows = BLOCKS_PER_VLINE;
int map_symbols_per_line = BLOCKS_PER_HLINE;

void RedrawMap(void)
{
	int i,j;
	WORD color, prev_color = 0x0;
	char c[2];
	WORD border_color = map_border_color;

	fprintf(stderr,"\n\n");
	
	settextcolor(border_color);
	prev_color = border_color;
	c[0] = 0xC9; c[1] = 0;
	fprintf(stderr,c);
	for(j = 0; j < map_symbols_per_line; j++){
		c[0] = 0xCD; c[1] = 0;
		fprintf(stderr,c);
	}
	c[0] = 0xBB; c[1] = 0;
	fprintf(stderr,c);
	fprintf(stderr,"\n");

	for(i = 0; i < map_rows; i++){
		if(border_color != prev_color) settextcolor(border_color);
		prev_color = border_color;
		c[0] = 0xBA; c[1] = 0;
		fprintf(stderr,c);
		for(j = 0; j < map_symbols_per_line; j++){
			color = colors[(int)cluster_map[i * map_symbols_per_line + j]];
			if(color != prev_color) settextcolor(color);
			prev_color = color;
			c[0] = map_symbol; c[1] = 0;
			fprintf(stderr,"%s",c);
		}
		if(border_color != prev_color) settextcolor(border_color);
		prev_color = border_color;
		c[0] = 0xBA; c[1] = 0;
		fprintf(stderr,c);
		fprintf(stderr,"\n");
	}

	if(border_color != prev_color) settextcolor(border_color);
	prev_color = border_color;
	c[0] = 0xC8; c[1] = 0;
	fprintf(stderr,c);
	for(j = 0; j < map_symbols_per_line; j++){
		c[0] = 0xCD; c[1] = 0;
		fprintf(stderr,c);
	}
	c[0] = 0xBC; c[1] = 0;
	fprintf(stderr,c);
	fprintf(stderr,"\n");

	fprintf(stderr,"\n");
	if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	else settextcolor(console_attr);
	map_completed = TRUE;
}

int __stdcall ProgressCallback(int done_flag)
{
	STATISTIC stat;
	char op; char *op_name = ""/*, *last_op_name*/;
	double percentage;
	ERRORHANDLERPROC eh = NULL;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	COORD cursor_pos;
	
	if(err_flag || err_flag2) return 0;
	
	if(first_run){
		/*
		* Ultra fast ntfs analysis contains one piece of code 
		* that heavily loads CPU for one-two seconds.
		*/
		SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_ABOVE_NORMAL);
		first_run = FALSE;
	}
	
	if(map_completed){
		if(!GetConsoleScreenBufferInfo(hOut,&csbi)){
			if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
			printf("\nCannot retrieve cursor position!\n");
			if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			return 0;
		}
		cursor_pos.X = 0;
		cursor_pos.Y = csbi.dwCursorPosition.Y - map_rows - 3 - 2;
		if(!SetConsoleCursorPosition(hOut,cursor_pos)){
			if(!b_flag) settextcolor(FOREGROUND_RED | FOREGROUND_INTENSITY);
			printf("\nCannot set cursor position!\n");
			if(!b_flag) settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			return 0;
		}
	}
	
	/* show error message no more than once */
	if(err_flag) eh = udefrag_set_error_handler(NULL);
	if(udefrag_get_progress(&stat,&percentage) >= 0){
		op = stat.current_operation;
		if(op == 'A' || op == 'a')      op_name = "analyse:  ";
		else if(op == 'D' || op == 'd') op_name = "defrag:   ";
		else                            op_name = "optimize: ";

		//if(last_op == 'A' || last_op == 'a')      last_op_name = "Analyse:  ";
		//else if(last_op == 'D' || last_op == 'd') last_op_name = "Defrag:   ";
		//else                                      last_op_name = "Optimize: ";

		//if(op != 'A' && !last_op) fprintf(stderr,"\rAnalyse:  100%%\n");
		//if(op != last_op && last_op) fprintf(stderr,"\r%s100%%\n",last_op_name);
		//last_op = op;
		fprintf(stderr,"\r%c: %s%3u%% complete, fragmented/total = %lu/%lu",
			letter,op_name,(int)percentage,stat.fragmfilecounter,stat.filecounter);
	} else {
		err_flag = TRUE;
	}
	if(eh) udefrag_set_error_handler(eh);
	
	if(err_flag) return 0;

	if(done_flag && !stop_flag){ /* set progress indicator to 100% state */
		fprintf(stderr,"\r%c: %s100%% complete, fragmented/total = %lu/%lu",
			letter,op_name,stat.fragmfilecounter,stat.filecounter);
		if(!m_flag) fprintf(stderr,"\n");
	}
	
	if(m_flag){ /* display cluster map */
		/* show error message no more than once */
		if(err_flag2) eh = udefrag_set_error_handler(NULL);
		if(udefrag_get_map(cluster_map,map_rows * map_symbols_per_line) >= 0){
			RedrawMap();
		} else {
			err_flag2 = TRUE;
		}
		if(eh) udefrag_set_error_handler(eh);
	}
	
	return 0;
}

/* oblsolete code */
void parse_cmdline__(int argc, char **argv)
{
	int i;
	char c1,c2,c3;

	if(argc < 2) h_flag = 1;
	for(i = 1; i < argc; i++){
		c1 = argv[i][0];
		if(!c1) continue;
		c2 = argv[i][1];
		/* next check supports UltraDefrag context menu handler */
		if(c2 == ':') letter = (char)c1;
		if(c1 == '-'){
			c2 = (char)tolower((int)c2);
			if(!strchr("abosideh?lpvm",c2)){
				/* unknown option */
				unk_opt[16] = c2;
				unknown_option = 1;
			}
			if(strchr("iesd",c2))
				obsolete_option = 1;
			if(c2 == 'h' || c2 == '?')
				h_flag = 1;
			else if(c2 == 'a') a_flag = 1;
			else if(c2 == 'o') o_flag = 1;
			else if(c2 == 'b') b_flag = 1;
			else if(c2 == 'l'){
				l_flag = 1;
				c3 = (char)tolower((int)argv[i][2]);
				if(c3 == 'a') la_flag = 1;
			}
			else if(c2 == 'p') p_flag = 1;
			else if(c2 == 'v') v_flag = 1;
			else if(c2 == 'm') m_flag = 1;
		}
	}
	/* if only -b or -p options are specified, show help message */
	//if(argc == 2 && b_flag) h_flag = 1;
	if(!l_flag && !letter) h_flag = 1;
}

void show_help(void)
{
	printf(
		"===============================================================================\n"
		VERSIONINTITLE " - Powerful disk defragmentation tool for Windows NT\n"
		"Copyright (c) Dmitri Arkhangelski, 2007-2009.\n"
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
		"  -a,  --analyze                      Analyze specified volume only\n"
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
		"  -b,  --use-system-color-scheme      Use B/W scheme instead of green color\n"
		"  -p,  --suppress-progress-indicator  Hide progress indicator\n"
		"  -v,  --show-volume-information      Show volume information after a job\n"
		"  -m,  --show-cluster-map             Show map representing clusters\n"
		"                                      on specified volume\n"
		"       --map-border-color=color       Specify cluster map border color here.\n"
		"                                      Available color values: black, white,\n"
		"                                      red, green, blue, yellow, magenta, cyan,\n"
		"                                      darkred, darkgreen, darkblue, darkyellow,\n"
		"                                      darkmagenta, darkcyan, gray.\n"
		"                                      Yellow color is used by default.\n"
		"       --map-symbol=x                 You can specify map symbol here.\n"
		"                                      There are two supported formats: you can\n"
		"                                      type symbol directly or specify\n"
		"                                      its number in hexadecimal form. \n"
		"                                      Like this: --map-symbol=0x1 :-)\n"
		"                                      Valid numbers are in range: 0x1 - 0xFF\n"
		"                                      \'%%\' symbol is used by default.\n"
		"       --map-rows=n                   Number of rows in cluster map.\n"
		"                                      Default value is 10.\n"
		"       --map-symbols-per-line=n       Number of map symbols\n"
		"                                      containing in each row of map.\n"
		"                                      Default value is 68.\n"
		"\n"
		"  Volume defragmentation options can be specified through system\n"
		"  environment variables as described in UltraDefrag Handbook.\n"
		"\n"
		"Accepted environment variables:\n"
		"\n"
		"  UD_IN_FILTER                        List of files to be included\n"
		"                                      in defragmentation process. File names\n"
		"                                      must be separated by semicolon. P.a.:\n"
		"                                      set UD_IN_FILTER=My Documents. After\n"
		"                                      this assignment defragger will process\n"
		"                                      My Documents directory contents only.\n"
		"                                      The default value is an empty string\n"
		"                                      that means: all files will be included.\n"
		"\n"
		"  UD_EX_FILTER                        List of files to be excluded from\n"
		"                                      defragmentation process. File names\n"
		"                                      must be separated by semicolon. P.a.:\n"
		"                                      set UD_EX_FILTER=\n"
		"                                      system volume information;temp;recycler.\n"
		"                                      After this assignment many\n"
		"                                      temporary files will be excluded from the\n"
		"                                      defragmentation process. The default\n"
		"                                      value is an empty string. It means:\n"
		"                                      no files will be excluded.\n"
		"\n"
		"  UD_SIZELIMIT                        Ignore files larger than specified value.\n"
		"                                      You can either specify size in bytes or\n"
		"                                      use the following suffixes: Kb, Mb, Gb,\n"
		"                                      Tb, Pb, Eb. P.a., use\n"
		"                                      set UD_SIZELIMIT=10Mb to exclude\n"
		"                                      all files greater than 10 megabytes.\n"
		"                                      The default value is zero. It means:\n"
		"                                      there is no size limit.\n"
		"\n"
		"  UD_REFRESH_INTERVAL                 Specify the progress indicator refresh\n"
		"                                      interval in milliseconds. The default\n"
		"                                      value is 500.\n"
		"\n"
		"  UD_DISABLE_REPORTS                  Set this parameter to 1 (one) to disable\n"
		"                                      reports generation. The default\n"
		"                                      value is 0.\n"
		"\n"
		"  UD_DBGPRINT_LEVEL                   This parameter can be in one of three\n"
		"                                      states. Set UD_DBGPRINT_LEVEL=NORMAL\n"
		"                                      to view useful messages about the analyse\n"
		"                                      or defrag progress. Select DETAILED\n"
		"                                      to create a bug report to send to the\n"
		"                                      author when an error is encountered.\n"
		"                                      Select PARANOID in extraordinary cases.\n"
		"                                      The default value is NORMAL. Logs can be\n"
		"                                      found in %%windir%%\\Ultradefrag\\logs\n"
		"                                      directory.\n"
		);
	Exit(0);
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
					sscanf(optarg,"%x",&map_symbol_number);
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

int __cdecl main(int argc, char **argv)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	STATISTIC stat;

	/* analyse command line */
	parse_cmdline(argc,argv);

	hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if(GetConsoleScreenBufferInfo(hOut,&csbi))
		console_attr = csbi.wAttributes;
	if(!b_flag)
		settextcolor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);

	if(m_flag){
		cluster_map = malloc(map_rows * map_symbols_per_line);
		if(!cluster_map){
			printf("Cannot allocate %i bytes of memory for cluster map!\n\n",
				map_rows * map_symbols_per_line);
			return 1;
		}
		memset(cluster_map,0,map_rows * map_symbols_per_line);
	}
	
	if(screensaver_mode){
		RunScreenSaver();
		Exit(0);
	}

	if(h_flag) show_help();

	/* display copyright */
	printf(VERSIONINTITLE ", " //"console interface\n"
			"Copyright (c) Dmitri Arkhangelski, 2007-2009.\n"
			"UltraDefrag comes with ABSOLUTELY NO WARRANTY. This is free software, \n"
			"and you are welcome to redistribute it under certain conditions.\n\n");
	/* handle unknown option and help request */
	if(unknown_option) HandleError(unk_opt,1);
	if(obsolete_option)
		HandleError("The -i, -e, -s, -d options are oblolete.\n"
					"Use environment variables instead!",1);

	/* set udefrag.dll ErrorHandler */
	udefrag_set_error_handler(ErrorHandler);
	
	/* show list of volumes if requested */
	if(l_flag){ exit(show_vollist()); }
	/* validate driveletter */
	if(!letter)	HandleError("Drive letter should be specified!",1);
	if(udefrag_validate_volume(letter,FALSE) < 0) Exit(1);
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine,TRUE);
	/* do our job */
	if(!m_flag){
		if(udefrag_init(0) < 0) Exit(2);
	} else {
		if(udefrag_init(map_rows * map_symbols_per_line) < 0) Exit(2);
	}

	if(udefrag_kernel_mode()) printf("(Kernel Mode)\n\n");
	else printf("(User Mode)\n\n");

	if(m_flag){ /* prepare console buffer for map */
		fprintf(stderr,"\r%c: %s%3u%% complete, fragmented/total = %u/%u",
			letter,"analyse:  ",0,0,0);
		RedrawMap();
	}
	
	if(!p_flag){
		if(a_flag) {
			//printf("Preparing to analyse volume %c: ...\n\n",letter);
			if(udefrag_analyse(letter,ProgressCallback) < 0) Exit(3);
		}
		else if(o_flag) {
			//printf("Preparing to optimize volume %c: ...\n\n",letter);
			if(udefrag_optimize(letter,ProgressCallback) < 0) Exit(3);
		}
		else { 
			//printf("Preparing to defragment volume %c: ...\n\n",letter);
			if(udefrag_defragment(letter,ProgressCallback) < 0) Exit(3);
		}
	} else {
		if(a_flag) { if(udefrag_analyse(letter,NULL) < 0) Exit(3); }
		else if(o_flag) { if(udefrag_optimize(letter,NULL) < 0) Exit(3); }
		else { if(udefrag_defragment(letter,NULL) < 0) Exit(3); }
	}

	/* display results and exit */
	if(v_flag){
		if(udefrag_get_progress(&stat,NULL) >= 0)
			printf("\n%s",udefrag_get_default_formatted_results(&stat));
	}
	Exit(0);
	return 0; /* we will never reach this point */
}
