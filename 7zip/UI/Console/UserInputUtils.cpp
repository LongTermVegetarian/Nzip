// UserInputUtils.cpp

#include "StdAfx.h"

#include "Common/StdInStream.h"
#include "Common/StdOutStream.h"

#include "UserInputUtils.h"

static const char kYes = 'Y';
static const char kNo = 'N';
static const char kYesAll = 'A';
static const char kNoAll = 'S';
static const char kAutoRename = 'U';
static const char kQuit = 'Q';

static const char *kFirstQuestionMessage = "?\n";
static const char *kHelpQuestionMessage = 
  "(Y)es / (N)o / (A)lways / (S)kip all / A(u)to rename / (Q)uit? ";

// return true if pressed Quite;
// in: anAll
// out: anAll, anYes;

NUserAnswerMode::EEnum ScanUserYesNoAllQuit()
{
  g_StdErr << kFirstQuestionMessage;
  do
  {
    g_StdErr << kHelpQuestionMessage;
    AString scannedString = g_StdIn.ScanStringUntilNewLine();
    scannedString.Trim();
    if(!scannedString.IsEmpty())
      switch(::MyCharUpper(scannedString[0]))
      {
        case kYes:
          return NUserAnswerMode::kYes;
        case kNo:
          return NUserAnswerMode::kNo;
        case kYesAll:
          return NUserAnswerMode::kYesAll;
        case kNoAll:
          return NUserAnswerMode::kNoAll;
        case kAutoRename:
          return NUserAnswerMode::kAutoRename;
        case kQuit:
          return NUserAnswerMode::kQuit;
      }
  }
  while(true);
}