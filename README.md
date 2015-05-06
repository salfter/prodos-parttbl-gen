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

Read ptbl.c for documentation (build instructions and usage).
