#!/usr/bin/env python3
import os
os.environ["CUDA_VISIBLE_DEVICES"] = "-1"

import sys
import time
import tensorflow as tf

all_dataset = tf.data.Dataset.list_files(
    "/nfs/general/shared/Key*/*.jpg").shuffle(buffer_size=10000)

keys_tensor = tf.constant(['KeyA', 'KeyD'])
vals_tensor = tf.constant([0, 1])
init = tf.lookup.KeyValueTensorInitializer(keys_tensor, vals_tensor)
table = tf.lookup.StaticHashTable(init, default_value=-1)


def CreateImageAndLabel(filename):
    image = tf.io.read_file(filename)
    image = tf.io.decode_and_crop_jpeg(image, crop_window=tf.constant(
        [220, 260, 40, 80], dtype=tf.int32), channels=3)
    image = tf.image.convert_image_dtype(image, tf.float32)
    labelString = tf.strings.split(filename, '/')[4]
    # label = tf.one_hot(table.lookup(labelString),2)
    return image, table.lookup(labelString)


data_augmentation = tf.keras.Sequential(
    [
        tf.keras.layers.RandomRotation(0.1),
        tf.keras.layers.RandomTranslation(0.1, 0.1)
    ]
)

AUTOTUNE = tf.data.AUTOTUNE

num_parallel_calls = 64  # tf.data.AUTOTUNE

def benchmark(dataset, num_epochs=5):
    start_time = time.perf_counter()
    for epoch_num in range(num_epochs):
        epoch_size = 128
        epoch_start = time.perf_counter()
        i = 0
        for _ in dataset:
            i += 1
            if i > epoch_size:
                break
            pass
        epoch_time = time.perf_counter() - epoch_start
        print("epoch time:", epoch_time, epoch_size/epoch_time)
        sys.stdout.flush()
    print("Execution time:", time.perf_counter() - start_time)
    sys.stdout.flush()

for num_parallel_calls in [1, 2, 4, 8, 16, 32, 64, 128]:
    train_dataset = all_dataset.repeat().map(CreateImageAndLabel, num_parallel_calls=num_parallel_calls,
                                             deterministic=False).batch(
        16).map(lambda x, y: (data_augmentation(x), y), num_parallel_calls=num_parallel_calls,
                deterministic=False)

    print(num_parallel_calls)
    benchmark(train_dataset)
