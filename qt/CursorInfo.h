#pragma once
// Original GotoLineColPanel::initCursorPosData, parameterized transport and encoding.
// GPL-2.0-or-later. Stage printf output to avoid the original overlapping source/destination UB.
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <vector>
#include <cstdint>
namespace GotoCore {
using std::string;
constexpr int BUFFER_500=500, BUFFER_100=100, BUFFER_20=20;
inline int safePrint(char* output,size_t size,const char* format,...) {
    std::vector<char> result(size);
    va_list args; va_start(args,format); int length=vsnprintf(result.data(),size,format,args); va_end(args);
    if(size) std::memcpy(output,result.data(),size);
    return length;
}
#define CUR_POS_DATA_LINE           "       Line: "
#define CUR_POS_DATA_CHAR_COL       "Char Column: "
#define CUR_POS_DATA_BYTE_COL       "Byte Column: "
#define CUR_POS_DATA_ANSI_BYTE      "  ANSI Byte: "
#define CUR_POS_DATA_INVALID_UTF8   "Invalid UTF-8 Byte Sequence!"
#define CUR_POS_DATA_UTF8_BYTES     "UTF-8 Bytes: "
#define CUR_POS_DATA_UNICODE        "    Unicode: "
#define CUR_POS_DATA_UNICODE_BLOCK  " Char Block: "
template<class Send,class Names> void cursorInfo(Send send,Names& nb,bool ansi,intptr_t line,intptr_t column,intptr_t atPos,char* cursorPosData) {

   unsigned char atChar;
   intptr_t colPos;

   string unicodeBlock(260, '\0');
   string unicodeName(260, '\0');

   colPos = send(SCI_GETCOLUMN, atPos, 0) + 1;
   atChar = static_cast<unsigned char>(send(SCI_GETCHARAT, atPos, 0));

   safePrint(cursorPosData, BUFFER_500, "%s%llu\n%s%llu\n%s%llu\n\n%s0x%X [%u]",
      CUR_POS_DATA_LINE, static_cast<long long>(line),
      CUR_POS_DATA_CHAR_COL, static_cast<long long>(colPos),
      CUR_POS_DATA_BYTE_COL, static_cast<long long>(column),
      CUR_POS_DATA_ANSI_BYTE, atChar, atChar);

   if ((atChar & 0x80) == 0 || ansi) {
      nb.getUnicodeBlockAndName(atChar, unicodeBlock.data(), 260, unicodeName.data(), 260);
      safePrint(cursorPosData, BUFFER_500, "%s\n%s%s\n%s", cursorPosData,
         CUR_POS_DATA_UNICODE_BLOCK, unicodeBlock.c_str(), unicodeName.c_str());
      return;
   }

   intptr_t utf8StartPos{ atPos };
   unsigned char utf8StartChar{ atChar };

   while ((utf8StartChar & 0xC0) == 0x80 && atPos - utf8StartPos < 3) {
      utf8StartPos--;
      utf8StartChar = static_cast<unsigned char>(send(SCI_GETCHARAT, utf8StartPos, 0));
   }

   if ((utf8StartChar & 0x40) == 0) {
      safePrint(cursorPosData, BUFFER_500, "%s\n%s", cursorPosData, CUR_POS_DATA_INVALID_UTF8);
      return;
   }

   unsigned char utf8ByteChar;
   intptr_t utf8BytePos{ utf8StartPos };
   int unicodeHead{ 0 }, unicodeTail{ 0 };
   bool atMark;
   char utf8Text[BUFFER_100];

   atMark = (utf8StartPos == atPos);
   safePrint(utf8Text, BUFFER_100, "%s%s0x%X%s", CUR_POS_DATA_UTF8_BYTES,
      (atMark ? "<" : ""), utf8StartChar, (atMark ? ">" : ""));

   if ((utf8StartChar & 0xC0) == 0xC0) {
      atMark = (++utf8BytePos == atPos);
      utf8ByteChar = static_cast<unsigned char>(send(SCI_GETCHARAT, utf8BytePos, 0));

      safePrint(utf8Text, BUFFER_100, "%s %s0x%X%s", utf8Text,
         (atMark ? "<" : ""), utf8ByteChar, (atMark ? ">" : ""));

      unicodeHead = (utf8StartChar & 31) << 6;
      unicodeTail = (utf8ByteChar & 63);
   }

   if ((utf8StartChar & 0xE0) == 0xE0) {
      atMark = (++utf8BytePos == atPos);
      utf8ByteChar = static_cast<unsigned char>(send(SCI_GETCHARAT, utf8BytePos, 0));

      safePrint(utf8Text, BUFFER_100, "%s %s0x%X%s", utf8Text,
         (atMark ? "<" : ""), utf8ByteChar, (atMark ? ">" : ""));

      unicodeHead = (utf8StartChar & 15) << 12;
      unicodeTail = (unicodeTail << 6) + (utf8ByteChar & 63);
   }

   if ((utf8StartChar & 0xF0) == 0xF0) {
      atMark = (++utf8BytePos == atPos);
      utf8ByteChar = static_cast<unsigned char>(send(SCI_GETCHARAT, utf8BytePos, 0));

      atMark = (utf8BytePos == atPos);
      safePrint(utf8Text, BUFFER_100, "%s %s0x%X%s", utf8Text,
         (atMark ? "<" : ""), utf8ByteChar, (atMark ? ">" : ""));

      unicodeHead = (utf8StartChar & 7) << 18;
      unicodeTail = (unicodeTail << 6) + (utf8ByteChar & 63);
   }

   if (atPos > utf8BytePos) {
      safePrint(cursorPosData, BUFFER_100, "%s\n%s", cursorPosData, CUR_POS_DATA_INVALID_UTF8);
   }
   else {
      char unicodePoint[BUFFER_20];

      safePrint(unicodePoint, BUFFER_20, "%X", (unicodeHead + unicodeTail));
      safePrint(cursorPosData, BUFFER_500, "%s\n%s\n%sU+%s%s", cursorPosData, utf8Text,
         CUR_POS_DATA_UNICODE, ((strlen(unicodePoint) % 2 == 0) ? "" : "0"), unicodePoint);
   }

   nb.getUnicodeBlockAndName((unicodeHead + unicodeTail), unicodeBlock.data(), 260, unicodeName.data(), 260);
   safePrint(cursorPosData, BUFFER_500, "%s\n%s%s\n%s", cursorPosData,
      CUR_POS_DATA_UNICODE_BLOCK, unicodeBlock.c_str(), unicodeName.c_str());
}
}
