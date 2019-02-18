#include <pspumd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "config.h"
#include "dirbrowse.h"
#include "fs.h"
#include "glib2d_helper.h"
#include "menu_error.h"
#include "menu_settings.h"
#include "menu_fileoptions.h"
#include "screenshot.h"
#include "status_bar.h"
#include "textures.h"
#include "utils.h"

#define MENUBAR_X_BOUNDARY  0

static char multi_select_dir_old[512];
static int menubar_selection = 0;
static float menubar_x = -180.0;
u64 total_storage = 0, used_storage = 0;

static void Menu_HandleMultiSelect(void) {
	// multi_select_dir can only hold one dir
	strcpy(multi_select_dir_old, cwd);
	if (strcmp(multi_select_dir_old, multi_select_dir) != 0)
		FileOptions_ResetClipboard();

	char path[512];
	File *file = Dirbrowse_GetFileIndex(position);
	strcpy(path, cwd);
	strcpy(path + strlen(path), file->name);
	strcpy(multi_select_dir, cwd);
			
	if (!multi_select[position]) {
		multi_select[position] = true;
		multi_select_indices[position] = multi_select_index; // Store the index in the position
		Utils_AppendArr(multi_select_paths[multi_select_index], path, multi_select_index);
		multi_select_index += 1;
	}
	else {
		multi_select[position] = false;
		strcpy(multi_select_paths[multi_select_indices[position]], "");
		multi_select_indices[position] = -1;
	}

	Utils_SetMax(&multi_select_index, 0, 50);
	Utils_SetMin(&multi_select_index, 50, 0);
}

static void Menu_ControlMenubar(void) {
	char *buf;

	if (Utils_IsButtonPressed(PSP_CTRL_UP))
		menubar_selection--;
	else if (Utils_IsButtonPressed(PSP_CTRL_DOWN))
		menubar_selection++;

	Utils_SetMax(&menubar_selection, 0, Utils_IsModelPSPGo()? 2 : 3);
	Utils_SetMin(&menubar_selection, Utils_IsModelPSPGo()? 2 : 3, 0);

	if (Utils_IsButtonPressed(PSP_CTRL_ENTER)) {
		switch (menubar_selection) {
			case 0:
				if (BROWSE_STATE == BROWSE_STATE_UMD)
					sceUmdDeactivate(1, "disc0:");
				if (BROWSE_STATE != BROWSE_STATE_SD) {
					buf = malloc(256);
					memset(root_path, 0, strlen(root_path));
					strcpy(root_path, Utils_IsEF0()? "ef0:/" : "ms0:/");

					if (FS_FileExists("lastdir.txt")) {
						if (R_FAILED(FS_ReadFile("lastdir.txt", buf, 256))) {
							free(buf);
							strcpy(cwd, Utils_IsEF0()? "ef0:/" : "ms0:/");
						}

						char tempPath[256];
						sscanf(buf, "%[^\n]s", tempPath);

						if (FS_DirExists(tempPath)) // Incase a directory previously visited had been deleted, set start path to sdmc:/ to avoid errors.
							strcpy(cwd, tempPath);
						else
							strcpy(cwd, Utils_IsEF0()? "ef0:/" : "ms0:/");

						free(buf);
					}

					BROWSE_STATE = BROWSE_STATE_SD;

					menubar_x -= 10.0;
					menubar_selection = 0;
					menubar_x = -180;

					Dirbrowse_PopulateFiles(true);
					MENU_STATE = MENU_STATE_HOME;
				}
				break;
			case 1:
				if (BROWSE_STATE == BROWSE_STATE_UMD)
					sceUmdDeactivate(1, "disc0:");
				if (BROWSE_STATE != BROWSE_STATE_FLASH0) {
					memset(root_path, 0, strlen(root_path));
					strcpy(root_path, "flash0:/");
					strcpy(cwd, "flash0:/");
					
					BROWSE_STATE = BROWSE_STATE_FLASH0;
					
					menubar_x -= 10.0;
					menubar_selection = 0;
					menubar_x = -180;

					Dirbrowse_PopulateFiles(true);
					MENU_STATE = MENU_STATE_HOME;
				}
				break;
			case 2:
				if (BROWSE_STATE == BROWSE_STATE_UMD)
					sceUmdDeactivate(1, "disc0:");
				if (BROWSE_STATE != BROWSE_STATE_FLASH1) {
					memset(root_path, 0, strlen(root_path));
					strcpy(root_path, "flash1:/");
					strcpy(cwd, "flash1:/");
					
					BROWSE_STATE = BROWSE_STATE_FLASH1;
					
					menubar_x -= 10.0;
					menubar_selection = 0;
					menubar_x = -180;

					Dirbrowse_PopulateFiles(true);
					MENU_STATE = MENU_STATE_HOME;
				}
				break;
			case 3:
				if (BROWSE_STATE != BROWSE_STATE_UMD) {
					if (sceUmdCheckMedium() != 0) {
						sceUmdActivate(1, "disc0:");
						sceUmdWaitDriveStat(UMD_WAITFORINIT);
						
						memset(root_path, 0, strlen(root_path));
						strcpy(root_path, "disc0:/");
						strcpy(cwd, "disc0:/");
						
						BROWSE_STATE = BROWSE_STATE_UMD;
						
						menubar_x -= 10.0;
						menubar_selection = 0;
						menubar_x = -180;

						Dirbrowse_PopulateFiles(true);
						MENU_STATE = MENU_STATE_HOME;
					}
					else
						Menu_DisplayError("Could not read UMD drive.", 0);
				}
				break;
		}
	}
	else if ((Utils_IsButtonPressed(PSP_CTRL_CANCEL)) || (Utils_IsButtonPressed(PSP_CTRL_SELECT))) {
		menubar_x -= 10.0;
		menubar_selection = 0;
		menubar_x = -180;
		MENU_STATE = MENU_STATE_HOME;
	}
}

static void Menu_ControlHome(void) {
	if (fileCount > 0) {
		if (Utils_IsButtonPressed(PSP_CTRL_UP))
			position--;
		else if (Utils_IsButtonPressed(PSP_CTRL_DOWN))
			position++;

		Utils_SetMax(&position, 0, fileCount - 1);
		Utils_SetMin(&position, fileCount - 1, 0);

		if (Utils_IsButtonPressed(PSP_CTRL_LEFT))
			position = 0;
		else if (Utils_IsButtonPressed(PSP_CTRL_RIGHT))
			position = fileCount - 1;

		if (Utils_IsButtonPressed(PSP_CTRL_SQUARE))
			Menu_HandleMultiSelect();

		if (Utils_IsButtonPressed(PSP_CTRL_ENTER))
			Dirbrowse_OpenFile();
		else if ((strcmp(cwd, root_path) != 0) && (Utils_IsButtonPressed(PSP_CTRL_CANCEL))) {
			Dirbrowse_Navigate(true);
			Dirbrowse_PopulateFiles(true);
		}
	}

	if (Utils_IsButtonPressed(PSP_CTRL_SELECT)) {
		menubar_selection = BROWSE_STATE;
		MENU_STATE = MENU_STATE_MENUBAR;
	}
	else if (Utils_IsButtonPressed(PSP_CTRL_START))
		MENU_STATE = MENU_STATE_SETTINGS;
	else if (Utils_IsButtonPressed(PSP_CTRL_TRIANGLE))
		MENU_STATE = MENU_STATE_FILEOPTIONS;
}

static void Menu_DisplayMenubar(void) {
	G2D_DrawImage(bg_header, menubar_x, 20);
	G2D_DrawRect(menubar_x, 90, 180, 252, config.dark_theme? BLACK_BG : WHITE);
	G2D_DrawRect(menubar_x + 180, 20, 1, 252, config.dark_theme? SELECTOR_COLOUR_DARK : G2D_RGBA(200, 200, 200, 255));
	G2D_DrawRect(menubar_x, 90 + (30 * menubar_selection), 180, 30, config.dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);

	intraFontSetStyle(font, 0.6f, config.dark_theme? WHITE : BLACK, G2D_RGBA(0, 0, 0, 0), 0.f, INTRAFONT_ALIGN_LEFT);
	G2D_DrawImage(config.dark_theme? icon_sd_dark : icon_sd, menubar_x + 10, 92);
	intraFontPrint(font, menubar_x + 50, 90 + ((30 - (font->glyph->height - 6)) / 2), Utils_IsEF0()? "ef0:/" : "ms0:/");

	G2D_DrawImage(config.dark_theme? icon_secure_dark : icon_secure, menubar_x + 10, 122);
	intraFontPrint(font, menubar_x + 50, 90 + ((30 - (font->glyph->height - 6)) / 2) + 30, "flash0:/");

	G2D_DrawImage(config.dark_theme? icon_secure_dark : icon_secure, menubar_x + 10, 152);
	intraFontPrint(font, menubar_x + 50, 90 + ((30 - (font->glyph->height - 6)) / 2) + 60, "flash1:/");

	if (!Utils_IsModelPSPGo()) {
		G2D_DrawImage(config.dark_theme? icon_secure_dark : icon_secure, menubar_x + 10, 182);
		intraFontPrint(font, menubar_x + 50, 90 + ((30 - (font->glyph->height - 6)) / 2) + 90, "disc0:/");
	}
}

void Menu_Main(void) {
	Dirbrowse_PopulateFiles(false);
	memset(multi_select, 0, sizeof(multi_select)); // Reset all multi selected items

	total_storage = Utils_GetTotalStorage();
	used_storage = Utils_GetUsedStorage();

	while (1) {
		g2dClear(config.dark_theme? BLACK_BG : WHITE);
		G2D_DrawRect(0, 0, 480, 20, config.dark_theme? STATUS_BAR_DARK : STATUS_BAR_LIGHT);
		G2D_DrawRect(0, 20, 480, 42, config.dark_theme? MENU_BAR_DARK : MENU_BAR_LIGHT);
		G2D_DrawImage(icon_nav_drawer, 5, 25);

		StatusBar_DisplayTime();
		Dirbrowse_DisplayFiles();

		Utils_ReadControls();

		if (MENU_STATE == MENU_STATE_HOME)
			Menu_ControlHome();
		else if (MENU_STATE == MENU_STATE_FILEOPTIONS) {
			Menu_DisplayFileOptions();
			Menu_ControlFileOptions();
		}
		else if (MENU_STATE == MENU_STATE_PROPERTIES) {
			Menu_DisplayFileProperties();
			Menu_ControlFileProperties();
		}
		else if (MENU_STATE == MENU_STATE_DELETE) {
			Menu_DisplayDeleteDialog();
			Menu_ControlDeleteDialog();
		}
		else if (MENU_STATE == MENU_STATE_MENUBAR) {
			menubar_x += 10.0;

			if (menubar_x > -1)
				menubar_x = MENUBAR_X_BOUNDARY;
			Menu_ControlMenubar();
			Menu_DisplayMenubar();
		}
		else if (MENU_STATE == MENU_STATE_SETTINGS)
			Menu_DisplaySettings();

		if (((Utils_IsButtonHeld(PSP_CTRL_LTRIGGER)) && (Utils_IsButtonPressed(PSP_CTRL_RTRIGGER))) || ((Utils_IsButtonHeld(PSP_CTRL_RTRIGGER)) && (Utils_IsButtonPressed(PSP_CTRL_LTRIGGER))))
			Screenshot_Capture();

		else if (((Utils_IsButtonHeld(PSP_CTRL_START)) && (Utils_IsButtonPressed(PSP_CTRL_SELECT))) || ((Utils_IsButtonHeld(PSP_CTRL_SELECT)) && (Utils_IsButtonPressed(PSP_CTRL_START))))
			longjmp(exitJmp, 1);

		Utils_HandleUSB();
		g2dFlip(G2D_VSYNC);
	}
}