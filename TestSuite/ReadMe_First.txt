
General Information:

    This test suite is desingned to be used with volumes of 1GB in size,
    which allows for a really short processing time.

    It is used on a virtual machine to test the algorithm.

    The following volumes are used:

        1) FAT
        2) FAT32
        3) NTFS
        4) NTFS with compression enabled
        5) NTFS with 25% of compressed files, which mimics a regular system disk

    The fragmentation utility from http://www.mydefrag.com/ is used to create
    fragmented files.

---

                    !!! CAUTION !!!

    The creation process will first delete all regular files contained in the root volume,
    so make sure to not use a volume, where you have valuable data stored.

    It is best to use virtual test volumes without any existing data.

---

Setup:

    1) Change the drive letters in the batch scripts to match your test drives.
       This is done by changing the definition of the "ProcessVolumes" variable,
       which takes a list of drives separated by spaces (Example: G: H: I:)

    2) In the NTFS batch script you can additionally specify the drive for the mixed
       files, regular and compressed.
       This is done by changing the definition of the "MixedVolume" variable,
       which contains a drive letter (Example: I:).
       If you do not like to create a mixed drive change the line to read:
            set MixedVolume=

    3) CHKDSK is running before and after the fragmented files creation process,
       to make sure the volume is consistent.
