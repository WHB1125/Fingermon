import os, sys, io
import M5
from M5 import *
import time  # 引入 time 模块来控制动画速度

# --- 全局变量配置 ---
frames = [
    "res/img/sprite_001.png",
    "res/img/sprite_002.png",
    "res/img/sprite_003.png"
]
current_frame = 0
direction = 1  # 新增：1 表示正向播放，-1 表示反向播放
poke_image = None  # 提前声明一个图片组件变量


def setup():
    global poke_image
    M5.begin()

    Widgets.setRotation(0)  # 0是竖屏

    # 屏幕刷白
    Widgets.fillScreen(0xFFFFFF)

    # 初始化图片组件
    # 注意：如果图片放大后，这里的 X/Y 坐标可能需要微调才能保持居中
    poke_image = Widgets.Image(frames[0], 25, 115)


def loop():
    global current_frame, direction, poke_image
    M5.update()

    # 1. 替换当前显示的图片
    poke_image.setImage(frames[current_frame])

    # 2. 帧数递增或递减（应用方向）
    current_frame += direction

    # 3. 检查边界并反转方向
    if current_frame >= len(frames) - 1:
        # 如果达到了最后一张 (索引2)，强制停在最后一张，并让下一步反向播放
        current_frame = len(frames) - 1
        direction = -1
    elif current_frame <= 0:
        # 如果退回到了第一张 (索引0)，强制停在第一张，并让下一步正向播放
        current_frame = 0
        direction = 1

    # 4. 控制动画速度 (500 毫秒)
    time.sleep_ms(300)


if __name__ == '__main__':
    try:
        setup()
        while True:
            loop()
    except (Exception, KeyboardInterrupt) as e:
        try:
            from utility import print_error_msg

            print_error_msg(e)
        except ImportError:
            print("please update to latest firmware")