import glob
import os
import shutil


class ImageProvider():
    def __init__(self):
        self.IMAGES_BASE_DIR = '/nfs/general/shared/images'

    def safePath(self, p):
        p = os.path.normpath(p)
        if os.path.commonprefix((p, self.IMAGES_BASE_DIR)) != self.IMAGES_BASE_DIR:
            return None
        return p

    def listImages(self, dataset):
        if dataset not in os.listdir(self.IMAGES_BASE_DIR):
            return {}
        basedir=os.path.join(self.IMAGES_BASE_DIR, dataset)
        classes = [(x, os.path.join(basedir, x)) for x in os.listdir(basedir)]
        classes = filter(lambda x: os.path.isdir(x[1]), classes)
        r = {}
        for c in classes:
            r[c[0]] = [x[len(self.IMAGES_BASE_DIR)+1:] for x in  glob.glob(c[1] + '/*.jpg')]
        return r

    def filePath(self, image_path):
        return self.safePath(os.path.join(self.IMAGES_BASE_DIR, image_path))

    def makePath(self, className, imageBaseName):
        return self.safePath(os.path.join(self.IMAGES_BASE_DIR, className, imageBaseName))

    def relabelImage(self, sourceImage, targetClass):
        src = self.filePath(sourceImage)
        dst = self.makePath(targetClass, os.path.basename(sourceImage))
        shutil.move(src, dst)

