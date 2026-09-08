#pragma once
#include "Preferences.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <Plugin.h>
#include <string>
#include <vector>
class CommandLineOptions {
    using wstring = std::wstring;
    int cmdLineNum=-1,cmdColNum=-1;
    struct FilePath { QString path; bool has_root; };
    std::vector<FilePath> filePaths;
    static int number(const wstring& value) { try { return std::stoi(value); } catch(...) { return 0; } }
void scanGLC(ALL_PREFERENCES& allPrefs, wstring glcOptions) {
   std::size_t nOptEnd{};
   int nOptVal{};
   wstring sOption{}, sOptKey{};

   while (glcOptions.length() > 0) {
      nOptEnd = glcOptions.find(L";", 1);

      if (nOptEnd == wstring::npos) {
         sOption = glcOptions.substr(0);
         glcOptions = L"";
      }
      else {
         sOption = glcOptions.substr(0, nOptEnd);
         glcOptions = glcOptions.substr(nOptEnd + 1);
      }

      sOptKey = sOption.substr(0, 1);
      nOptVal = number(sOption.substr(1));

      if (sOptKey == L"b")
         allPrefs.useByteCol = (nOptVal != 0);
      else if (sOptKey == L"c")
         allPrefs.centerCaret = (nOptVal != 0);
      else if (sOptKey == L"d")
         allPrefs.showCalltip = (nOptVal != 0);
      else if (sOptKey == L"e")
         allPrefs.edgeBuffer = (nOptVal < 1) ? 1 : ((nOptVal > 20) ? 20 : nOptVal);
      else if (sOptKey == L"f")
         allPrefs.caretFlashSeconds = (nOptVal < 1) ? 1 : ((nOptVal > 10) ? 10 : nOptVal);
      else if (sOptKey == L"h")
         allPrefs.braceHilite = (nOptVal != 0);
      else if (sOptKey == L"p")
         allPrefs.cmdProcPersist = (nOptVal != 0);
      else if (sOptKey == L"q")
         allPrefs.cmdProcHidden = (nOptVal != 0);
   }
}

public:
    void scan(ALL_PREFERENCES& prefs) {
        bool haveLine=false,haveColumn=false; filePaths.clear();
        for(const auto& arg:QCoreApplication::arguments()) {
            if(arg.startsWith("-n")) { if(!haveLine) { cmdLineNum=number(arg.mid(2).toStdWString()); haveLine=true; } }
            else if(arg.startsWith("-c")) { if(!haveColumn) { cmdColNum=number(arg.mid(2).toStdWString()); haveColumn=true; } }
            else if(arg.startsWith("-pluginMessage=")) scanGLC(prefs,arg.mid(15).toStdWString());
            else if(!arg.startsWith('-')) { QFileInfo f(arg); filePaths.push_back({(f.isAbsolute()?arg:f.fileName()).toUpper(),f.isAbsolute()}); }
        }
    }
    bool gotoCol(int& line,int& column,bool persist) {
        if(cmdLineNum<0 || cmdColNum<0) return false;
        const QString path=QString::fromUtf8(QtPlugin::currentPath()).toUpper();
        for(auto it=filePaths.begin();it!=filePaths.end();++it) {
            if(it->path!=(it->has_root?path:QFileInfo(path).fileName())) continue;
            if(QtPlugin::sci(SCI_LINEFROMPOSITION,QtPlugin::sci(SCI_GETCURRENTPOS))+1!=cmdLineNum) return false;
            line=cmdLineNum; column=cmdColNum; if(!persist) filePaths.erase(it); return true;
        }
        return false;
    }
};
