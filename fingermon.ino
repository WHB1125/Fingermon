#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <M5StickCPlus.h>
#include "images.h"

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
      
      // 🚀 修复 1 & 2：物理偏移量和颜色反转校准
      cfg.offset_x         = 52;   // 修正左上角偏移
      cfg.offset_y         = 40;   // 修正左上角偏移
      cfg.invert           = true; // 修正 IPS 屏反色问题
      
      _panel_instance.config(cfg);
    }
    setPanel(&_panel_instance);
  }
};

LGFX lcd;
LGFX_Sprite canvas(&lcd);     // 底层：地图大画布 (135x240)
LGFX_Sprite pokeSprite(&lcd); // 顶层：宝可梦专用小画布 (96x96)

// --- 游戏配置 ---
const int W = 96;
const int H = 96;
float worldX = 0, worldY = 0; 
const float MOVE_SPEED = 8.0;

enum PokeState {
    MOVE_DOWN = 0, MOVE_UP = 3, MOVE_LEFT = 6, MOVE_LD = 9, MOVE_LU = 12
};

PokeState currentState = MOVE_DOWN;
int animIdx = 1;
int direction = 1;
bool isMirrored = false;
float accX = 0, accY = 0, accZ = 0;

void drawSimpleTileMap() {
    canvas.fillSprite(lgfx::color565(200, 255, 200)); 
    int gridSize = 40;
    int offX = (int)worldX % gridSize;
    int offY = (int)worldY % gridSize;
    for (int x = -offX; x < 135; x += gridSize) {
        canvas.drawFastVLine(x, 0, 240, lgfx::color565(150, 220, 150));
    }
    for (int y = -offY; y < 240; y += gridSize) {
        canvas.drawFastHLine(0, y, 135, lgfx::color565(150, 220, 150));
    }
}

void setup() {
    M5.begin();
    M5.IMU.Init();
    
    lcd.init();
    lcd.setRotation(0); // 确保竖屏
    
    // 初始化大画布
    canvas.setColorDepth(16); 
    canvas.createSprite(135, 240); 
    
    // 初始化宝可梦画布
    pokeSprite.setColorDepth(16);
    pokeSprite.createSprite(W, H);
    pokeSprite.setSwapBytes(true); 
}

void updateLogic() {
    M5.update();
    
    if (M5.BtnB.isPressed()) {
        M5.IMU.getAccelData(&accX, &accY, &accZ);
        float threshold = 0.4;
        
        if (accX < -threshold) { isMirrored = true; worldX += MOVE_SPEED; }
        else if (accX > threshold) { isMirrored = false; worldX -= MOVE_SPEED; }

        bool xActive = abs(accX) > threshold;
        bool yActive = abs(accY) > threshold;

        if (xActive && yActive) {
            if (accY < 0) { currentState = MOVE_LU; worldY -= MOVE_SPEED; }
            else          { currentState = MOVE_LD; worldY += MOVE_SPEED; }
        } 
        else if (xActive) {
            currentState = MOVE_LEFT;
        } 
        else if (yActive) {
            isMirrored = false;
            if (accY < 0) { currentState = MOVE_UP; worldY -= MOVE_SPEED; }
            else          { currentState = MOVE_DOWN; worldY += MOVE_SPEED; }
        }

        animIdx += direction;
        if (animIdx >= 2 || animIdx <= 0) direction *= -1;
    } else {
        animIdx = 1; 
    }
}

void loop() {
    updateLogic();

    // 1. 画背景网格到底层大画布
    drawSimpleTileMap();

    // 2. 将数组数据灌入宝可梦小画布
    const uint16_t* currentFramePtr = epd_bitmap_allArray[currentState + animIdx];
    pokeSprite.pushImage(0, 0, W, H, currentFramePtr);

    // 计算屏幕中心坐标 (LovyanGFX 默认以中心点为锚点)
    int centerX = 135 / 2;
    int centerY = 240 / 2;
    
    // 如果 isMirrored 为 true，X轴缩放为 -1.0 (即水平翻转)
    float scaleX = isMirrored ? -1.0 : 1.0;
    
    // API: pushRotateZoom(&目标画布, 目标X, 目标Y, 旋转角度, X缩放, Y缩放, 透明色)
    // 这里的 0x0000 就是自动剔除黑色背景
    pokeSprite.pushRotateZoom(&canvas, centerX, centerY, 0.0, scaleX, 1.0, 0xFFFF);

    // 4. 将合成好的整张画面推送到物理屏幕
    canvas.pushSprite(0, 0);

    delay(300); 
}