#!/usr/bin/env python3

import ctypes
import os.path
import sys
import subprocess

# Load FAT16 library
dll_name = "libfat16_driver.so"
dllabspath = os.path.dirname(os.path.abspath(__file__)) + os.path.sep + dll_name
_LIB = ctypes.CDLL(dllabspath)

test_records = {}

def record_test_result(test_name, result):
    if not result:
        test_records[test_name] = result
    elif not (test_name in test_records.keys()):
        test_records[test_name] = result

def restore_image(image_path):
    subprocess.run(['git', 'checkout' , image_path])

def unmount_image():
    subprocess.run(['umount', '/mnt'])

def mount_image(image_path):
    subprocess.run(['mount', image_path, '/mnt'])

def delete_file(filename):
    subprocess.run(['rm', '-f', '/mnt/' + filename])

def create_empty_file(image_path, filename):
    mount_image(image_path)
    delete_file(filename)
    subprocess.run(['touch', '/mnt/' + filename])
    unmount_image()

def create_small_file(image_path, filename, content):
    mount_image(image_path)
    small_file = open('/mnt/' + filename, 'w')
    small_file.write(content)
    small_file.close()
    unmount_image()

def check_small_file_content(image_path, filename, expected_content):
    mount_image(image_path)
    content = subprocess.check_output(['cat', '/mnt/' + filename])
    content = content.decode()
    unmount_image()
    return content == expected_content

def check_big_file_content(image_path, filename):
    return True

def test_init(image_path):
    restore_image(image_path)
    _LIB.linux_load_image(str.encode(image_path))
    print('----- init -----')
    ret = _LIB.fat16_init()
    record_test_result('init', ret == 0)
    _LIB.linux_release_image()

def test_read_empty_file(image_path):
    restore_image(image_path)
    create_empty_file(image_path, 'HELLO.TXT')
    _LIB.linux_load_image(str.encode(image_path))
    print('----- read empty file -----')
    mode = (ctypes.c_char)(str.encode('r'))
    fd = _LIB.fat16_open(str.encode('HELLO.TXT'), mode)
    if fd < 0:
        record_test_result('read_empty_file', False)

    buf = (ctypes.c_uint8 * 1)()
    ret = _LIB.fat16_read(fd, buf, 1)
    if ret != -9:
        record_test_result('read_empty_file', False)
    ret = _LIB.fat16_close(fd)
    _LIB.linux_release_image()
    record_test_result('read_empty_file', ret == 0)

def test_read_small_file(image_path):
    restore_image(image_path)
    create_small_file(image_path, 'HELLO.TXT', 'Hello World')
    _LIB.linux_load_image(str.encode(image_path))
    print('----- read small file -----')

    mode = (ctypes.c_char)(str.encode('r'))
    fd = _LIB.fat16_open(str.encode('HELLO.TXT'), mode)
    if fd < 0:
        record_test_result('read_small_file', False)

    buf = (ctypes.c_uint8 * 20)()
    ret = _LIB.fat16_read(fd, buf, 20)
    if ret != 11:
        record_test_result('read_small_file', False)

    if "".join(map(chr, buf))[0:11] != 'Hello World':
        record_test_result('read_small_file', False)

    ret = _LIB.fat16_close(fd)
    record_test_result('read_small_file', ret == 0)

    _LIB.linux_release_image()

def test_write_small_file(image_path):
    restore_image(image_path)
    _LIB.linux_load_image(str.encode(image_path))
    print('----- write small file -----')
    mode = (ctypes.c_char)(str.encode('w'))
    fd = _LIB.fat16_open(str.encode('HELLO.TXT'), mode)
    if fd < 0:
        record_test_result('write_small_file', False)

    content = 'This is a test'
    buf = (ctypes.c_uint8 * len(content))(*content.encode('utf-8'))
    ret = _LIB.fat16_write(fd, buf, len(content))

    # ret should be equal to len(content) because we assume there will be enough space
    # left
    if ret != len(content):
        record_test_result('write_small_file', False)

    if not check_small_file_content(image_path, 'HELLO.TXT', content):
        record_test_result('write_small_file', False)

    ret = _LIB.fat16_close(fd)
    record_test_result('write_small_file', ret == 0)
    _LIB.linux_release_image()

def test_write_big_file(image_path):
    restore_image(image_path)
    _LIB.linux_load_image(str.encode(image_path))
    print('----- write big file -----')
    mode = (ctypes.c_char)(str.encode('w'))
    fd = _LIB.fat16_open(str.encode('BIG.TXT'), mode)
    if fd < 0:
        record_test_result('write_big_file', False)

    # Write 256KB
    for i in range(256*1024):
        data = [i % 256]
        buf = (ctypes.c_uint8 * len(data))(*data)
        ret = _LIB.fat16_write(fd, buf, len(data))
        if ret != len(data):
            record_test_result('write_big_file', False)

    if not check_big_file_content(image_path, 'BIG.TXT'):
        record_test_result('write_big_file', False)

    ret = _LIB.fat16_close(fd)
    record_test_result('write_big_file', ret == 0)
    _LIB.linux_release_image()

def test_write_erase_file_content(image_path):
    restore_image(image_path)
    create_small_file(image_path, 'HELLO.TXT', 'Hello World')
    _LIB.linux_load_image(str.encode(image_path))
    print('----- write erase file content -----')
    mode = (ctypes.c_char)(str.encode('w'))
    fd = _LIB.fat16_open(str.encode('HELLO.TXT'), mode)
    if fd < 0:
        record_test_result('write_erase_file_content', False)
    ret = _LIB.fat16_close(fd)
    record_test_result('write_erase_file_content', ret == 0)

    mount_image(image_path)
    content = subprocess.check_output(['cat', '/mnt/HELLO.TXT'])
    record_test_result('write_erase_file_content', 0 == len(content))
    unmount_image()
    _LIB.linux_release_image()

def main(argv):
    image_path = 'data/fs.img'
    if len(argv) > 1:
        image_path = argv[1]

    # Perform tests
    test_init(image_path)
    test_read_empty_file(image_path)
    test_read_small_file(image_path)
    test_write_small_file(image_path)
    test_write_big_file(image_path)
    test_write_erase_file_content(image_path)

    # Print test results
    print('\n\n\n##### TEST RESULTS #####')
    for test_name, test_result in test_records.items():
        print('{}: {}'.format(test_name, 'PASS' if test_result else 'FAIL'))

if __name__ == "__main__":
    main(sys.argv)
