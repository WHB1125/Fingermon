from PIL import Image
import os

# --- 配置区 ---
input_root = "output_sprites"  # 你之前裁剪出来的根文件夹
output_root = "scaled_sprites"  # 放大后的保存根文件夹
scale_factor = 3  # 放大倍数（例如放大 3 倍）

# 确保输出的根文件夹存在
if not os.path.exists(output_root):
    os.makedirs(output_root)

# 1. 遍历 input_root 下的所有内容 (即那些 1_Hurt, 2_Idle 等分类文件夹)
for folder_name in os.listdir(input_root):
    folder_path = os.path.join(input_root, folder_name)

    # 2. 判断当前项是不是一个文件夹
    if os.path.isdir(folder_path):
        # 在输出目录中，创建一模一样的子文件夹名
        out_subfolder = os.path.join(output_root, folder_name)
        os.makedirs(out_subfolder, exist_ok=True)

        # 3. 遍历这个子文件夹里的所有图片
        for filename in os.listdir(folder_path):
            if filename.endswith(".png"):
                img_path = os.path.join(folder_path, filename)
                img = Image.open(img_path)

                # 计算新尺寸并使用最邻近插值法 (NEAREST) 无损放大
                new_size = (img.width * scale_factor, img.height * scale_factor)
                scaled_img = img.resize(new_size, Image.NEAREST)

                # 拼装最终保存路径，存入对应的子文件夹
                save_path = os.path.join(out_subfolder, filename)
                scaled_img.save(save_path)

                print(f"✅ 已放大: [{folder_name}] -> {filename}")

print("\n🎉 全部放大完成！连同文件夹结构已经完美复刻到了 scaled_sprites 目录中。")