import sys

def read_binary_file(file_path):
    try:
        with open(file_path, 'rb') as file:  # 以二进制模式读取文件
            data = file.read()  # 读取文件的所有内容
            
            try:
                # 尝试解码为UTF-8文本
                text = data.decode('utf-8')
                print("Decoded text output:")
                print(text)
            except UnicodeDecodeError:
                # 如果解码失败，则以十六进制形式输出
                print("Binary file content (hex):")
                print(data.hex())
    except FileNotFoundError:
        print(f"File not found: {file_path}")
    except IOError as e:
        print(f"Error reading file: {file_path}, {e}")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python script.py <binary_file>")
    else:
        file_path = sys.argv[1]
        read_binary_file(file_path)
