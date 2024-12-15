from image_io_utility import *


if __name__ == "__main__":
	l = LoadYUV("single_image_1920x1080.yuv", (1920, 1080))
	ret, image = l.read()
