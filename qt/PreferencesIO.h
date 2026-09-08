#pragma once
#include "Preferences.h"
#include <Plugin.h>
#include <QSettings>
#include <algorithm>
class PreferencesIO {
public:
    struct Field { const char* key; int ALL_PREFERENCES::* member; int min,max; };
    inline static const Field fields[] = {
        {"FillOnFocus",&ALL_PREFERENCES::fillOnFocus,0,1}, {"FillOnTabChange",&ALL_PREFERENCES::fillOnTabChange,0,1},
        {"ShowCallTip",&ALL_PREFERENCES::showCalltip,0,1}, {"BraceHighlight",&ALL_PREFERENCES::braceHilite,0,1},
        {"UseByteCol",&ALL_PREFERENCES::useByteCol,0,1}, {"CenterCaret",&ALL_PREFERENCES::centerCaret,0,1},
        {"EdgeBuffer",&ALL_PREFERENCES::edgeBuffer,1,20}, {"FlashSeconds",&ALL_PREFERENCES::caretFlashSeconds,1,10},
        {"ShowTooltip",&ALL_PREFERENCES::showTooltip,0,1}, {"TooltipSeconds",&ALL_PREFERENCES::tooltipSeconds,1,30},
        {"CmdProcHidden",&ALL_PREFERENCES::cmdProcHidden,0,1}, {"CmdProcPersist",&ALL_PREFERENCES::cmdProcPersist,0,1}
    };
    ALL_PREFERENCES loadPreferences() {
        ALL_PREFERENCES p;
        QSettings ini(QtPlugin::configPath("GotoLineCol.ini"),QSettings::IniFormat);
        for(auto f:fields) {
            int v=ini.value(QString("Preferences/")+f.key,p.*(f.member)).toInt();
            p.*(f.member)=f.max==1?v:std::clamp(v,f.min,f.max);
        }
        return p;
    }
    void savePreferences(const ALL_PREFERENCES& p,bool byteOnly=false) {
        QSettings ini(QtPlugin::configPath("GotoLineCol.ini"),QSettings::IniFormat);
        for(auto f:fields) if(byteOnly==(f.member==&ALL_PREFERENCES::useByteCol)) ini.setValue(QString("Preferences/")+f.key,p.*(f.member));
        ini.sync();
    }
};
