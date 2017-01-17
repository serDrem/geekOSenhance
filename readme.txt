In order to be able just run our compiled project, skip to section 2.

Note: your openSUSE should be 32 bit.

Section 1:
Your system should have the following components installed to start developing GeekOS
0- update you openSUSE fully then install
1- gcc
2- make
3- nasm

Section 2:
To install bochs emulator, search in google "opensuse bochs" then downloaded the file called
  "bochs-2.2.1-263.1.i586.rpm" from 
  software.opensuse.org/pabage/bochs?search_term=bochs
  Chose opneSUSE 11.4 which is marked as official release, pick 32 Bit
  then 
  - By double clicking on the downloaded file, you will be installing it.

To run our compiled project:
from the command line, navigate to the directory called /build where the files 
fd.img & diskc.img
located and type the following command
> bochs
then the bochs will start, don't worry about the menue, hit enter to start the actual emulator.
NOTE: in the same direcoty as the files there should be a file named
.bochsrc
     this file contains the information that lets bochs load GeekOS
DONE.
Section 3:
in order to build our project, navigate to the directory called
/project/build/
then type the following commands one after the other
> make clean
> make depend
> make
and after it finish compiling you can run it using instruction in section 2

