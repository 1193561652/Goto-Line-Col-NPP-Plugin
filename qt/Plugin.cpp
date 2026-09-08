// GPL-2.0-or-later. Original GotoLineCol owners with Qt UI/transport.
#include <Plugin.h>
#include "PreferencesIO.h"
#include "NavigationCore.h"
#include "CursorInfo.h"
#include "NameBlocks.h"
#include "CommandLineOptions.h"
#include <QApplication>
#include <QDockWidget>
#include <QMainWindow>
#include <QFormLayout>
#include <QSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QTimer>
#include <QPointer>
#include <QFileInfo>
#include <QSignalBlocker>
#include <functional>
namespace {
using namespace QtPlugin;
class NavigationSpinBox : public QSpinBox {
public:
    std::function<void()> stepped;
    void stepBy(int steps) override { const auto previous=value(); QSpinBox::stepBy(steps); if(value()!=previous && stepped) stepped(); }
};
QMainWindow* mainWindow() { for(auto w:QApplication::topLevelWidgets()) if(auto m=qobject_cast<QMainWindow*>(w)) return m; return nullptr; }
void aboutPlugin(void*) { about("Goto Line, Column","Goto Line, Column plugin for Notepad++\nVersion: 2.4.5.1\nCopyright (C) 2025 by Shridhar Kumar\nQt port using the original algorithms.\nhttps://github.com/shriprem/Goto-Line-Col-NPP-Plugin"); }
class GotoLineColPanel : public QDockWidget {
    PreferencesIO prefsIO;
    ALL_PREFERENCES allPrefs;
    NameBlocks nb;
    CommandLineOptions cmdOpt;
    NavigationSpinBox *line,*column;
    QCheckBox* bytes;
    QLabel *range,*columnLabel,*notes,*info;
    QTimer flash;
    int flashView=-1, previousCaret=0;
    char cursorPosData[500]{};
    void restoreCaret() {
        if(flashView<0 || !host) return;
        host->send_scintilla(host->host_context,flashView,SCI_SETCARETSTYLE,previousCaret,0);
        if(allPrefs.braceHilite) {
            auto pos=host->send_scintilla(host->host_context,flashView,SCI_GETCURRENTPOS,0,0);
            host->send_scintilla(host->host_context,flashView,SCI_BRACEHIGHLIGHT,pos,-1);
        }
        flashView=-1;
    }
    void updateRange() {
        auto max=GotoCore::getLineMaxPos(sci,allPrefs.useByteCol,line->value());
        range->setText(QString("[Max for file: %1]  [Max for line: %2]").arg(sci(SCI_GETLINECOUNT)).arg(max));
    }
    void navigateToColPos(intptr_t targetLine,intptr_t targetColumn) {
        targetLine=std::clamp<intptr_t>(targetLine,1,sci(SCI_GETLINECOUNT));
        sci(SCI_ENSUREVISIBLE,targetLine-1);
        auto max=GotoCore::getLineMaxPos(sci,allPrefs.useByteCol,targetLine);
        auto start=sci(SCI_POSITIONFROMLINE,targetLine-1);
        auto go=[&](intptr_t c) { return GotoCore::setDocumentColumn(sci,allPrefs.useByteCol,targetLine,start,max,c); };
        if(allPrefs.centerCaret) { sci(SCI_SETXCARETPOLICY,CARET_JUMPS|CARET_EVEN); sci(SCI_SETYCARETPOLICY,CARET_JUMPS|CARET_EVEN); }
        else { sci(SCI_SETXCARETPOLICY,0); sci(SCI_SETYCARETPOLICY,0); go(targetColumn-allPrefs.edgeBuffer); go(targetColumn+allPrefs.edgeBuffer); }
        auto pos=go(targetColumn); sci(SCI_GRABFOCUS);
        loadCursorPosData();
        if(allPrefs.showCalltip) sci(SCI_CALLTIPSHOW,pos,reinterpret_cast<intptr_t>(cursorPosData));
        // Original idempotency rule: another navigation does not restart a running flash.
        if(!flash.isActive()) {
            flashView=host->get_current_view(host->host_context); previousCaret=int(sci(SCI_GETCARETSTYLE));
            sci(SCI_SETCARETSTYLE,CARETSTYLE_BLOCK); flash.start(allPrefs.caretFlashSeconds*1000);
        }
    }
public:
    GotoLineColPanel() : QDockWidget("Goto Line, Column",mainWindow()) {
        setObjectName("gotoLineColPanel");
        setWindowIcon(QIcon(":/gotolinecol/icon.ico"));
        const bool exists=QFileInfo::exists(configPath("GotoLineCol.ini"));
        allPrefs=prefsIO.loadPreferences();
        if(!exists) { prefsIO.savePreferences(allPrefs); prefsIO.savePreferences(allPrefs,true); }
        cmdOpt.scan(allPrefs); nb.init();
        auto body=new QWidget; auto form=new QFormLayout(body);
        line=new NavigationSpinBox; line->setObjectName("gotoLine"); line->setRange(1,INT_MAX);
        column=new NavigationSpinBox; column->setObjectName("gotoColumn"); column->setRange(1,INT_MAX);
        form->addRow("Goto line:",line); columnLabel=new QLabel; form->addRow(columnLabel,column);
        range=new QLabel; range->setObjectName("gotoRange"); form->addRow(range);
        bytes=new QCheckBox("Use byte count for column value computation"); bytes->setObjectName("useByteCol"); form->addRow(bytes);
        notes=new QLabel; notes->setWordWrap(true); form->addRow(notes);
        auto go=new QPushButton("&Go"); go->setObjectName("gotoGo"); form->addRow(go);
        auto close=new QPushButton("&Close"); form->addRow(close);
        auto preferences=new QPushButton("&Preferences"); form->addRow(preferences);
        info=new QLabel; info->setObjectName("cursorInfo"); info->setFont(QFont("Consolas")); info->setTextInteractionFlags(Qt::TextSelectableByMouse); form->addRow("Cursor Position Data",info);
        auto aboutButton=new QPushButton("&About"); form->addRow(aboutButton);
        setWidget(body); resize(380,520);
        if(auto m=mainWindow()) m->addDockWidget(Qt::RightDockWidgetArea,this);
        connect(go,&QPushButton::clicked,this,[this] { navigateToColPos(line->value(),column->value()); });
        connect(close,&QPushButton::clicked,this,[this] { sci(SCI_GRABFOCUS); hide(); });
        connect(preferences,&QPushButton::clicked,this,[this] { showPreferencesDialog(); });
        connect(aboutButton,&QPushButton::clicked,this,[] { invoke<aboutPlugin>(nullptr); });
        connect(line,qOverload<int>(&QSpinBox::valueChanged),this,[this] { updateRange(); });
        connect(bytes,&QCheckBox::toggled,this,[this](bool value) { allPrefs.useByteCol=value; prefsIO.savePreferences(allPrefs,true); updatePanelInfo(); });
        connect(this,&QDockWidget::visibilityChanged,this,[](bool visible) { if(!commands.empty()) commands[0].initially_checked=visible; });
        line->stepped=column->stepped=[this] { navigateToColPos(line->value(),column->value()); };
        connect(qApp,&QApplication::focusChanged,this,[this](QWidget* old,QWidget* now) {
            if(allPrefs.fillOnFocus && now && isAncestorOf(now) && (!old || !isAncestorOf(old))) updatePanelColPos();
        });
        flash.setSingleShot(true); connect(&flash,&QTimer::timeout,this,[this] { restoreCaret(); });
        hide();
    }
    ~GotoLineColPanel() override { flash.stop(); restoreCaret(); sci(SCI_CALLTIPCANCEL); }
    void updatePanelColPos() {
        QSignalBlocker a(line),b(column);
        auto pos=sci(SCI_GETCURRENTPOS), ln=sci(SCI_LINEFROMPOSITION,pos)+1;
        line->setValue(int(std::min<intptr_t>(ln,INT_MAX)));
        column->setValue(int(std::min<intptr_t>(GotoCore::getDocumentColumn(sci,allPrefs.useByteCol,pos,ln),INT_MAX)));
        updateRange();
    }
    void updatePanelInfo() {
        { QSignalBlocker blocker(bytes); bytes->setChecked(allPrefs.useByteCol); }
        columnLabel->setText(allPrefs.useByteCol?"Byte column:":"Char column:");
        notes->setText(QString("Each TAB char counts as %1 column(s). Each multibyte UTF-8 char counts as %2 column(s).")
            .arg(allPrefs.useByteCol?1:int(sci(SCI_GETTABWIDTH))).arg(allPrefs.useByteCol?"multiple":"one"));
        updatePanelColPos(); loadCursorPosData();
    }
    void loadCursorPosData() {
        auto pos=sci(SCI_GETCURRENTPOS),ln=sci(SCI_LINEFROMPOSITION,pos),start=sci(SCI_POSITIONFROMLINE,ln);
        bool ansi=sci(SCI_GETCODEPAGE)!=SC_CP_UTF8;
        if(host->struct_size>=offsetof(NppPluginHostInfo,get_buffer_encoding)+sizeof(host->get_buffer_encoding) && host->get_buffer_encoding && host->get_current_buffer_id)
            ansi=host->get_buffer_encoding(host->host_context,host->get_current_buffer_id(host->host_context))==0;
        GotoCore::cursorInfo(sci,nb,ansi,ln+1,pos-start+1,pos,cursorPosData);
        info->setText(QString::fromUtf8(cursorPosData));
    }
    void onBufferActivated() {
        int ln,col;
        if((isVisible()||allPrefs.cmdProcHidden) && cmdOpt.gotoCol(ln,col,allPrefs.cmdProcPersist)) { navigateToColPos(ln,col); if(isVisible()) updatePanelColPos(); }
        else if(isVisible()&&allPrefs.fillOnTabChange) updatePanelColPos();
    }
    void showPreferencesDialog() {
        QDialog dialog(this); dialog.setObjectName("gotoPreferences"); dialog.setWindowTitle("Preferences"); auto form=new QFormLayout(&dialog);
        std::vector<std::pair<QWidget*,PreferencesIO::Field>> fields;
        const char* labels[]={"Auto-fill on panel focus","Auto-fill on document tab change","Display character code at destination","Highlight character at destination","Use byte columns","Center cursor at destination","Edge buffer","Cursor flash duration (seconds)","Show tooltips","Tooltip duration (seconds)","Process command line while hidden","Keep command-line positions for subsequent visits"};
        int index=0;
        for(auto f:PreferencesIO::fields) { if(f.member==&ALL_PREFERENCES::useByteCol) continue;
            // Byte mode belongs to the main panel, as in the original dialog.
            if(index==4) ++index;
            const char* label=labels[index++]; QWidget* input;
            if(f.max==1) { auto checkbox=new QCheckBox(label); checkbox->setChecked(allPrefs.*(f.member)!=0); form->addRow(checkbox); input=checkbox; }
            else { auto spin=new QSpinBox; spin->setRange(f.min,f.max); spin->setValue(allPrefs.*(f.member)); form->addRow(label,spin); input=spin; }
            input->setObjectName(f.key); input->setToolTip(allPrefs.showTooltip?label:""); input->setToolTipDuration(allPrefs.tooltipSeconds*1000);
            fields.push_back({input,f});
        }
        auto buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::RestoreDefaults); form->addRow(buttons);
        connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept); connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
        connect(buttons->button(QDialogButtonBox::RestoreDefaults),&QPushButton::clicked,&dialog,[&] { ALL_PREFERENCES defaults; for(auto pair:fields) { if(auto c=qobject_cast<QCheckBox*>(pair.first)) c->setChecked(defaults.*(pair.second.member)!=0); else qobject_cast<QSpinBox*>(pair.first)->setValue(defaults.*(pair.second.member)); } });
        if(dialog.exec()==QDialog::Accepted) { for(auto pair:fields) { if(auto c=qobject_cast<QCheckBox*>(pair.first)) allPrefs.*(pair.second.member)=c->isChecked(); else allPrefs.*(pair.second.member)=qobject_cast<QSpinBox*>(pair.first)->value(); } prefsIO.savePreferences(allPrefs); updatePanelInfo(); }
    }
};
QPointer<GotoLineColPanel> panel;
GotoLineColPanel* getPanel() { if(!panel) panel=new GotoLineColPanel; return panel; }
void toggle(void*) { auto p=getPanel(); if(p->isVisible()) p->hide(); else { p->show(); p->updatePanelInfo(); p->findChild<QSpinBox*>("gotoLine")->setFocus(); } }
void preferences(void*) { getPanel()->showPreferencesDialog(); }
void setup() { delete panel; addToggle("&Show GotoLineCol Panel",invoke<toggle>,false); commands[0].shortcut={1,0,0,0,0x76}; add("&Preferences",invoke<preferences>); add("",nullptr); add("&About",invoke<aboutPlugin>); getPanel(); }
void notify(const NppPluginNotification* n) {
    if(n->code==NPP_PLUGIN_NOTIFICATION_SHUTDOWN) delete panel;
    else if(n->code==NPP_PLUGIN_NOTIFICATION_BUFFER_ACTIVATED) getPanel()->onBufferActivated();
    else if(n->code==NPP_PLUGIN_NOTIFICATION_UPDATE_UI && panel) { sci(SCI_CALLTIPCANCEL); panel->loadCursorPosData(); }
}
}
NPP_QT_EXPORTS("Goto Line, Column",setup,notify)
