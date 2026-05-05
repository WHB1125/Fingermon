#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <M5StickCPlus.h>
#include "images.h"
#include "blockchain.h"

PokemonDNA currentDNA;

// --- UI 计时器与控制标志 ---
uint32_t uiShowTime = 0;
bool showDNACard = false;

// --- LovyanGFX 硬件配置 ---
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789  _panel_instance;
  lgfx::Bus_SPI       _bus_instance;
public:
  LGFX(void) {
    {
      auto cfg = _bus_instance.config();
      cfg.pin_mosi = 15;
      cfg.pin_sclk = 13;
      cfg.pin_dc   = 23;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs           =  5;
      cfg.pin_rst          = 18;
      cfg.panel_width      = 135;
      cfg.panel_height     = 240;
      cfg.offset_x         = 52;   
      cfg.offset_y         = 40;   
      cfg.invert           = true; 
      _panel_instance.config(cfg);
    }
    setPanel(&_panel_instance);
  }
};

LGFX lcd;
LGFX_Sprite canvas(&lcd);
LGFX_Sprite pokeSprite(&lcd);
HardwareSerial QRSerial(2); 

// --- 游戏核心参数 ---
const int W = 96;
const int H = 96;
float worldX = 0, worldY = 0; 
const float MOVE_SPEED = 5.0;

// --- 图鉴与状态系统 ---
extern const uint16_t** pokedex[]; 
extern const uint16_t* epd_bitmap_tree_allArray[]; 
int currentPkmnId = 0;       
int unlockedIdViaScan = 0;   

enum PokeState { MOVE_DOWN = 0, MOVE_UP = 3, MOVE_LEFT = 6, MOVE_LD = 9, MOVE_LU = 12 };
PokeState currentState = MOVE_DOWN;
int animIdx = 1;
int direction = 1;
bool isMirrored = false;
float accX = 0, accY = 0, accZ = 0;

// --- 地图配置 ---
#define TILE_SIZE 32
#define MAP_W 100
#define MAP_H 150  
#define T_EMPTY 0

byte myTileMap[MAP_H][MAP_W];   
byte myObjectMap[MAP_H][MAP_W]; 

const byte treePattern[4][3] = {
    {1,  5,  6 },  
    {7,  8,  9 },  
    {10, 11, 12},  
    {2,  3,  4 }   
};

// --- 物品系统 ---
bool isItemOnMap = false;
float itemWorldX = 0;
float itemWorldY = 0;

// ==========================================
// 初始化
// ==========================================
void setup() {
    M5.begin();
    M5.IMU.Init();
    QRSerial.begin(115200, SERIAL_8N1, 33, 32); 
    lcd.init();
    lcd.setRotation(0); 
    canvas.setColorDepth(16); 
    canvas.createSprite(135, 240); 
    pokeSprite.setColorDepth(16);
    pokeSprite.createSprite(W, H);
    pokeSprite.setSwapBytes(true); 
    canvas.setSwapBytes(true);
    randomSeed(analogRead(36));

    for (int r = 0; r < MAP_H; r++) {
        for (int c = 0; c < MAP_W; c++) {
            myTileMap[r][c] = random(0, 4); 
            myObjectMap[r][c] = T_EMPTY;
        }
    }

    int treesToPlant = 40;
    while (treesToPlant > 0) {
        int r = random(0, MAP_H - 4); 
        int c = random(0, MAP_W - 3); 
        bool canPlant = true;
        if (abs(r - MAP_H/2) < 10 && abs(c - MAP_W/2) < 10) canPlant = false; 
        if (canPlant) {
            for (int tr = 0; tr < 4; tr++) {
                for (int tc = 0; tc < 3; tc++) {
                    myObjectMap[r + tr][c + tc] = treePattern[tr][tc];
                }
            }
            treesToPlant--;
        }
    }

    worldX = (MAP_W * TILE_SIZE) / 2 - (135 / 2);
    worldY = (MAP_H * TILE_SIZE) / 2 - (240 / 2);
}

bool canMove(float nextWorldX, float nextWorldY) {
    float feetX = nextWorldX + (135 / 2);
    float feetY = nextWorldY + (240 / 2) + 30;
    int gridX = feetX / TILE_SIZE;
    int gridY = feetY / TILE_SIZE;
    if (gridX < 0 || gridX >= MAP_W || gridY < 0 || gridY >= MAP_H) return false;
    return (myObjectMap[gridY][gridX] == T_EMPTY);
}

void drawTileMap() {
    for (int r = 0; r < MAP_H; r++) {
        for (int c = 0; c < MAP_W; c++) {
            int x = c * TILE_SIZE - (int)worldX;
            int y = r * TILE_SIZE - (int)worldY;
            if (x <= -TILE_SIZE || x >= 135 || y <= -TILE_SIZE || y >= 240) continue;
            canvas.pushImage(x, y, TILE_SIZE, TILE_SIZE, epd_bitmap_grass_allArray[myTileMap[r][c]]);
            int objIdx = myObjectMap[r][c];
            if (objIdx != T_EMPTY) {
                canvas.pushImage(x, y, TILE_SIZE, TILE_SIZE, epd_bitmap_tree_allArray[objIdx - 1], 0xFFFF);
            }
        }
    }
}

// 🚀 修复点：扫码时正确初始化精灵球的坐标
void checkScan() {
    if (QRSerial.available()) {
        String qrData = QRSerial.readString();
        qrData.trim();

        if (qrData.startsWith("0x") && qrData.length() >= 42) {
            M5.Beep.tone(4000, 100);
            currentDNA = generateDNA(qrData);
            String upperAddr = qrData;
            upperAddr.toUpperCase(); 

            if (upperAddr.indexOf("71CA") != -1) { 
                unlockedIdViaScan = 1; // 皮卡丘
            } else {
                unlockedIdViaScan = 0; // 耿鬼
            }
            
            // 精灵球生成在玩家面前 60 像素处
            float offset = 60.0;
            itemWorldX = worldX + (135 / 2);
            itemWorldY = worldY + (240 / 2);

            if (currentState == MOVE_UP) itemWorldY -= offset;
            else if (currentState == MOVE_DOWN) itemWorldY += offset;
            else if (currentState == MOVE_LEFT) itemWorldX += isMirrored ? offset : -offset;
            else itemWorldY += offset; // 默认下方

            isItemOnMap = true;
            showDNACard = false; // 还没捡球，先不显示卡片
        }
    }
}

void drawDNACard() {
    int barY = 205; 
    canvas.fillRect(0, barY, 135, 35, lgfx::color565(30, 30, 30));
    canvas.drawFastHLine(0, barY, 135, currentDNA.isRare ? 0xFEA0 : TFT_WHITE);

    String pkmnName = (currentPkmnId == 1) ? "Pikachu" : "Gengar";
    canvas.setTextColor(currentDNA.isRare ? 0xFEA0 : TFT_CYAN);
    canvas.setCursor(5, barY + 4);
    canvas.print(pkmnName + " ");
    canvas.print(currentDNA.isRare ? "RARE" : "Common");

    canvas.setTextColor(TFT_WHITE);
    canvas.setCursor(5, barY + 15);
    canvas.print(currentDNA.address.substring(0, 8) + "..." + currentDNA.address.substring(38));

    canvas.setCursor(5, barY + 25);
    canvas.printf("HP:%d  ATK:%d", currentDNA.hp, currentDNA.atk);
}

void handleInteraction() {
    if (isItemOnMap) {
        float dx = (worldX + 135/2) - itemWorldX;
        float dy = (worldY + 240/2) - itemWorldY;
        float dist = sqrt(dx*dx + dy*dy);

        if (dist < 35 && M5.BtnA.wasPressed()) {
            M5.Beep.tone(5000, 100);
            for(int i = 0; i < 2; i++) {
                canvas.fillSprite(TFT_WHITE); canvas.pushSprite(0, 0); delay(40);
                canvas.fillSprite(TFT_BLACK); canvas.pushSprite(0, 0); delay(40);
            }
            currentPkmnId = unlockedIdViaScan; 
            isItemOnMap = false; 
            showDNACard = true;
            uiShowTime = millis();
        }
    }
}

void updateLogic() {
    M5.update();
    if (M5.BtnB.isPressed()) {
        M5.IMU.getAccelData(&accX, &accY, &accZ);
        float threshold = 0.4;
        float nextX = worldX, nextY = worldY;

        if (accX < -threshold) { isMirrored = true; nextX += MOVE_SPEED; }
        else if (accX > threshold) { isMirrored = false; nextX -= MOVE_SPEED; }

        if (abs(accX) > threshold && abs(accY) > threshold) {
            if (accY < 0) { currentState = MOVE_LU; nextY -= MOVE_SPEED; }
            else { currentState = MOVE_LD; nextY += MOVE_SPEED; }
        } else if (abs(accX) > threshold) {
            currentState = MOVE_LEFT;
        } else if (abs(accY) > threshold) {
            isMirrored = false;
            if (accY < 0) { currentState = MOVE_UP; nextY -= MOVE_SPEED; }
            else { currentState = MOVE_DOWN; nextY += MOVE_SPEED; }
        }
        
        if (canMove(nextX, worldY)) worldX = nextX;
        if (canMove(worldX, nextY)) worldY = nextY;

        static uint32_t lastAnim = 0;
        if (millis() - lastAnim > 150) { 
            animIdx += direction;
            if (animIdx >= 2 || animIdx <= 0) direction *= -1;
            lastAnim = millis();
        }
    } else { animIdx = 1; }
    worldX = constrain(worldX, 0, (MAP_W * TILE_SIZE) - 135);
    worldY = constrain(worldY, 0, (MAP_H * TILE_SIZE) - 240);
}

void loop() {
    checkScan();
    updateLogic();
    handleInteraction();

    drawTileMap();

    // 绘制精灵球
    if (isItemOnMap) {
        int sx = itemWorldX - worldX;
        int sy = itemWorldY - worldY;
        if (sx > -10 && sx < 145 && sy > -10 && sy < 250) {
            canvas.drawCircle(sx, sy, 8, TFT_BLACK); 
            canvas.fillArc(sx, sy, 0, 8, 180, 360, TFT_RED);
            canvas.fillArc(sx, sy, 0, 8, 0, 180, TFT_WHITE);
            canvas.drawFastHLine(sx - 8, sy, 17, TFT_BLACK);
            canvas.fillCircle(sx, sy, 2, TFT_WHITE);
        }
    }

    const uint16_t* ptr = pokedex[currentPkmnId][currentState + animIdx];
    pokeSprite.pushImage(0, 0, W, H, ptr);
    pokeSprite.pushRotateZoom(&canvas, 135/2, 240/2, 0, isMirrored ? -1.0 : 1.0, 1.0, 0xFFFF);
    
    if (showDNACard && (millis() - uiShowTime < 5000)) {
        drawDNACard(); 
    } else {
        showDNACard = false;
    }

    canvas.pushSprite(0, 0);
    delay(100); 
}