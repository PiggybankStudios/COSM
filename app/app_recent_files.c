/*
File:   app_recent_files.c
Author: Taylor Robbins
Date:   09\09\2025
Description: 
	** Holds the API that handles saving and loading the list of recently opened file paths
*/

void FreeRecentFile(RecentFile* recentFile)
{
	NotNull(recentFile);
	FreeStr8(stdHeap, &recentFile->path);
	ClearPointer(recentFile);
}

void AppClearRecentFiles()
{
	VarArrayLoop(&app->recentFiles, rIndex)
	{
		VarArrayLoopGet(RecentFile, recentFile, &app->recentFiles, rIndex);
		FreeRecentFile(recentFile);
	}
	VarArrayClear(&app->recentFiles);
}

void AppLoadRecentFilesList()
{
	ScratchBegin(scratch);
	FilePath settingsFolderPath = OsGetSettingsSavePath(scratch, Str8_Empty, StrLit(PROJECT_FOLDER_NAME_STR), false);
	FilePath savePath = JoinStringsInArena3(scratch,
		settingsFolderPath,
		DoesPathHaveTrailingSlash(settingsFolderPath) ? Str8_Empty : StrLit("/"),
		StrLit(RECENT_FILES_SAVE_FILEPATH),
		false
	);
	
	if (OsDoesFileExist(savePath))
	{
		Str8 fileContent = Str8_Empty;
		if (OsReadTextFile(savePath, scratch, &fileContent))
		{
			AppClearRecentFiles();
			LineParser parser = NewLineParser(fileContent);
			Str8 fileLine = ZEROED;
			while (LineParserGetLine(&parser, &fileLine))
			{
				if (!IsEmptyStr(fileLine))
				{
					RecentFile* newFile = VarArrayAdd(RecentFile, &app->recentFiles);
					NotNull(newFile);
					newFile->path = AllocStr8(stdHeap, fileLine);
					newFile->fileExists = OsDoesFileExist(newFile->path);
					// ClayId buttonClayId = ToClayIdPrint(scratch, "%.*s_Btn", StrPrint(newFile->path)); //TODO: This should go on uiArena
					// newFile->tooltipId = AddTooltipClay(&app->tooltipRegions, buttonClayId, newFile->path, DEFAULT_TOOLTIP_DELAY, 2)->id;
				}
			}
			
			PrintLine_D("Loaded %llu recent file%s from \"%.*s\"", app->recentFiles.length, Plural(app->recentFiles.length, "s"), StrPrint(savePath));
			u64 programTime = (appIn != nullptr) ? appIn->programTime : 0;
			if (app->recentFilesSaveWatch.arena == nullptr) { OsInitFileWatch(stdHeap, savePath, CHECK_RECENT_FILES_CHANGED_PERIOD, programTime, &app->recentFilesSaveWatch); }
			else { OsResetFileWatch(&app->recentFilesSaveWatch, programTime); }
		}
		else { PrintLine_W("No recent files save found at \"%.*s\"", StrPrint(savePath)); }
	}
	ScratchEnd(scratch);
}

void AppSaveRecentFilesList()
{
	ScratchBegin(scratch);
	
	FilePath settingsFolderPath = OsGetSettingsSavePath(scratch, Str8_Empty, StrLit(PROJECT_FOLDER_NAME_STR), true);
	Result createSettingsFolderResult = OsCreateFolder(settingsFolderPath, true);
	if (createSettingsFolderResult != Result_Success)
	{
		NotifyPrint_E("Failed to create settings folder at \"%.*s\": %s", StrPrint(settingsFolderPath), GetResultStr(createSettingsFolderResult));
		return;
	}
	FilePath savePath = JoinStringsInArena3(scratch,
		settingsFolderPath,
		DoesPathHaveTrailingSlash(settingsFolderPath) ? Str8_Empty : StrLit("/"),
		StrLit(RECENT_FILES_SAVE_FILEPATH),
		false
	);
	
	OsFile fileHandle = ZEROED;
	if (OsOpenFile(scratch, savePath, OsOpenFileMode_Write, false, &fileHandle))
	{
		VarArrayLoop(&app->recentFiles, rIndex)
		{
			VarArrayLoopGet(RecentFile, recentFile, &app->recentFiles, rIndex);
			bool writeSuccess = OsWriteToOpenTextFile(&fileHandle, recentFile->path);
			Assert(writeSuccess);
			writeSuccess = OsWriteToOpenTextFile(&fileHandle, StrLit("\n"));
			Assert(writeSuccess);
		}
		OsCloseFile(&fileHandle);
		
		u64 programTime = (appIn != nullptr) ? appIn->programTime : 0;
		if (app->recentFilesSaveWatch.arena == nullptr) { OsInitFileWatch(stdHeap, savePath, CHECK_RECENT_FILES_CHANGED_PERIOD, programTime, &app->recentFilesSaveWatch); }
		else { OsResetFileWatch(&app->recentFilesSaveWatch, programTime); }
	}
	else { NotifyPrint_E("Failed to save recent files list to \"%.*s\"", StrPrint(savePath)); }
	
	ScratchEnd(scratch);
}

void AppRememberRecentFile(FilePath filePath)
{
	ScratchBegin(scratch);
	Str8 fullPath = OsGetFullPath(scratch, filePath);
	bool alreadyExists = false;
	VarArrayLoop(&app->recentFiles, rIndex)
	{
		VarArrayLoopGet(RecentFile, recentFile, &app->recentFiles, rIndex);
		if (StrAnyCaseEquals(recentFile->path, fullPath))
		{
			alreadyExists = true;
			if (rIndex+1 < app->recentFiles.length)
			{
				//Move this path to the end of the array
				RecentFile temp;
				MyMemCopy(&temp, recentFile, sizeof(RecentFile));
				VarArrayRemove(RecentFile, &app->recentFiles, recentFile);
				RecentFile* newRecentFile = VarArrayAdd(RecentFile, &app->recentFiles);
				MyMemCopy(newRecentFile, &temp, sizeof(RecentFile));
				break;
			}
		}
	}
	
	if (!alreadyExists)
	{
		RecentFile* newRecentFile = VarArrayAdd(RecentFile, &app->recentFiles);
		NotNull(newRecentFile);
		ClearPointer(newRecentFile);
		newRecentFile->path = AllocStr8(stdHeap, fullPath);
		newRecentFile->fileExists = true;
		NotNull(newRecentFile->path.chars);
		// ClayId buttonClayId = ToClayIdPrint(scratch, "%.*s_Btn", StrPrint(newRecentFile->path)); //TODO: This should go on uiArena
		// newRecentFile->tooltipId = AddTooltipClay(&app->tooltipRegions, buttonClayId, newRecentFile->path, DEFAULT_TOOLTIP_DELAY, 2)->id;
		
		while (app->recentFiles.length > RECENT_FILES_MAX_LENGTH)
		{
			RecentFile* firstFile = VarArrayGetFirst(RecentFile, &app->recentFiles);
			FreeRecentFile(firstFile);
			VarArrayRemoveFirst(RecentFile, &app->recentFiles);
		}
	}
	
	AppSaveRecentFilesList();
	
	ScratchEnd(scratch);
}

Str8 GetUniqueRecentFilePath(FilePath filePath)
{
	if (IsEmptyStr(filePath)) { return filePath; }
	uxx numParts = CountPathParts(filePath, false);
	if (numParts == 1) { return filePath; }
	
	for (uxx pIndex = 0; pIndex < numParts; pIndex++)
	{
		// Str8 GetPathPart(FilePath path, ixx partIndex, bool includeEmptyBeginOrEnd)
		Str8 pathPart = GetPathPart(filePath, numParts-1 - pIndex, false);
		uxx partStartIndex = (uxx)(pathPart.chars - filePath.chars);
		Str8 subPath = StrSliceFrom(filePath, partStartIndex);
		
		bool foundConflict = false;
		VarArrayLoop(&app->recentFiles, rIndex)
		{
			VarArrayLoopGet(RecentFile, recentFile, &app->recentFiles, rIndex);
			if (!StrAnyCaseEquals(recentFile->path, filePath) &&
				StrAnyCaseEndsWith(recentFile->path, subPath))
			{
				foundConflict = true;
				break;
			}
		}
		if (!foundConflict) { return subPath; }
	}
	
	return filePath;
}
