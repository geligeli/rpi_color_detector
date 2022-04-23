#!/usr/bin/env python3

import argparse
import glob
import itertools
import os
import shutil

import tensorflow as tf

AUTOTUNE = tf.data.AUTOTUNE


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
        basedir = os.path.join(self.IMAGES_BASE_DIR, dataset)
        classes = [(x, os.path.join(basedir, x)) for x in os.listdir(basedir)]
        classes = filter(lambda x: os.path.isdir(x[1]), classes)
        r = {}
        for c in classes:
            r[c[0]] = [x[len(self.IMAGES_BASE_DIR)+1:]
                       for x in glob.glob(c[1] + '/*.jpg')]
        return r

    def filePath(self, image_path):
        return self.safePath(os.path.join(self.IMAGES_BASE_DIR, image_path))

    def directoryPathForDataset(self, datasetName):
        return self.safePath(os.path.join(self.IMAGES_BASE_DIR, datasetName))

    def relabelImage(self, sourceImage, targetClass):
        dataset, _, base_name = sourceImage.split('/')
        src = self.filePath(sourceImage)
        dst = self.filePath(os.path.join(dataset, targetClass, base_name))
        print(src, dst)
        shutil.move(src, dst)

    def labelImageList(self, image_list, model, label_names):
        def ExtractRoiForPrediction(image):
            x = 0.3
            target_width = int(640*x)
            target_height = int(480*x)
            offset_width = (640-target_width)//2
            offset_height = (480-target_height)//2
            cropped = tf.image.crop_to_bounding_box(
                image, offset_height, offset_width, target_height, target_width)
            return tf.cast(cropped, tf.float32)

        def LoadImage(filename):
            image = tf.io.read_file(filename)
            image = tf.io.decode_jpeg(image)
            image.set_shape([480, 640, 3])
            return image

        label_names = tf.constant(label_names)
        training_data = tf.data.Dataset.from_generator(
            lambda: image_list, output_signature=tf.TensorSpec(shape=(), dtype=tf.string))
        training_data = training_data.map(
            LoadImage, deterministic=True, num_parallel_calls=AUTOTUNE)
        training_data = training_data.map(
            ExtractRoiForPrediction, deterministic=True, num_parallel_calls=AUTOTUNE)
        training_data = training_data.batch(16)
        training_data = training_data.map(
            model, deterministic=True, num_parallel_calls=AUTOTUNE)
        training_data = training_data.map(
            lambda x: tf.gather(label_names, tf.argmax(x, axis=1)))
        for batch in training_data:
            for c in batch:
                yield c.numpy().decode('utf-8')

    def validLablesForDataset(self, dataset):
        if dataset not in os.listdir(self.IMAGES_BASE_DIR):
            return None
        basedir = os.path.join(self.IMAGES_BASE_DIR, dataset)
        classes = [(x, os.path.join(basedir, x)) for x in os.listdir(basedir)]
        classes = filter(lambda x: os.path.isdir(x[1]), classes)
        return sorted([x[0] for x in classes if x[0] not in ['_garbage', 'unclassified']])

    def classifyImagesAndCreateDatasetFolter(self, src_images, model, label_names, new_dataset_name):
        datasetDir = self.directoryPathForDataset(new_dataset_name)
        assert not os.path.exists(datasetDir)
        os.mkdir(datasetDir)
        for l in set(label_names + ['_gargabe', 'unclassified']):
            print(l)
            os.mkdir(os.path.join(datasetDir, l))
        for img, label in zip(src_images, self.labelImageList(src_images, model, label_names)):
            src = os.path.realpath(img)
            srcBasename = os.path.basename(src)
            dst = os.path.join(datasetDir, label, srcBasename)
            os.symlink(src, dst)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description='classifyImagesAndCreateDatasetFolter args')
    parser.add_argument('-src_dataset')
    parser.add_argument('-model')
    parser.add_argument('-new_dataset_name')
    args = parser.parse_args()
    print(args)
    ip = ImageProvider()
    src_images = ip.listImages(args.src_dataset)
    label_names = ip.validLablesForDataset(args.src_dataset)
    print(label_names)
    model = tf.saved_model.load(args.model)
    src_images = [ip.filePath(x) for x in itertools.chain(*src_images.values())]
    ip.classifyImagesAndCreateDatasetFolter(src_images, model, label_names, args.new_dataset_name)
