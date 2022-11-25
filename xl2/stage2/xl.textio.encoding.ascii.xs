// *****************************************************************************
// xl.textio.encoding.ascii.xs                                        XL project
// *****************************************************************************
//
// File description:
//
//      Basic module describing the ASCII encoding
//
//
//
//
//
//
//
//
// *****************************************************************************
// This software is licensed under the GNU General Public License v3+
// (C) 2003-2004,2007,2009-2010,2015-2017, Christophe de Dinechin <christophe@dinechin.org>
// *****************************************************************************
// This file is part of XL
//
// XL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
//
// XL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with XL, in a file named COPYING.
// If not, see <https://www.gnu.org/licenses/>.
// *****************************************************************************

module XL.TEXT_IO.ENCODING.ASCII with

    function to_lower(C : character) return character
    function is_space(C : character) return boolean
    function is_line_break(C : character) return boolean
    function is_digit(C : character) return boolean
    function is_letter(C : character) return boolean
    function is_nul(C : character) return boolean