#include <genesis.h>
#include "comms.h"

//Define TEST to have a static game list, so you can try on emulators
//#define TEST

const char *Types[] =
{
	" MD  ",	//0
	" CD  ",	//1
	" SMS ",	//2
	" 32X ",	//3
	" MD+ ",	//4
	" UNK ",	//5
	" UNK ",	//6
	"<DIR>",	//7
};


struct _GameListEntry	//64 bytes entry
{
	const char FileName[58];
	u16 screenshotID;	//screenshot number. FFFF or FFFE = no screenshot (actually anything > 0x8000 is no screenshot).
	u16 directoryID;	//this is the ID passed to the MCU to load game/change dir
	u8 NameLength;	//File name length
	u8 Flags;	// See Types array
};

#ifdef  TEST

const struct _GameListEntry gamelist[] =
{
	{ "Game1",0,0,5,0 },
	{ "Game2",0,0,5,0 },
	{ "Game3",0,0,5,0 },
	{ "Game4",0,0,5,0 },
	{ "Game5",0,0,5,0 },
	{ "Game6",0,0,5,0 },
	{ "Game7",0,0,5,0 },
	{ "Game8",0,0,5,0 },
	{ "Game9",0,0,5,0 },
	//0        1         2         3         4
	//123456789012345678901234567890123456789012345678
   { "Game10xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx1",0,0,48,0 },
   { "Game11",0,0,5,1 },
   { "Game12",0,0,5,2 },
   { "Game13",0,0,5,3 },
   { "Game14",0,0,5,4 },
   { "Game15",0,0,5,5 },
   { "Game16",0,0,5,6 },
   { "Game17",0,0,5,7 },
   { "Game18",0,0,5,0 },
   { "Game19",0,0,5,0 },

   { "Game20xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx1",0,0,48,0 },
   { "Game21xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx1",0,0,48,0 },
   { "Game22",0,0,5,0 },
   { "Game23",0,0,5,0 },
   { "Game24",0,0,5,0 },
   { "Game25",0,0,5,0 },
   { "Game26",0,0,5,0 },
   { "Game27",0,0,5,0 },
   { "Game28",0,0,5,0 },
   { "Game29",0,0,5,0 },
};

#define NUMGAMES	29
#else

const struct _GameListEntry *gamelist = (struct _GameListEntry *)(GAMESTARTADDRESS + 2);
#define NUMGAMES	HWREG(GAMESTARTADDRESS)

#endif

void SendCommandInRam(u8 command, u8 param)
{
	SEND_COMMAND(command, param);

	for (int i = 0; i < 1000; ++i)
	{
		asm("nop");
		asm("nop");
		asm("nop");
	}


	WAIT_RAM_COMMAND_END();
}

void CopyFunction(void *dst, void *func)
{
	volatile u16* ff = (volatile u16*)func;
	volatile u16 *dd = (volatile u16*)dst;

	while ((*ff) != 0x4e75)
	{
		*dd++ = *ff++;
	}

	*dd = 0x4e75;
}


void SEND_COMMAND_IN_RAM(u8 command, u8 param)
{
	u8 buf[520];

	CopyFunction(buf, SendCommandInRam);



	SYS_disableInts();
	((void(*)(u8, u8))buf)(command, param);
	SYS_enableInts();
}

u16 framecount;
u16 repeat(u16 trig, u16 buttons)
{
	if (!buttons)
		return trig;

	if (trig)
	{
		framecount = 20;
	}
	else
	{
		--framecount;
		if (!framecount)
		{
			trig = buttons;
			framecount = 2;
		}
	}

	return trig;
}

void LoadGame(u16 selection)
{
	const u16 LoadingLinePos = 10;
	const struct _GameListEntry *gd = gamelist + selection;

	char *filename = (char*)gd->FileName;

	VDP_fillTileMapRect(PLAN_A, 0x0020, 0, 0, 40, 28);

	if (gd->directoryID == 0xFEFF)	//load last game
	{
		HWREG(COMM_RAM) = selection;
		SEND_COMMAND(MCU_CONTROL_COMMAND_GETLASTNAME, 0);
		WAIT_COMMAND_END();


		filename = (char*)COMM_RAM;
	}
	else
	{
		//just use COMM_RAM as temporary
		filename = (char*)COMM_RAM;
		strcpy(filename, (char*)gd->FileName);
	}


	int l = strlen(filename);
	int x = 0;

	if (l > 38)
	{
		x = 1;
		//find a space, comma or period to break the line
		char *p = filename + 38;
		while (p != filename)
		{
			--p;
			if (*p == ' ' || *p == ',' || *p == '.')
				break;
		}
		if (p == filename)	//no break, do hard wrap
		{
			char a = filename[38];
			filename[38] = 0;
			VDP_drawText(filename, x, LoadingLinePos + 2);

			filename[38] = a;
			l = strlen(filename + 38);
			x = (38 - l) / 2;
			VDP_drawText(filename + 38, x, LoadingLinePos + 3);
		}
		else
		{
			*p = 0;
			l = strlen(filename);
			x = (38 - l) / 2;
			VDP_drawText(filename, x, LoadingLinePos + 2);
			++p;

			l = strlen(p);
			x = (38 - l) / 2;
			VDP_drawText(p, x, LoadingLinePos + 3);
		}
	}
	else
	{
		x = (38 - l) / 2;
		VDP_drawText(filename, x, LoadingLinePos + 2);
	}

	//reset the input ports state
	HWREGB(0xA1000B) = 0;
	HWREGB(0xA10009) = 0;
	HWREGB(0xA1000D) = 0;

	HWREG(COMM_RAM) = selection;
	SEND_COMMAND_IN_RAM(MCU_CONTROL_COMMAND_LOADGAME, 0);

}



#define VISIBLE_LINES	24

int main()
{
	u16 prev;
	u8 refresh = 1;
	u8 bigrefresh = 1;
	s16 top = 0;
	u16 selection = 0;

	VDP_drawText("MEGASD MENU", 16,0);

	VDP_setPalette(PAL0, font_pal_lib.data);
	VDP_setPaletteColor((PAL0 * 16) + 15, 0x0888);	//grey
	VDP_setPaletteColor((PAL1 * 16) + 15, 0x00FF);	//yellow
	VDP_setTextPalette(PAL0);
	
	VDP_drawText("READING SD CARD...", 10, 12);

	OPEN_COMMANDS();

#ifndef TEST
	//Load main Directory
	SEND_COMMAND_IN_RAM(MCU_CONTROL_COMMAND_LOADDIR, 0);
#endif


	//Clear message
	VDP_drawText("                  ", 10, 12);

	prev = JOY_readJoypad(JOY_1);

	while(1)
	{
		u16 current;
		u16 trig;

		int i;

		current = JOY_readJoypad(JOY_1);

		trig = (current ^ prev) & current;

		trig = repeat(trig, (current & (BUTTON_DOWN | BUTTON_UP | BUTTON_LEFT | BUTTON_RIGHT)));

		if (trig & BUTTON_DOWN)
		{
			if (selection - top < VISIBLE_LINES - 1 && selection < NUMGAMES - 1)
			{
				++selection;
				refresh = 1;
			}
		}

		if (trig & BUTTON_UP)
		{
			if (selection - top > 0)
			{
				--selection;
				refresh = 1;
			}
		}

		if (trig & BUTTON_RIGHT)
		{
			if (top + VISIBLE_LINES < NUMGAMES)
			{
				top += VISIBLE_LINES;
				selection = top;
				refresh = 1;
				bigrefresh = 1;
			}
		}

		if (trig & BUTTON_LEFT)
		{
			if (top > 0)
			{
				top -= VISIBLE_LINES;
				if (top < 0)
					top = 0;
				selection = top;
				refresh = 1;
				bigrefresh = 1;
			}
		}

		if (trig & BUTTON_A)
		{
			const struct _GameListEntry *gd = gamelist + selection;

			if (gd->directoryID == 0xFFFD)	//this is a separator
			{
			}
			else if ((gd->Flags & 0x7) == 7)	//change directory
			{
				VDP_fillTileMapRect(PLAN_A, 0x0020, 0, 0, 40, 28);

				HWREG(COMM_RAM) = selection;
				SEND_COMMAND(MCU_CONTROL_COMMAND_CHANGEDIR, 0);
				WAIT_COMMAND_END();

				VDP_drawText("READING SD CARD...", 10, 12);

				SEND_COMMAND_IN_RAM(MCU_CONTROL_COMMAND_LOADDIR, 0);

				refresh = 1;
				bigrefresh = 1;
				top = 0;
				selection = 0;
			}
			else
			{
				LoadGame(selection);
			}
		}

		if (refresh)
		{
			if (bigrefresh)
			{
				VDP_fillTileMapRect(PLAN_A, 0x0020, 0, 0, 40, 28);

				VDP_drawText("MEGASD MENU", 16, 0);
			}

			for (int i = 0; i < VISIBLE_LINES; ++i)
			{
				if(selection == top + i)
					VDP_setTextPalette(PAL1);
				else
					VDP_setTextPalette(PAL0);
				if (top + i >= NUMGAMES)
					continue;
				VDP_drawText(Types[gamelist[top + i].Flags & 7], 0, 1 + i);
				VDP_drawText(gamelist[top + i].FileName, 5, 1 + i);
			}

			refresh = 0;
			bigrefresh = 0;
		}

		prev = current;

		VDP_waitVSync();
	}
}