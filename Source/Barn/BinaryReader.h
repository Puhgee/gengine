//
//  BinaryReader.h
//  GEngine
//
//  Created by Clark Kromenaker on 8/5/17.
//

#pragma once
#include <fstream>

class BinaryReader
{
public:
    BinaryReader(const char* filePath);
    BinaryReader(const char* memory, unsigned int memoryLength);
    ~BinaryReader();
    
    bool CanRead() const;
    
    void Seek(int position);
    void Skip(int size);
    
    int GetPosition();
    
    void Read(char* charBuffer, int size);
    
    uint8_t ReadUByte();
    int8_t ReadByte();
    
    uint16_t ReadUShort();
    int16_t ReadShort();
    
    uint32_t ReadUInt();
    int32_t ReadInt();
    
private:
    std::istream* stream = nullptr;
};
