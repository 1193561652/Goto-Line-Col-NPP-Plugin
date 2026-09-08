#pragma once
// Original column calculations; Scintilla transport parameterized. GPL-2.0-or-later.
#include <cstdint>
namespace GotoCore {
template<class Send> intptr_t getLineMaxPos(Send send, bool useByteCol, intptr_t line) {

   intptr_t endPos = send(SCI_GETLINEENDPOSITION, line - 1, 0);

   intptr_t col = (useByteCol) ?
      endPos - send(SCI_POSITIONFROMLINE, line - 1, 0) : send(SCI_GETCOLUMN, endPos, 0);

   return col + 1;
}
template<class Send> intptr_t getDocumentColumn(Send send, bool useByteCol, intptr_t pos, intptr_t line) {
   intptr_t col = (useByteCol) ?
      pos - send(SCI_POSITIONFROMLINE, line - 1, 0) : send(SCI_GETCOLUMN, pos, 0);

   return col + 1;
}
template<class Send> intptr_t setDocumentColumn(Send send, bool useByteCol, intptr_t line, intptr_t lineStartPos, intptr_t lineMaxPos, intptr_t column) {
   column = (column < 1) ? 1 :
      (column > lineMaxPos) ? lineMaxPos : column;

   intptr_t gotoPos = (useByteCol) ? lineStartPos + column - 1 : send(SCI_FINDCOLUMN, line - 1, column - 1);

   send(SCI_GOTOPOS, gotoPos, 0);
   return gotoPos;
}
}
