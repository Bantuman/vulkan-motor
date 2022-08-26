from PIL import Image

baseImage = Image.open("studs_normal.png")
outImage = Image.new(mode='RGB', size=(baseImage.width, baseImage.height))

for x in range(0, baseImage.width):
    for y in range(0, baseImage.height):
        p = baseImage.getpixel((x, y))
        outImage.putpixel((x, y), (p[1], p[0], p[2]))

outImage.save("studs_normal_new.png")