/*****************************************************************************
* Vector Mame Menu - MakeINI
*
* Generate a vmmenu.ini file containing all the vector games
* listed in your mame.xml file
*
* Author:  Chad Gray
* Created: 10/11/09
*
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char * searchfor(char*, char*, int, char*);
void printhelp(void);

int main(int argc, char *argv[])
{
	if( argc == 2 )
	{
		if (!strcmp(argv[1], "-help"))
		{
			printhelp();
			exit(0);
		}
	}
	char	temp[200], manuf[50], desc[50], mame[50], clone[50];
	FILE	*fp;
	fp = fopen ("mame.xml","rt" );
	if (fp == NULL)
	{	
		printf("* Error - Unable to open input file mame.xml\n\n");
		printhelp();
		exit(1) ;
	}
	clone[0]	= 0;
	mame[0]		= 0;
	desc[0]		= 0;
	manuf[0]	= 0;
	while (fgets(temp, 199, fp) != NULL)
	{
		if (strstr(temp, "<game name") != NULL) searchfor(temp, "<game name=", 34, mame);
		if (strstr(temp, "<description") != NULL) searchfor(temp, "<description", 60, desc);
		if (strstr(temp, "<manufacturer") != NULL)
		{
			searchfor(temp, "<manufacturer", 60, manuf);
			if (strstr(manuf, " (") != NULL)		manuf[strstr(manuf, " (") - manuf] = 0;
		}
		if (strstr(temp, "cloneof=") != NULL)	searchfor(temp, "cloneof=", 34, clone);
		if ( (strstr(temp, "<display") || strstr(temp, "<video")) && (strstr(temp, "vector")) )
		{
			if (strlen(clone) == 0) strcpy(clone, mame);
				printf("%s|%s|%s|%s\n", manuf, desc, clone, mame);
		}
		if (strstr(temp, "</game>") != NULL)
			clone[0] = 0;
	}
	fclose(fp);
	exit(0);
}

char * searchfor(char* text, char* lookfor, int delimiter, char *mytemp)
{
	int pos, pos2 = 0;
	pos = (strstr(text, lookfor) - text) + strlen(lookfor) + 1;
	while (text[pos] != delimiter)
	{
		mytemp[pos2] = text[pos];
		pos++;
		pos2++;
	}
	mytemp[pos2] = 0;
	return mytemp;
}

void printhelp()
{
	printf("VMenu ini file creator v1.00\n");
	printf("============================\n\n");
	printf("Usage:\nmakeini.exe >vmmenu.ini\n\n");
	printf("This utility should be run from the same directory as your mame.xml file\n");
	printf("to create the vmmenu.ini file required by vmenu.exe\n\n");
	printf("You can create the mame.xml file by running:\n");
	printf("mame.exe -listxml >mame.xml\n\n");
	printf("Substitute mame.exe with the name of your mame variant if necessary.\n\n");
}

