#include <cstdlib>
#include <fstream>
#include "Common.hpp"
#include "ReadLargeFileTest.hpp"
#include "../driver/fat16.h"
#include "linux_hal.h"


ReadLargeFileTest::ReadLargeFileTest(unsigned int bytes_count):
Test(std::string("ReadLargeFileTest (") + std::to_string(bytes_count) + std::string(")")),
m_bytes_count(bytes_count)
{
}

void ReadLargeFileTest::init()
{
    restore_image();
    mount_image();
    create_large_file("HELLO.TXT");
    system("mkdir /mnt/TMP");
    create_large_file("/TMP/HELLO.TXT");
    unmount_image();
    load_image();
}

bool ReadLargeFileTest::run()
{
    if (fat16_init(linux_dev) < 0)
        return false;

    {
        int fd = fat16_open("HELLO.TXT", 'r');
        if (fd < 0)
            return false;

        unsigned int bytes_read_count = 0;
        srand(2);
        while (bytes_read_count < m_bytes_count) {
            char expected = rand();
            char buf = 0;
            if (fat16_read(fd, &buf, sizeof(buf)) != 1)
               return false;

            if (buf != expected) {
                printf("Found %02x but expected %02X\n", buf, expected);
                return false;
            }

            ++bytes_read_count;
        }

        /* Check if we reached end of file */
        char buf = 0;
        if (fat16_read(fd, &buf, sizeof(buf)) != 0)
            return false;

        if (fat16_close(fd) < 0)
            return false;
    }

    {
        int fd = fat16_open("/TMP/HELLO.TXT", 'r');
        if (fd < 0)
            return false;

        unsigned int bytes_read_count = 0;
        srand(2);
        while (bytes_read_count < m_bytes_count) {
            char expected = rand();
            char buf = 0;
            if (fat16_read(fd, &buf, sizeof(buf)) != 1)
               return false;

            if (buf != expected) {
                printf("Found %02x but expected %02X\n", buf, expected);
                return false;
            }

            ++bytes_read_count;
        }

        /* Check if we reached end of file */
        char buf = 0;
        if (fat16_read(fd, &buf, sizeof(buf)) != 0)
            return false;

        if (fat16_close(fd) < 0)
            return false;
    }

    return true;
}

void ReadLargeFileTest::create_large_file(const std::string &filename)
{
    std::string path = "/mnt/";
    path += filename;
    std::ofstream file(path);
    srand(2);
    for (unsigned int i = 0; i < m_bytes_count; ++i)
        file.put(rand());
    file.close();
}
