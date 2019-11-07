SET SGDK=G:\SGDK-Master

%SGDK%\bin\gcc -m68000 -Wall -fno-builtin -I %SGDK%\inc -B%SGDK%\bin -c rom_head.c -o rom_head.o
%SGDK%\bin\ld rom_head.o -T %SGDK%\md.ld -nostdlib --oformat binary -o rom_head.bin
%SGDK%\bin\gcc -m68000 -Wall -fno-builtin -I %SGDK%\inc -B%SGDK%\bin -c sega.s -o sega.o
%SGDK%\bin\gcc main.c -m68000 -Wall -O2 -fno-builtin -B %SGDK%\bin -n -T %SGDK%\md.ld -nostdlib sega.o -I%SGDK%\inc -I%SGDK%\res %SGDK%\lib\libmd.a %SGDK%\lib\libgcc.a -o main.out
%SGDK%\bin\objcopy -O binary main.out main.bin
%SGDK%\bin\sizebnd main.bin -sizealign 131072
copy /Y main.bin menu.msd

