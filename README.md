# SampleMegaSDMenu
Sample replacement menu for MegaSD. Use it as a base to build your own game selection menus.

This is a very simple menu for the MegaSD to showcase the ability to run custom menus and how to communicate with the MCU that handles the SD card communications and game loading.


## Using replacement menus

In order to make MegaSD load the replacement menu, the built file must be copied to the root of the SD card and named **menu.msd**. 

MegaSD will always boot its own menu first, show the splash and verify if there is a firmware update. The firmware update process is handled by the internal menu. 
After the splash, and if no firmware update was present, MegaSD will check if the *menu.msd* file is present on the sd card root, and if it is, it will be loaded and booted from the reset vector again.

Holding *START* while booting past the splash menu will skip the custom menu loading, and will boot to the default MegaSD menu.

## Development info

To build this sample menu you need SGDK downloaded, and its path set in mk.bat. Then just run mk.bat to have a menu.msd file that you can copy to the sd card root to use as replacement.

Menu.msd file must be 128KB (131072), otherwise MegaSD may refuse to load it.

It's highly recommended to reinitialize the VDP in your own code.

Check *comms.h* for the available commands. Check the code for parameters, there is no documentation written yet.

As you can see in the *comms.h* file, the game list is written by the MCU to address 0x20000, that means the menu rom can only be 128KB in size. This is so it can be used from the expansion port. The game list is 64KB in size, so it ends at 0x2FFFF.

There is a PCM chip mapped at address 0x30000. This chip is the same that is used in the MegaCD, so refere to MegaCD documentation to know how to use it. It's 8 bits only, and it's mapped to the odd bytes of that address space.

Screenshots are stored at address 0x200000 (you have a define in *comms.h*). The structure is the one specified in the [GameDBManagerMD](https://github.com/Terraonion-dev/GameDBManagerMD#screenshots-section-2048--num-screenshots-variable-size-max-1024-screenshots). The screenshot index is returned in the game list structure.

This menu sample uses SGDK for simplicity, but it's probably not useful if you want to add lots of graphics to your menu due to the 128KB limit. SGDK has a large rom footprint. This sample is slightly over 64KB and doesn't have any graphics (for comparison, the internal MegaSD menu, that uses its own libraries, is 80KB with all its graphics).
