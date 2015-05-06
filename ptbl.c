/*

ProDOS/HFS Partition Table Generator
by Scott Alfter (https://alfter.us/)
5 February 2003

Copyright (C) 2003-2015 by Scott Alfter

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.



Version History
===============
 4 Feb 03  original release
 5 Feb 03  added HFS support and fixed a bug that surfaced when I tried
           creating partitions of unequal sizes
 6 May 15  license change


I threw this together as a quick hack to put my System 6.0.1 floppies onto a
CD-ROM, since I couldn't figure out how to get mkisofs to create an HFS
CD-ROM with multiple partitons.  This program creates an Apple-format
partition table with the specified number of ProDOS and/or HFS partitions. 
Each partition has its own volume name and size in blocks.  If the first
partition is a ProDOS partition and has a file named PRODOS, the CD will be
bootable on a suitably-equipped Apple II.

Floppy images can be created with GSHK or ShrinkIt and expanded into files
with NuLib.  To create images of hard-disk partitions, make sure your Linux
kernel includes CONFIG_MAC_PARTITION and whatever support is needed to
access your hard drive (probably CONFIG_SCSI, CONFIG_BLK_DEV_SD, and the
appropriate SCSI low-level driver), move the drive from your II to your
Linux box, and dd the appropriate /dev/sdwhatever into a file.

Once you've created the partition table, you just cat the generated
partition table and the filesystem images together in the order they were
specified when the table was generated.


Usage: ptbl partcount part1name part1size part1type ...
       partcount=number (n) of partitions to create
       part1name=volume label of 1st partition
       part1size=size of 1st partition, in 512-byte blocks
       part1type="Apple_PRODOS" or "Apple_HFS"
       The partition table is written to stdout, so make sure you catch it.

Example: you have two ShrinkIt 800K floppy archives, disk1.sdk and
disk2.sdk, that you want to put on a CD.  Do the following to extract the
images:

nulib x disk1.sdk; nulib x disk2.sdk

The extracted image files will take the volume name of the original
disks...we'll say they're DISK1 and DISK2.  Generate the CD image with this
incantation:

(ptbl 2 DISK1 1600 Apple_PRODOS DISK2 1600 Apple_PRODOS; cat DISK1 DISK2) >a2cd.iso

You should be able to burn the resulting image with any CD-burning app...I
tested it with Nero on Win2K, and it worked fine.


I wrote and tested this program under Linux.  I also tested it under Cygwin
and got the same results.  It should compile under any other POSIXish
platform (*BSD, MacOS X, etc.) without much fuss.  For non-POSIX
environments (Win32, MacOS pre-X, etc.), YMMV.  It might even compile on an
Apple IIGS with ORCA/C, but this is definitely untested.  (It might choke on
the ANSI-style function prototypes and C++-style comments.)

To build, you should be able to just type in "make" if that's available on
your system.  Otherwise, just compile this straight into an executable.


Documentation on the partition table format was found at this URL:
http://developer.apple.com/techpubs/mac/Devices/Devices-121.html#MARKER-9-35

This is a partial quote from that page...don't know why the crack-addicts at
Apple use Pascal instead of C to describe their data structures.  The two
constants in the #defines further down were filched from some other pages on
Apple's website that were a short distance from the URL given above.

The ddPad field also appears to be the wrong size, since it results in a
total size for the struct of 516 bytes.  I've corrected for this.

More detailed information on the partition-table struct is at this URL:
http://developer.apple.com/techpubs/mac/Devices/Devices-126.html#MARKER-9-1

This includes a list of valid partition-table types...we're only interested
in Apple_PRODOS for this application.

TYPE Block0 = 
PACKED RECORD
   sbSig:         Integer;    {device signature}
   sbBlkSize:     Integer;    {block size of the device}
   sbBlkCount:    LongInt;    {number of blocks on the device}
   sbDevType:     Integer;    {reserved}
   sbDevId:       Integer;    {reserved}
   sbData:        LongInt;    {reserved}
   sbDrvrCount:   Integer;    {number of driver descriptor entries}
   ddBlock:       LongInt;    {first driver's starting block}
   ddSize:        Integer;    {size of the driver, in 512-byte blocks}
   ddType:        Integer;    {operating system type (MacOS = 1)}
   ddPad:         ARRAY [0..242] OF Integer; {additional drivers, if any}
END;

TYPE Partition = 
RECORD
   pmSig:         Integer;       {partition signature}
   pmSigPad:      Integer;       {reserved}
   pmMapBlkCnt:   LongInt;       {number of blocks in partition map}
   pmPyPartStart: LongInt;       {first physical block of partition}
   pmPartBlkCnt:  LongInt;       {number of blocks in partition}
   pmPartName:    PACKED ARRAY [0..31] OF Char; {partition name}
   pmParType:     PACKED ARRAY [0..31] OF Char; {partition type}
   pmLgDataStart: LongInt;       {first logical block of data area}
   pmDataCnt:     LongInt;       {number of blocks in data area}
   pmPartStatus:  LongInt;       {partition status information}
   pmLgBootStart: LongInt;       {first logical block of boot code}
   pmBootSize:    LongInt;       {size of boot code, in bytes}
   pmBootAddr:    LongInt;       {boot code load address}
   pmBootAddr2:   LongInt;       {reserved}
   pmBootEntry:   LongInt;       {boot code entry point}
   pmBootEntry2:  LongInt;       {reserved}
   pmBootCksum:   LongInt;       {boot code checksum}
   pmProcessor:   PACKED ARRAY [0..15] OF Char; {processor type}
   pmPad:         ARRAY [0..187] OF Integer;    {reserved}
END;

*/

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>

typedef struct // driver descriptor record
{
	short sbSig;
	short sbBlkSize;
	long sbBlkCount;
	short sbDevType;
	short sbDevId;
	long sbData;
	short sbDrvrCount;
	long ddBlock;
	short ddSize;
	short ddType;
	short ddPad[241];
}
ddrec_t;

typedef struct // partition map entry record
{
	short pmSig;
	short pmSigPad;
	long pmMapBlkCnt;
	long pmPyPartStart;
	long pmPartBlkCnt;
	char pmPartName[32];
	char pmParType[32];
	long pmLgDataStart;
	long pmDataCnt;
	long pmPartStatus;
	long pmLgBootStart;
	long pmBootSize;
	long pmBootAddr;
	long pmBootAddr2;
	long pmBootEntry;
	long pmBootEntry2;
	long pmBootCksum;
	char pmProcessor[16];
	short pmPad[188];
}
ptrec_t;

#define sbSIGWord 17746 // device signature
#define pMapSIG 20557 // partition signature

// dump a block of data to stdout

void writeblock (void* ptr, int size)
{
	int i;
	char* block=(char*)ptr;
	
	for (i=0; i<size; i++)
		putchar(block[i]);
}	

// the main event

int main (int argc, char** argv)
{
	ddrec_t ddrec;
	ptrec_t ptrec;
	int partcount, partsize, disksize, partstart;
	int i;
	
	if (argc<4) // only a very basic sanity check
	{
		fprintf(stderr,"Usage: %s partcount part1name part1size part1type ...\n",argv[0]);
		exit(-1);
	}
	sscanf(argv[1],"%i",&partcount); // get # of partitions
	disksize=1+partcount;
	for (i=0; i<partcount; i++) // figure disk size
	{
		sscanf(argv[3+3*i],"%i",&partsize);
		disksize+=partsize;
	} 
	
	memset(&ddrec,0,sizeof(ddrec)); // set up driver descriptor (block 0)
	ddrec.sbSig=htons(sbSIGWord);
	ddrec.sbBlkSize=htons(512);
	ddrec.sbBlkCount=htonl(disksize);
	ddrec.sbDrvrCount=htons(0);
	ddrec.ddBlock=htonl(0);
	ddrec.ddSize=htons(0);
	ddrec.ddType=htons(1);
	
	memset(&ptrec,0,sizeof(ptrec)); // set up partition map entry
	ptrec.pmSig=htons(pMapSIG);
	ptrec.pmMapBlkCnt=htonl(partcount);
	ptrec.pmPartBlkCnt=htonl(partsize);
	ptrec.pmLgBootStart=htonl(0);
	ptrec.pmBootSize=htonl(1024);
	strcpy(ptrec.pmProcessor,"6502");
	
	writeblock(&ddrec,sizeof(ddrec)); // dump driver descriptor
	partstart=1+partcount;
	for (i=0; i<partcount; i++) // write out the specified number of partition map entries
	{
		ptrec.pmPyPartStart=htonl(partstart);
		strcpy(ptrec.pmPartName,argv[2+3*i]);
		strcpy(ptrec.pmParType,argv[4+3*i]);
		sscanf(argv[3+3*i],"%i",&partsize);
		ptrec.pmPartBlkCnt=htonl(partsize);
		writeblock(&ptrec,sizeof(ptrec));
		partstart+=partsize;
	}
	return 0;
}
