// Main.cpp

#include "StdAfx.h"

#include <initguid.h>
#include <io.h>

#include "Common/CommandLineParser.h"
#include "Common/StdOutStream.h"
#include "Common/Wildcard.h"
#include "Common/ListFileUtils.h"
#include "Common/StringConvert.h"
#include "Common/StdInStream.h"
#include "Common/StringToInt.h"
#include "Common/Exception.h"

#include "Windows/FileDir.h"
#include "Windows/FileName.h"
#include "Windows/Defs.h"
#include "Windows/Error.h"

#include "../../IPassword.h"
#include "../../ICoder.h"
#include "../../Compress/LZ/IMatchFinder.h"
#include "../Common/ArchiverInfo.h"
#include "../Common/UpdateAction.h"
#include "../Common/Update.h"
#include "../Common/Extract.h"
#include "../Common/ArchiveCommandLine.h"
#include "../Common/ExitCode.h"

#include "List.h"
#include "OpenCallbackConsole.h"
#include "ExtractCallbackConsole.h"
#include "UpdateCallbackConsole.h"

#ifndef EXCLUDE_COM
#include "Windows/DLL.h"
#endif

using namespace NWindows;
using namespace NFile;
using namespace NCommandLineParser;

HINSTANCE g_hInstance = 0;

static const char *kCopyrightString = "\n7-Zip"
#ifdef EXCLUDE_COM
" (A)"
#endif

#ifdef UNICODE
" [NT]"
#endif

" 4.09 beta  Copyright (c) 1999-2004 Igor Pavlov  2004-10-05\n";

static const char *kHelpString = 
    "\nUsage: 7z <command> [<switches>...] <archive_name> [<file_names>...]\n"
    "       [<@listfiles...>]\n"
    "\n"
    "<Commands>\n"
    "  a: Add files to archive\n"
    "  d: Delete files from archive\n"
    "  e: Extract files from archive\n"
    "  l: List contents of archive\n"
//    "  l[a|t][f]: List contents of archive\n"
//    "    a - with Additional fields\n"
//    "    t - with all fields\n"
//    "    f - with Full pathnames\n"
    "  t: Test integrity of archive\n"
    "  u: Update files to archive\n"
    "  x: eXtract files with full pathname\n"
    "<Switches>\n"
    "  -bd Disable percentage indicator\n"
    "  -i[r[-|0]]{@listfile|!wildcard}: Include filenames\n"
    "  -m{Parameters}: set compression Method\n"
    "  -o{Directory}: set Output directory\n"
    "  -p{Password}: set Password\n"
    "  -r[-|0]: Recurse subdirectories\n"
    "  -sfx[{name}]: Create SFX archive\n"
    "  -t{Type}: Set type of archive\n"
    "  -u[-][p#][q#][r#][x#][y#][z#][!newArchiveName]: Update options\n"
    "  -w[{path}]: assign Work directory. Empty path means a temporary directory\n"
    "  -x[r[-|0]]]{@listfile|!wildcard}: eXclude filenames\n"
    "  -y: assume Yes on all queries\n"
    "  -si: read data from stdin\n"
    "  -so: write data to stdout\n";

// ---------------------------
// exception messages

static const char *kProcessArchiveMessage = " archive: ";
static const char *kEverythingIsOk = "Everything is Ok";
static const char *kUserErrorMessage  = "Incorrect command line"; // NExitCode::kUserError

static const wchar_t *kDefaultSfxModule = L"7zCon.sfx";

void PrintHelp(void)
{
  g_StdErr << kHelpString;
}

static void ShowMessageAndThrowException(LPCSTR message, NExitCode::EEnum code)
{
  g_StdErr << message << endl;
  throw code;
}

static void PrintHelpAndExit() // yyy
{
  PrintHelp();
  ShowMessageAndThrowException(kUserErrorMessage, NExitCode::kUserError);
}

static void PrintProcessTitle(const AString &processTitle, const UString &archiveName)
{
  g_StdErr << endl << processTitle << 
      kProcessArchiveMessage << archiveName << endl << endl;
}

// int Main2(int numArguments, const char *arguments[])
int Main2()
{
  SetFileApisToOEM();
  
  UStringVector commandStrings;
  NCommandLineParser::SplitCommandLine(GetCommandLineW(), commandStrings);

  if(commandStrings.Size() == 1)
  {
    g_StdErr << kCopyrightString;
    PrintHelp();
    return 0;
  }
  commandStrings.Delete(0);

  CArchiveCommandLineOptions options;
  int result = ParseCommandLine(commandStrings, options);
  if (result != 0)
    return result;

  if (options.EnableHeaders)
    g_StdErr << kCopyrightString;

  if(options.HelpMode)
  {
    PrintHelp();
    return 0;
  }

  bool isExtractGroupCommand = options.Command.IsFromExtractGroup();
  if(isExtractGroupCommand || 
      options.Command.CommandType == NCommandType::kList)
  {
    if(isExtractGroupCommand)
    {
      CExtractCallbackConsole *ecs = new CExtractCallbackConsole;
      CMyComPtr<IFolderArchiveExtractCallback> extractCallback = ecs;
      ecs->PasswordIsDefined = options.PasswordEnabled;
      ecs->Password = options.Password;
      ecs->Init();

      COpenCallbackConsole openCallback;
      openCallback.PasswordIsDefined = options.PasswordEnabled;
      openCallback.Password = options.Password;

      CExtractOptions eo;
      eo.StdOutMode = options.StdOutMode;
      eo.PathMode = options.Command.GetPathMode();
      eo.TestMode = options.Command.IsTestMode();
      eo.OverwriteMode = options.OverwriteMode;
      eo.OutputDir = options.OutputDir;
      eo.YesToAll = options.YesToAll;
      HRESULT result = DecompressArchives(
          options.ArchivePathsSorted, 
          options.ArchivePathsFullSorted,
          options.WildcardCensor.Pairs.Front().Head, 
          eo, &openCallback, ecs);

      if (ecs->NumArchiveErrors != 0 || ecs->NumFileErrors != 0)
      {
        if (ecs->NumArchiveErrors != 0)
          g_StdErr << endl << "Archive Errors: " << ecs->NumArchiveErrors << endl;
        if (ecs->NumFileErrors != 0)
          g_StdErr << endl << "Sub items Errors: " << ecs->NumFileErrors << endl;
        return NExitCode::kFatalError;
      }
      if (result != S_OK)
        throw CSystemException(result);
    }
    else
    {
      HRESULT result = ListArchives(
          options.ArchivePathsSorted, 
          options.ArchivePathsFullSorted,
          options.WildcardCensor.Pairs.Front().Head, 
          options.EnableHeaders, 
          options.PasswordEnabled, 
          options.Password);
      if (result != S_OK)
        throw CSystemException(result);
    }
  }
  else if(options.Command.IsFromUpdateGroup())
  {
    UString workingDir;

    CUpdateOptions &uo = options.UpdateOptions;
    if (uo.SfxMode && uo.SfxModule.IsEmpty())
      uo.SfxModule = kDefaultSfxModule;

    bool passwordIsDefined = 
        options.PasswordEnabled && !options.Password.IsEmpty();

    COpenCallbackConsole openCallback;
    openCallback.PasswordIsDefined = passwordIsDefined;
    openCallback.Password = options.Password;

    CUpdateCallbackConsole callback;
    callback.EnablePercents = options.EnablePercents;
    callback.PasswordIsDefined = passwordIsDefined;
    callback.AskPassword = options.PasswordEnabled && options.Password.IsEmpty();
    callback.Password = options.Password;
    callback.StdOutMode = uo.StdOutMode;
    callback.Init();

    CUpdateErrorInfo errorInfo;

    HRESULT result = UpdateArchive(
        options.WildcardCensor, uo, 
        errorInfo, &openCallback, &callback);

    if (result != S_OK)
    {
      g_StdErr << "\nError:\n";
      if (!errorInfo.Message.IsEmpty())
        g_StdErr << errorInfo.Message << endl;
      if (!errorInfo.FileName.IsEmpty())
        g_StdErr << errorInfo.FileName << endl;
      if (!errorInfo.FileName2.IsEmpty())
        g_StdErr << errorInfo.FileName2 << endl;
      if (errorInfo.SystemError != 0)
        g_StdErr << NError::MyFormatMessageW(errorInfo.SystemError) << endl;
      throw CSystemException(result);
    }
    int exitCode = NExitCode::kSuccess;
    int numErrors = callback.FailedFiles.Size();
    if (numErrors == 0)
      g_StdErr << kEverythingIsOk << endl;
    else
    {
      g_StdErr << endl;
      g_StdErr << "WARNINGS for files:" << endl;
      for (int i = 0; i < numErrors; i++)
        g_StdErr << callback.FailedFiles[i] << endl;
      g_StdErr << "----------------" << endl;
      g_StdErr << "WARNING: Cannot open " << numErrors << " file";
      if (numErrors > 1)
        g_StdErr << "s";
      g_StdErr << endl;
      exitCode = NExitCode::kWarning;
    }
    return exitCode;
  }
  else 
    PrintHelpAndExit();
  return 0;
}