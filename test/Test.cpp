/*
 * Copyright (C) 2017  Francois Berder
 *
 * This file is part of fat16.
 *
 * fat16 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * fat16 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with fat16.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdexcept>
#include "Common.hpp"
#include "Test.hpp"


Test::Test(const std::string &name):
m_name(name)
{
}

void Test::init()
{
    restore_image();
    load_image();
}

void Test::release()
{
    release_image();
    restore_image();
}

const std::string Test::get_name() const
{
    return m_name;
}
