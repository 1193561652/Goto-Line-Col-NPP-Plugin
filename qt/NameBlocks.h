#pragma once
#include <QFile>
#include <QHash>
#include <QString>
#include <vector>
#include <cstdio>
// Original CSV range lookup, INI block/key lookup and centered name output.
class NameBlocks {
    struct Block { int begin,end; QByteArray name; };
    std::vector<Block> blocks;
    QHash<QByteArray,QByteArray> names;
public:
    void init() {
        QFile csv(":/gotolinecol/UnicodeBlocks.csv"); csv.open(QIODevice::ReadOnly);
        while(!csv.atEnd()) { auto parts=csv.readLine().trimmed().split(','); if(parts.size()>=3) blocks.push_back({parts[0].trimmed().toInt(nullptr,16),parts[1].trimmed().toInt(nullptr,16),parts[2].trimmed()}); }
        QFile ini(":/gotolinecol/UnicodeData.ini"); ini.open(QIODevice::ReadOnly); QByteArray section;
        while(!ini.atEnd()) { auto line=ini.readLine().trimmed(); if(line.startsWith('[')) section=line.mid(1,line.size()-2); else { auto pos=line.indexOf('='); if(pos>=0) names.insert(section+'/'+line.left(pos).toLower(),line.mid(pos+1)); } }
    }
    int getUnicodeBlockAndName(int cp,char* block,int blocksize,char* name,int namesize) {
        QByteArray blockName; for(auto& b:blocks) if(cp>=b.begin && cp<=b.end) { blockName=b.name; break; }
        if(blockName.isEmpty()) { std::snprintf(block,blocksize,"---"); std::snprintf(name,namesize,"---"); return 0; }
        auto code=QByteArray::number(cp,16).rightJustified(cp>0xffff?6:4,'0');
        auto label=names.value(blockName+'/'+code,"---"); if(label.size()<24) label.prepend(QByteArray((24-label.size())/2,' '));
        std::snprintf(block,blocksize,"%s",blockName.constData()); return std::snprintf(name,namesize,"%s",label.constData());
    }
};
