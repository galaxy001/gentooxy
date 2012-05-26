Mac OS X Unlocker for VMware
============================

1. Introduction
---------------

The package is a combination of the Unlocker code written by Zenith432 plus some
scripts written by myself that wrap the actual unlocker code. 

It has been tested against:

* Workstation 8.0 on Windows and Linux (32 & 64-bit versions)
* Player 4.0 on Windows and Linux (32 & 64-bit versions)
* Fusion 4.0 on Snow Leopard and Lion
* ESXi 5.0

The patch code carries out the following modification dependent on the product
being patched:

* Fix vmware-vmx and derivatives to allow Mac OS X to boot
* Fix vmwarebase .dll or .so to allow Apple to be selected during creation
* Copy darwin.iso if needed to VMware folder

Note that not all products recognise the darwin.iso via install tools menu item.
You will have to manually mount the darwin.iso for example on Workstation.

Also Player is missing vmware-vmx-debug and vmware-vmx-stats files and so an 
error is shown during patching as the files are not found. This can be safely 
ignored.

In all cases make sure VMware is not running, and any background guests have 
been shutdown.

2. Windows
----------
On Windows you will need to either run cmd.exe as Administrator or using 
Explorer right click on the command file and select "Run as administrator".

install.cmd   - patches VMware and copies darwin.iso tools image to VMware
uninstall.cmd - restores VMware and removes darwin.iso tools image from VMware

3. Linux
---------
On Linux you will need to be either root or use sudo to run the scripts. 

You may need to ensure the contents of the linux folder have execute permissions
by running chmod +x against the 3 files.

install.sh   - patches VMware and copies darwin.iso tools image to VMware
uninstall.sh - restores VMware and removes darwin.iso tools image from VMware

4. Mac OS X
-----------
On Mac OS X you will need to be either root or use sudo to run the scripts. 
This is really only needed if you want to use client versions of Mac OS X.

You may need to ensure the contents of the osx folder have execute permissions
by running chmod +x against the 3 files.

install.sh   - patches VMware 
uninstall.sh - restores VMware 

5. ESXi
-------
ESXi has to be patched using the scripts, as the unlocker is used to overlay 
the ESXi firmware at runtime. You will need to transfer the files to the 
ESXi host either using vSphere client or SCP.

Once uploaded you will need to either use the ESXi support console or use SSH to
run the commands. Please note that you will need to reboot the host for the 
patches to become active.

You may need to ensure the contents of the esxi folder have execute permissions
by running chmod +x against the 3 files.

install.sh   - patches VMware 
uninstall.sh - restores VMware 

6. Zenith432's Unlocker
-----------------------

In all cases the unlocker can be run without the scripts but you would need to
carry out additional actions which the scripts encapsulate for you especially on
ESXi. If you want to run the unlocker directly the parameters are:

Usage: ./Unlocker.Linux64 [-h] [-u] [target_directory]
  -h: print help
  -u: remove the patch
  target_directory: customize location of vmx executables

On all platforms you must run it with administrator or root privileges.
The source code is provided and Zenith432 makes it freely available for
modification.


Thanks to Zenith432 for building the unlocker and Mac Son of Knife for all
the testing and support.


History
-------
11/10/11 1.0.0 - First release 
07/11/11 1.0.1 - Fixed typo in Windows command files
07/12/11 1.0.2 - Updated patcher and tools for latest release WKS 8.0.1 & FUS 4.1.1

(c) 2011 Dave Parsons
