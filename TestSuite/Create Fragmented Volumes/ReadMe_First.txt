
General Information:

    This test suite is designed to be used with volumes of 1GB in size,
    which allows for a really short processing time.

    It is used on a virtual machine to test the algorithm.

    The following volumes are used:

        1) L: ... FAT
        2) M: ... FAT32
        3) N: ... exFAT
        4) O: ... NTFS
        5) P: ... NTFS with compression enabled
        6) Q: ... NTFS with 25% of compressed files, which mimics a regular system disk
        7) R: ... UDF v1.02
        8) S: ... UDF v1.50
        9) T: ... UDF v2.00
       10) U: ... UDF v2.01
       11) V: ... UDF v2.50
       12) W: ... UDF v2.50 with duplicated meta-data

    The fragmentation utility from http://www.mydefrag.com/ is used to create
    fragmented files.

---

                    !!! CAUTION !!!

    The creation process will first format the volume,
    so make sure to not use a volume, where you have valuable data stored.

    It is best to use virtual test volumes without any existing data.

---

Setup:

    1) Prepare separate volumes, 1GB in size with the disk management utility
    
    2) assign the volume letters and file system types as outlined above
    
        a) You may change the drive letters in the batch scripts to match your test drives too.
           This is done by changing the definition of the "ProcessVolumes" variable,
           which takes a list of drives separated by spaces (Example: G: H: I:)

        b) In the NTFS batch script you can additionally specify the drive for the mixed
           files (regular and compressed).
           This is done by changing the definition of the "MixedVolume" variable,
           which contains a drive letter (Example: I:).
           If you do not like to create a mixed drive change the line to read:
           set MixedVolume=
           
           It is also possible to create a fully compressed NTFS volume, see "set CompressedVolume="

        c) UDF volumes are only supported on Vista and higher
        
        d) exFAT is only included with Vista and above.
           For XP you need to download and install the driver from http://support.microsoft.com/kb/955704/en-us

    3) CHKDSK is executed before and after the fragmented files creation process,
       to make sure the volume is consistent.

    4) The volume is formated to the correct file system to always start with a fresh volume,
       so do not use volumes with existing data, since the data will be lost.
       In addition do only use volumes of 1GB in size, since quick format is not used, which
       would only clear the file allocation table and would not detect bad clusters.