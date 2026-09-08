#include "GoToLineColPanel.h"
#include "PreferencesDialog.h"
#include "AboutDialog.h"
#include "../../qt/CursorInfo.h"
#include "../../qt/NavigationCore.h"
#include <wchar.h>

extern HINSTANCE _gModule;
extern GotoLineColPanel _gotoPanel;
PreferencesDialog _prefsDlg;
AboutDialog _aboutDlg;

INT_PTR CALLBACK GotoLineColPanel::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) {
   switch (message) {
   case WM_COMMAND:
      switch LOWORD(wParam) {
      case IDC_GOLINE_EDIT:
         updateColumnRangeText(getInputLineValidated());
         break;

      case IDOK:
         navigateToColPos();
         break;

      case IDCANCEL:
      case IDCLOSE:
         setFocusOnEditor();
         display(false);
         break;

      case IDC_GOLINECOL_PREFS:
         SetFocus(_hSelf);
         showPreferencesDialog();
         break;

      case IDC_GOCOL_PREF_USE_BYTE_CHAR:
         allPrefs.useByteCol = IsDlgButtonChecked(_hSelf, IDC_GOCOL_PREF_USE_BYTE_CHAR);
         _prefsIO.saveByteCol(allPrefs.useByteCol);
         updatePanelInfo();
         break;

      case IDC_GOLINECOL_ABOUT_BUTTON:
         showAboutDialog();
         break;
      }

      break;

   case WM_LBUTTONDOWN:
   case WM_MBUTTONDOWN:
   case WM_RBUTTONDOWN:
      scanCommandLine();
      SetFocus(_hSelf);
      break;

   case WM_NOTIFY:
   {
      LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);

      if (pnmh->hwndFrom == _hParent && LOWORD(pnmh->code) == DMN_CLOSE) {
         display(FALSE);
      }
      else {
         switch (pnmh->code) {
         case UDN_DELTAPOS:
            bool bNext{ ((LPNMUPDOWN)lParam)->iDelta > 0 };

            switch (((LPNMHDR)lParam)->idFrom) {
            case IDC_GOLINE_SPIN:
               switchLine(bNext);
               break;

            case IDC_GOCOL_SPIN:
               switchCol(bNext);
               break;
            }
            break;
         }
      }
      break;
   }

   case WM_INITDIALOG:
      NPPDM_AutoSubclassAndThemeChildControls(_hSelf);
      break;

   case WM_CTLCOLORDLG:
   case WM_CTLCOLORLISTBOX:
   case WM_CTLCOLORSTATIC:
      if (NPPDM_IsEnabled()) {
         return NPPDM_OnCtlColorDarker(reinterpret_cast<HDC>(wParam));
      }
      break;

   case WM_CTLCOLOREDIT:
      if (NPPDM_IsEnabled()) {
         return NPPDM_OnCtlColorSofter(reinterpret_cast<HDC>(wParam));
      }
      break;

   case WM_PRINTCLIENT:
      if (NPPDM_IsEnabled()) return TRUE;
      break;

   case WM_SETFOCUS:
      if (allPrefs.fillOnFocus)
         updatePanelColPos();
      break;

   case WM_SIZE:
      onPanelResize(lParam);
      break;

   case WM_SHOWWINDOW:
      Utils::checkMenuItem(MI_GOTO_PANEL, wParam);
      break;

   default:
      return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
   }

   return FALSE;
}

void GotoLineColPanel::initPrefs() {
   _prefsIO.init();
   allPrefs = _prefsIO.loadPreferences();
   scanCommandLine();
   nb.init();
}

void GotoLineColPanel::initPanel() {
   bool recentOS = Utils::checkBaseOS(WV_VISTA);
   wstring fontName = recentOS ? L"Consolas" : L"Courier New";
   int fontHeight = recentOS ? 10 : 8;

   hFieldInfo = GetDlgItem(_hSelf, IDC_GOLINECOL_FIELD_INFO);

   CheckDlgButton(_hSelf, IDC_GOCOL_PREF_USE_BYTE_CHAR, (allPrefs.useByteCol != FALSE) ? BST_CHECKED : BST_UNCHECKED);

   Utils::setFontBold(_hSelf, IDOK);
   Utils::setFont(_hSelf, IDC_GOLINECOL_FIELD_LABEL, fontName, fontHeight, FW_BOLD, FALSE, TRUE);
   Utils::setFont(_hSelf, IDC_GOLINECOL_FIELD_INFO, fontName, fontHeight);

   Utils::loadBitmap(_hSelf, IDC_GOLINECOL_ABOUT_BUTTON, IDB_GOLINECOL_ABOUT_BITMAP);
   Utils::addTooltip(_hSelf, IDC_GOLINECOL_ABOUT_BUTTON, L"", ABOUT_DIALOG_TITLE, TRUE);

   if (_gLanguage != LANG_ENGLISH) localize();
}

void GotoLineColPanel::onBufferActivated() {
   int lineNum{}, colNum{};
   if ((isVisible() || allPrefs.cmdProcHidden) && cmdOpt.gotoCol(lineNum, colNum, allPrefs.cmdProcPersist)) {
      navigateToColPos(lineNum, colNum);
      if (isVisible()) updatePanelColPos();
   }
   else if (isVisible() && allPrefs.fillOnTabChange) {
      updatePanelColPos();
   }
}

void GotoLineColPanel::localize() {
   SetDlgItemText(_hSelf, IDC_GOLINE_STATIC, GOLINECOL_LABEL_GOLINE);
   SetDlgItemText(_hSelf, IDC_GOCOL_STATIC, GOLINECOL_LABEL_BYTE_COL);
   SetDlgItemText(_hSelf, IDOK, GOLINECOL_LABEL_BTN_GO);
   SetDlgItemText(_hSelf, IDCLOSE, GOLINECOL_LABEL_BTN_CLOSE);
   SetDlgItemText(_hSelf, IDC_GOLINECOL_PREFS, MENU_PREFERENCES);
   SetDlgItemText(_hSelf, IDC_GOCOL_PREF_USE_BYTE_CHAR, GOCOL_PREF_USE_BYTE_CHAR);
   SetDlgItemText(_hSelf, IDC_GOLINECOL_FIELD_LABEL, GOLINECOL_FIELD_LABEL);
}

void GotoLineColPanel::display(bool toShow) {
   DockingDlgInterface::display(toShow);

   panelMounted = toShow;

   if (toShow) {
      SetFocus(GetDlgItem(_hSelf, IDC_GOLINE_EDIT));
   }
   else {
      if (_prefsDlg.isCreated() && _prefsDlg.isVisible())
         _prefsDlg.display(FALSE);

      if (_aboutDlg.isCreated() && _aboutDlg.isVisible())
         _aboutDlg.display(FALSE);
   }
}

void GotoLineColPanel::refreshDarkMode() {
   if (isCreated()) {
      NPPDM_AutoThemeChildControls(_hSelf);
      redraw();
   }

   if (_prefsDlg.isCreated())
      _prefsDlg.refreshDarkMode();

   if (_aboutDlg.isCreated())
      _aboutDlg.refreshDarkMode();
}

void GotoLineColPanel::setParent(HWND parent2set) {
   _hParent = parent2set;
}

void GotoLineColPanel::showPreferencesDialog() {
   _prefsDlg.doDialog((HINSTANCE)_gModule);
}

void GotoLineColPanel::showAboutDialog() {
   _aboutDlg.doDialog((HINSTANCE)_gModule);
}


void GotoLineColPanel::reloadPanelPreferences() {
   allPrefs = _prefsIO.loadPreferences();
   updatePanelInfo();
}

void GotoLineColPanel::updatePanelInfo() {
   HWND hScintilla{ GetCurrentScintilla() };
   if (!hScintilla) return;

   SetDlgItemText(_hSelf, IDC_GOCOL_STATIC,
      allPrefs.useByteCol ? GOLINECOL_LABEL_BYTE_COL : GOLINECOL_LABEL_CHAR_COL);

   int tabWidth = allPrefs.useByteCol ? 1 : static_cast<int>(SendMessage(hScintilla, SCI_GETTABWIDTH, 0, 0));

   wstring tabNote = (tabWidth == 1) ? GOLINECOL_TAB_SINGLE_COL :
      (GOLINECOL_TAB_MULTI_COL + to_wstring(tabWidth) + GOLINECOL_TAB_COLUMNS);
   SetDlgItemText(_hSelf, IDC_GOCOL_TAB_CHAR_NOTE, tabNote.c_str());

   wstring utf8CharNote = allPrefs.useByteCol ? GOLINECOL_UTF8_MULTI_COL : GOLINECOL_UTF8_SINGLE_COL;
   SetDlgItemText(_hSelf, IDC_GOCOL_UTF8_CHAR_NOTE, utf8CharNote.c_str());

   updatePanelColPos();
}

void GotoLineColPanel::updatePanelColPos() {
   HWND hScintilla{ GetCurrentScintilla() };
   if (!hScintilla) return;

   intptr_t pos = SendMessage(hScintilla, SCI_GETCURRENTPOS, 0, 0);
   intptr_t line = SendMessage(hScintilla, SCI_LINEFROMPOSITION, pos, 0) + 1;
   intptr_t col = getDocumentColumn(hScintilla, pos, line);

   SetDlgItemText(_hSelf, IDC_GOLINE_EDIT, to_wstring(line).c_str());
   updateLineRangeText();

   SetDlgItemText(_hSelf, IDC_GOCOL_EDIT, to_wstring(col).c_str());
   updateColumnRangeText(line);
}

void GotoLineColPanel::clearCalltip() {
   HWND hScintilla{ GetCurrentScintilla() };
   if (!hScintilla) return;

   SendMessage(hScintilla, SCI_CALLTIPCANCEL, NULL, NULL);
}

/// *** Private Functions: *** ///

intptr_t GotoLineColPanel::getLineCount() {
   HWND hScintilla{ GetCurrentScintilla() };
   if (!hScintilla) return -1;

   return SendMessage(hScintilla, SCI_GETLINECOUNT, 0, 0);
};

void GotoLineColPanel::setFocusOnEditor() {
   HWND hScintilla{ GetCurrentScintilla() };
   if (!hScintilla) return;

   SendMessage(hScintilla, SCI_GRABFOCUS, 0, 0);
}

void GotoLineColPanel::onPanelResize(LPARAM lParam) {
   RECT rcInfo;
   GetWindowRect(hFieldInfo, &rcInfo);

   // Get fieldInfo top-leftEdge coordinates relative to dock panel
   POINT pt{ rcInfo.left, rcInfo.top };
   ScreenToClient(_hSelf, &pt);

   MoveWindow(hFieldInfo, pt.x, pt.y, (LOWORD(lParam) - pt.x - 3), (rcInfo.bottom - rcInfo.top), TRUE);
   SetWindowTextA(hFieldInfo, cursorPosData);

   // About button
   HWND hAboutBtn{ GetDlgItem(_hSelf, IDC_GOLINECOL_ABOUT_BUTTON) };
   RECT rcAboutBtn;
   GetWindowRect(hAboutBtn, &rcAboutBtn);

   int aboutBtnWidth{ rcAboutBtn.right - rcAboutBtn.left };
   int aboutBtnHeight{ rcAboutBtn.bottom - rcAboutBtn.top };

   MoveWindow(hAboutBtn, (LOWORD(lParam) - aboutBtnWidth - 3), (HIWORD(lParam) - aboutBtnHeight - 3), aboutBtnWidth, aboutBtnHeight, TRUE);
}


intptr_t GotoLineColPanel::getLineMaxPos(intptr_t line) const {
   HWND editor = GetCurrentScintilla();
   if (!editor) return -1;
   return GotoCore::getLineMaxPos([editor](unsigned int m, uintptr_t w, intptr_t l) {
      return SendMessage(editor, m, w, l);
   }, allPrefs.useByteCol != 0, line);
}
intptr_t GotoLineColPanel::getDocumentColumn(HWND editor, intptr_t pos, intptr_t line) const {
   return GotoCore::getDocumentColumn([editor](unsigned int m, uintptr_t w, intptr_t l) {
      return SendMessage(editor, m, w, l);
   }, allPrefs.useByteCol != 0, pos, line);
}
intptr_t GotoLineColPanel::setDocumentColumn(HWND editor, intptr_t line, intptr_t start, intptr_t max, intptr_t column) const {
   return GotoCore::setDocumentColumn([editor](unsigned int m, uintptr_t w, intptr_t l) {
      return SendMessage(editor, m, w, l);
   }, allPrefs.useByteCol != 0, line, start, max, column);
}

intptr_t GotoLineColPanel::getInputLineValidated() {
   BOOL isSuccessful;

   intptr_t line = GetDlgItemInt(_hSelf, IDC_GOLINE_EDIT, &isSuccessful, FALSE);
   if (!isSuccessful)
      return 1;

   intptr_t lineCount = getLineCount();
   return ((line < 0) ? 1 : ((line > lineCount) ? lineCount : line));
};

int GotoLineColPanel::getInputColumn() const {
   BOOL isSuccessful;

   int col = GetDlgItemInt(_hSelf, IDC_GOCOL_EDIT, &isSuccessful, FALSE);
   return (isSuccessful ? col : 1);
};

void GotoLineColPanel::updateLineRangeText() {
   SetDlgItemText(_hSelf, IDC_GOLINE_RANGE, (GOLINECOL_MAX_FOR_FILE + to_wstring(getLineCount()) + L"]").c_str());
}

void GotoLineColPanel::updateColumnRangeText(intptr_t line) {
   SetDlgItemText(_hSelf, IDC_GOCOL_RANGE, (GOLINECOL_MAX_FOR_LINE + to_wstring(getLineMaxPos(line)) + L"]").c_str());
}

void GotoLineColPanel::switchLine(bool bNext) {
   intptr_t inputLine{ getInputLineValidated() };

   if (bNext) {
      if (inputLine + 1 > getLineCount()) {
         clearCalltip();
         return;
      }
      inputLine++;
   }
   else {
      if (inputLine == 1) {
         clearCalltip();
         return;
      }
      inputLine--;
   }

   SetDlgItemText(_hSelf, IDC_GOLINE_EDIT, to_wstring(inputLine).c_str());
   updateColumnRangeText(inputLine);
   navigateToColPos();
}

void GotoLineColPanel::switchCol(bool bNext) {
   int inputCol{ getInputColumn() };

   if (!bNext) {
      if (inputCol + 1 > getLineMaxPos(getInputLineValidated())) {
         clearCalltip();
         return;
      }
      inputCol++;
   }
   else {
      if (inputCol == 1) {
         clearCalltip();
         return;
      }
      inputCol--;
   }

   SetDlgItemText(_hSelf, IDC_GOCOL_EDIT, to_wstring(inputCol).c_str());
   navigateToColPos();
}

int GotoLineColPanel::navigateToColPos() {
   return navigateToColPos(getInputLineValidated(), getInputColumn());
}

int GotoLineColPanel::navigateToColPos(intptr_t line, intptr_t column) {
   HWND hScintilla{ GetCurrentScintilla() };
   if (!hScintilla) return FALSE;

   SendMessage(hScintilla, SCI_ENSUREVISIBLE, line - 1, 0);

   intptr_t lineMaxPos = getLineMaxPos(line);
   intptr_t lineStartPos = SendMessage(hScintilla, SCI_POSITIONFROMLINE, line - 1, 0);


   if (allPrefs.centerCaret) {
      SendMessage(hScintilla, SCI_SETXCARETPOLICY, CARET_JUMPS | CARET_EVEN, (LPARAM)0);
      SendMessage(hScintilla, SCI_SETYCARETPOLICY, CARET_JUMPS | CARET_EVEN, (LPARAM)0);
   }
   else {
      // Clear a buffer of edgebuffer positions on either side if possible so that
      // the cursor is not stuck while being aligned with the side margins
      SendMessage(hScintilla, SCI_SETXCARETPOLICY, 0, (LPARAM)0);
      SendMessage(hScintilla, SCI_SETYCARETPOLICY, 0, (LPARAM)0);
      setDocumentColumn(hScintilla, line, lineStartPos, lineMaxPos, column - allPrefs.edgeBuffer);
      setDocumentColumn(hScintilla, line, lineStartPos, lineMaxPos, column + allPrefs.edgeBuffer);
   }

   intptr_t atPos = setDocumentColumn(hScintilla, line, lineStartPos, lineMaxPos, column);
   setFocusOnEditor();

   // Display call tip
   if (allPrefs.showCalltip) {
      // No need to call initCursorPosData() here.
      // cursorPosData will be populated in loadCursorPosData via SCN_UPDATEUI
      //initCursorPosData(hScintilla, line, (atPos - lineStartPos + 1), atPos);
      PostMessage(hScintilla, SCI_CALLTIPSHOW, atPos, (LPARAM)cursorPosData);
   }

   // Flash caret
   HANDLE hThread = CreateThread(NULL, 0, threadPositionHighlighter, 0, 0, NULL);
   if (hThread) CloseHandle(hThread);

   return TRUE;
}


void GotoLineColPanel::initCursorPosData(HWND hScintilla, intptr_t line, intptr_t column, intptr_t atPos) {
   const bool ansi = NppMessage(NPPM_GETBUFFERENCODING, NppMessage(NPPM_GETCURRENTBUFFERID, 0, 0), 0) == 0;
   GotoCore::cursorInfo([hScintilla](unsigned int message, uintptr_t w, intptr_t l) {
      return SendMessage(hScintilla, message, w, l);
   }, nb, ansi, line, column, atPos, cursorPosData);
}

void GotoLineColPanel::loadCursorPosData() {
   HWND hScintilla{ GetCurrentScintilla() };
   if (!hScintilla) return;

   intptr_t curPos = SendMessage(hScintilla, SCI_GETCURRENTPOS, NULL, NULL);
   intptr_t curLine = SendMessage(hScintilla, SCI_LINEFROMPOSITION, curPos, NULL);
   intptr_t lineStart = SendMessage(hScintilla, SCI_POSITIONFROMLINE, curLine, NULL);

   initCursorPosData(hScintilla, (curLine + 1), (curPos - lineStart + 1), curPos);

   SetWindowTextA(hFieldInfo, cursorPosData);
}

DWORD WINAPI GotoLineColPanel::threadPositionHighlighter(void*) {
   HWND hScintilla{ GetCurrentScintilla() };
   if (!hScintilla) return FALSE;

   ALL_PREFERENCES allPrefs = _gotoPanel.getPrefs();

   // Look for Idem Potency Hold
   if (idemPotentKey)
      //  Idem Potency check failed. Another thread is processing this same function. Return immediately.
      return FALSE;
   else
      // OK to continue. Set Idem Potency Hold
      idemPotentKey = TRUE;

   // Modify caret style briefly to highlight the new position
   int currCaret = static_cast<int>(SendMessage(hScintilla, SCI_GETCARETSTYLE, 0, 0));
   SendMessage(hScintilla, SCI_SETCARETSTYLE, CARETSTYLE_BLOCK, 0);
   Sleep(allPrefs.caretFlashSeconds * 1000);
   SendMessage(hScintilla, SCI_SETCARETSTYLE, currCaret, 0);

   if (allPrefs.braceHilite) {
      SendMessage(hScintilla, SCI_BRACEHIGHLIGHT, SendMessage(hScintilla, SCI_GETCURRENTPOS, 0, 0), -1);
   }

   // Clear Idem Potency Hold
   idemPotentKey = FALSE;

   return TRUE;
}
