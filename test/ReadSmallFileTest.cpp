#include <fstream>
#include "Common.hpp"
#include "ReadSmallFileTest.hpp"
#include "../driver/fat16.h"


ReadSmallFileTest::ReadSmallFileTest():
Test("ReadSmallFileTest"),
m_content("Hello, World!")
{
}

void ReadSmallFileTest::init()
{
    restore_image();
    create_small_file("HELLO.TXT", m_content);
    load_image();
}

bool ReadSmallFileTest::run()
{
    if (fat16_init() < 0)
        return false;

    int fd = fat16_open("HELLO.TXT", 'r');
    if (fd < 0)
        return false;

    char buf[20];
    int ret = fat16_read(fd, buf, sizeof(buf));
    if (ret < 0)
        return false;
    if (m_content != std::string(buf))
        return false;

    if (fat16_close(fd) < 0)
        return false;

    return true;
}

void ReadSmallFileTest::create_small_file(const std::string &filename, const std::string &content)
{
    mount_image();

    std::string path = "/mnt/";
    path += filename;
    std::ofstream file(path);
    file << content;
    file.close();

    unmount_image();
}