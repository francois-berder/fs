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


#ifndef _WRITEERASECONTENTTEST_HPP_
#define _WRITEERASECONTENTTEST_HPP_

#include "Test.hpp"

class WriteEraseContentTest : public Test
{
    public :

        WriteEraseContentTest();

        virtual void init() override;
        virtual bool run() override;

    private :

        bool check_file_is_empty(const std::string &filepath);
};

#endif
