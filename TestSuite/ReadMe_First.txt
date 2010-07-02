
General Information:

    This test suite is desingned to be used with volumes of 1GB in size,
    which allows for a really short processing time.

    It is used on a virtual machine to test the algorithm.

    The following volumes are used:

        1) O: ... FAT
        2) P: ... FAT32
        3) Q: ... NTFS
        4) R: ... NTFS with compression enabled
        5) S: ... NTFS with 25% of compressed files, which mimics a regular system disk
        6) T: ... UDF

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
    
    2) asign the volume letters and file system types as outlined above
    
        a) You may change the drive letters in the batch scripts to match your test drives too.
           This is done by changing the definition of the "ProcessVolumes" variable,
           which takes a list of drives separated by spaces (Example: G: H: I:)

        b) In the NTFS batch script you can additionally specify the drive for the mixed
           files, regular and compressed.
           This is done by changing the definition of the "MixedVolume" variable,
           which contains a drive letter (Example: I:).
           If you do not like to create a mixed drive change the line to read:
                set MixedVolume=

    3) CHKDSK is executed before and after the fragmented files creation process,
       to make sure the volume is consistent.

    4) The volume is formated to the correct file system to always start with a fresh volume,
       so do not use volumes with existing data, since the data will be lost.
       In addition do only use volumes of 1GB in size, since quick format is not used, which
       would only clear the file allocation table.