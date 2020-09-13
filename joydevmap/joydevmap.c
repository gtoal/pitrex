#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <linux/joystick.h>

#include "config.h"


#define BTN_CNT (KEY_MAX - BTN_MISC + 1)

#define EXIT_INVARG 1
#define EXIT_OPEN 2
#define EXIT_IOCTL 3
#define EXIT_VERSION 4
#define EXIT_VALUE 5



void reportMissingArgument(const char* opt)
{
	fprintf(stderr, "Missing argument for option '%s'.\n", opt);
	exit(EXIT_INVARG);
}


void printHelp(FILE* out, int argc, char** argv)
{
	(void) argc;

	fprintf(out, "Usage:\n"
			"  %s -h\n"
			"  %s --help\n"
			"  %s {commands} --dev <device>\n"
			"\n"
			"Commands:\n"
			"  --list-axismap\n"
			"  --list-buttonmap\n"
			"  --set-axismap 'id, id, id...'\n"
			"  --set-buttonmap 'id, id, id...'\n",
			argv[0], argv[0], argv[0]);
}


__u8 getNumberOfAxes(int fd)
{
	__u8 cnt=0;
	if (ioctl(fd, JSIOCGAXES, &cnt) )
	{
		perror(PACKAGE_STRING ": error getting number of axes");
		exit(EXIT_IOCTL);
	}
	return cnt;
}


__u8 getNumberOfButtons(int fd)
{
	__u8 cnt=0;
	if (ioctl(fd, JSIOCGBUTTONS, &cnt) )
	{
		perror(PACKAGE_STRING ": error getting number of buttons");
		exit(EXIT_IOCTL);
	}
	return cnt;
}


void listAxismap(int fd)
{
	__u8 map[ABS_CNT] = {-1};
	const __u8 cnt = getNumberOfAxes(fd);
	printf("Got %d axes:\n", cnt);
	assert( cnt <= ABS_CNT);

	if( ioctl(fd, JSIOCGAXMAP, &map) < 0 )
	{
		perror(PACKAGE_STRING ": error getting axis map");
		exit(1);
	}
	
	for(int i=0; i<cnt; i++)
		printf("  %d => %d\n", i, map[i] );

	printf("\n");
	printf("Set this mapping with:\n");
	printf("  " PACKAGE_STRING " --set-axismap '");
	for(int i=0; i<cnt; i++)
	{
		if( i> 0)
			printf(", ");
		printf("%d", map[i] );
	}
	printf("'\n");
}


void listButtonmap(int fd)
{
	__u16 map[BTN_CNT] = {-1};
	const __u8 cnt = getNumberOfButtons(fd);
	printf("Got %d buttons:\n", cnt);

	if( ioctl(fd, JSIOCGBTNMAP, &map) < 0 )
	{
		perror(PACKAGE_STRING ": error getting button map");
		exit(1);
	}
	
	for(int i=0; i<cnt; i++)
		printf("  %d => %d\n", i, map[i] );

	printf("\n");
	printf("Set this mapping with:\n");
	printf("  " PACKAGE_STRING " --set-buttonmap '");
	for(int i=0; i<cnt; i++)
	{
		if( i> 0)
			printf(", ");
		printf("%d", map[i] );
	}
	printf("'\n");
}


/** Fill an integer array of given size by comma-separated values in str.
 * Return the number of values contained in str. */
unsigned parseIntArray(int* array, unsigned size, char* str)
{
	unsigned i;
	for(i=0; i<size; ++i)
	{
		sscanf(str, "%d", &array[i]);

		// Move up to next comma, and one step further
		str = strstr(str, ",");
		if( !str )
			break;
		str++;
	}

	// Skip further commas
	while( str )
	{
		i++;
		str = strstr(str, ",");
		if( !str )
			break;
		str++;
	}

	return i+1;
}


/** Take an array of size that contains cnt values and fill the rest of array
 * such that a permutation on {0, ..., size-1} results.
 * Returns 1 if resulting array is a permutation */
int fillPermutation(int* array, unsigned cnt, unsigned size)
{
	assert( cnt <= size );

	int* contains = malloc(sizeof(int)*size );
	memset(contains, 0, sizeof(int)*size );


	// Check which value is already there
	for(unsigned i=0; i<cnt; ++i)
	{
		const unsigned v = array[i];
		if( v>=size || contains[v] )
			return 0;
		contains[v] = 1;
	}

	// Do it the hard way on array[cnt, max]
	unsigned min=0;
	for(unsigned i=cnt; i<size; ++i)
	{
		// Get minimum free value
		while( contains[min] )
		{
			min++;
			assert( min<size );
		}

		array[i] = min;
		contains[min] = 1;
	}

	return 1;
}


void setAxismap(int fd, char* mapstr)
{
	__u8 map[ABS_CNT] = {-1};
	int imap[ABS_CNT] = {-1};


	const __u8 cnt = getNumberOfAxes(fd);
	printf("Setting %d axes.\n", cnt);
	assert( cnt <= ABS_CNT);


	const unsigned readcnt = parseIntArray(imap, cnt, mapstr);
	if( readcnt != cnt )
	{
		fprintf(stderr, "Invalid number of values given: %d\n", readcnt);
		exit(EXIT_VALUE);
	}

	// Nothing to do
	if( cnt<=0 )
		return;

	// Fill the remaining array such that we obtain a permutation
	if( !fillPermutation(imap, cnt, ABS_CNT) )
	{
		fprintf(stderr, "Mapping needs to be a permutation.\n");
		exit(EXIT_VALUE);
	}

	// Copy array and check bounds
	for(unsigned i=0; i<ABS_CNT; ++i)
	{
		if( imap[i]<0 || imap[i]>=ABS_CNT )
		{
			fprintf(stderr, "Value out of bounds: %d\n", imap[i]);
			exit(EXIT_VALUE);
		}

		map[i] = imap[i];
	}


	printf("Setting the following map:\n");	
	for(int i=0; i<cnt; i++)
		printf("  %d => %d\n", i, map[i] );


	if( ioctl(fd, JSIOCSAXMAP, &map) )
	{
		perror( PACKAGE_STRING ": error setting axis map");
		exit(EXIT_IOCTL);
	}
}


void setButtonmap(int fd, char* mapstr)
{
	__u16 map[BTN_CNT] = {-1};
	int imap[BTN_CNT] = {-1};

	const __u8 cnt = getNumberOfButtons(fd);
	printf("Setting %d buttons.\n", cnt);
	assert( cnt <= ABS_CNT);


	const unsigned readcnt = parseIntArray(imap, cnt, mapstr);
	if( readcnt != cnt )
	{
		fprintf(stderr, "Invalid number of values given: %d\n", readcnt);
		exit(EXIT_VALUE);
	}

	// Nothing to do
	if( cnt<=0 )
		return;

	// Fill the remaining array such that we obtain a permutation
	if( !fillPermutation(imap, cnt, BTN_CNT) )
	{
		fprintf(stderr, "Mapping needs to be a permutation.\n");
		exit(EXIT_VALUE);
	}

	// Copy array and check bounds
	for(unsigned i=0; i<ABS_CNT; ++i)
	{
		if( imap[i]<0 || imap[i]>=BTN_CNT )
		{
			fprintf(stderr, "Value out of bounds: %d\n", imap[i]);
			exit(EXIT_VALUE);
		}

		map[i] = imap[i];
	}


	printf("Setting the following map:\n");	
	for(int i=0; i<cnt; i++)
		printf("  %d => %d\n", i, map[i] );


	if( ioctl(fd, JSIOCSBTNMAP, &map) )
	{
		perror( PACKAGE_STRING ": error setting axis map");
		exit(EXIT_IOCTL);
	}
}


int main(int argc, char** argv)
{
	int argListAxismap=0;
	int argListButtonmap=0;
	char* argSetAxismap=0;
	char* argSetButtonmap=0;
	char* dev=0;


	for(int i=1; i<argc; ++i)
	{
		const char* opt = argv[i];

		if( !strcmp(opt,"--help") || !strcmp(opt,"-h") )
		{
			printHelp(stdout, argc, argv);
			return EXIT_SUCCESS;
		}
		else if( !strcmp(opt, "--dev") || !strcmp(opt,"-d") )
		{
			i++;
			if( i >= argc )			
				reportMissingArgument(opt);
			dev = argv[i];
		}
		else if( !strcmp(opt, "--list-axismap") )
		{
			argListAxismap = 1;
		}
		else if( !strcmp(opt, "--list-buttonmap") )
		{
			argListButtonmap = 1;
		}
		else if( !strcmp(opt, "--set-axismap") )
		{
			i++;
			if( i >= argc )			
				reportMissingArgument(opt);
			argSetAxismap = argv[i];
		}
		else if( !strcmp(opt, "--set-buttonmap") )
		{	
			i++;
			if( i >= argc )			
				reportMissingArgument(opt);
			argSetButtonmap = argv[i];
		}
		else
		{
			fprintf(stderr, "Invalid option '%s'.\n", opt);
			return EXIT_INVARG;
		}
	}


	if( !dev )
	{
		fprintf(stderr, "You need to specify a device.\n");
		return EXIT_INVARG;
	}


	int fd=-1;
	if( (fd = open(dev, O_RDONLY)) < 0 )
	{
		perror("Cannot open device.");
		return EXIT_OPEN;
	}


	int version;
	if( ioctl(fd, JSIOCGVERSION, &version) )
	{
		perror( PACKAGE_STRING ": error getting version");
		exit(EXIT_IOCTL);
	}

	if( version != JS_VERSION )
	{
		fprintf(stderr, PACKAGE_STRING ": compiled with different version %d\n", JS_VERSION);
		exit(EXIT_VERSION);
	}


	if( argListAxismap )
		listAxismap(fd);

	if( argListButtonmap )
		listButtonmap(fd);

	if( argSetAxismap )
		setAxismap(fd, argSetAxismap);

	if( argSetButtonmap )
		setButtonmap(fd, argSetButtonmap);


	close(fd);
	return 0;
}
