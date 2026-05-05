#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <Arduino.h>

struct PokemonDNA {
    String address;
    int hp;
    int atk;
    float speed;
    String rarity;
    bool isRare;
};

// 辅助函数：判断是否为纯字母 (A-F)
bool isLetter(char c) {
    return (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

PokemonDNA generateDNA(String addr) {
    PokemonDNA dna;
    dna.address = addr;
    
    // 1. 获取地址片段 (取 0x 之后的片段)
    // 比如取地址前两位 (索引2,3) 和最后两位
    String head = addr.substring(2, 4);
    String tail = addr.substring(addr.length() - 2);
    
    int headVal = (int)strtol(head.c_str(), NULL, 16);
    int tailVal = (int)strtol(tail.c_str(), NULL, 16);

    // 2. 映射属性 (平衡区间)
    // HP: 100-150, ATK: 20-40, Speed: 5.0-7.0
    dna.hp = map(tailVal, 0, 255, 100, 150);
    dna.atk = map(headVal, 0, 255, 20, 40);
    dna.speed = 5.0 + ((float)(headVal % 20) / 10.0);

    // 3. 稀有度判定：包含两个连续相同的字母 (A-F)
    dna.isRare = false;
    dna.rarity = "COMMON";
    
    // 从索引 2 开始跳过 "0x"
    for (int i = 2; i < addr.length() - 1; i++) {
        if (isLetter(addr[i]) && addr[i] == addr[i+1]) {
            dna.isRare = true;
            dna.rarity = "RARE";
            break; 
        }
    }
    
    return dna;
}

#endif