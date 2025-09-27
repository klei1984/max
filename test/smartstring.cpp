/* Copyright (c) 2023 M.A.X. Port Team
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "smartstring.hpp"

#include <gtest/gtest.h>

TEST(SmartStringTest, Ctor) {
    SmartString string1;
    SmartString string2("");
    SmartString string3("Mechanized Assault & Exploration");
    SmartString string4(100);
    SmartString string5(string1);
    SmartString string6(string4);
    SmartString string7(string3);

    EXPECT_STREQ(string1.GetCStr(), "");
    EXPECT_STREQ(string2.GetCStr(), "");
    EXPECT_STREQ(string3.GetCStr(), "Mechanized Assault & Exploration");
    EXPECT_STREQ(string4.GetCStr(), "");
    EXPECT_STREQ(string5.GetCStr(), "");
    EXPECT_STREQ(string6.GetCStr(), "");
    EXPECT_STREQ(string7.GetCStr(), "Mechanized Assault & Exploration");

    EXPECT_EQ(string1.GetLength(), 0);
    EXPECT_EQ(string2.GetLength(), 0);
    EXPECT_EQ(string3.GetLength(), 32);
    EXPECT_EQ(string4.GetLength(), 0);
    EXPECT_EQ(string5.GetLength(), 0);
    EXPECT_EQ(string6.GetLength(), 0);
    EXPECT_EQ(string7.GetLength(), 32);
}

TEST(SmartStringTest, Operators) {
    SmartString string1("Mechanized");
    SmartString string2("Assault");
    SmartString string3("Exploration");
    SmartString string4;

    string4 += string1;
    string4 += " ";
    string4 += string2;
    string4 += " ";
    string4 += '&';
    string4 += ' ';
    string4 += string3;

    EXPECT_STREQ(string4.GetCStr(), "Mechanized Assault & Exploration");
    EXPECT_TRUE(string4.IsEqual("Mechanized Assault & Exploration"));
    EXPECT_EQ(string4.GetLength(), 32);
}

TEST(SmartStringTest, UTF8) {
    SmartString string1(reinterpret_cast<const char*>(u8"Mechanized Assault & Exploration"));
    SmartString string2(reinterpret_cast<const char*>(u8"Gépesített Támadás és Felderítés"));

    EXPECT_EQ(string1.GetLength(), 32);
    EXPECT_EQ(string2.GetLength(), 39);

    string1.Toupper();

    EXPECT_TRUE(string1.IsEqual(reinterpret_cast<const char*>(u8"MECHANIZED ASSAULT & EXPLORATION")));

    /// \todo non single byte UTF8 encoded case conversions are not supported yet, would require ICU library
    // string2.Toupper();
    //
    // EXPECT_TRUE(string2.IsEqual(reinterpret_cast<const char*>(u8"GÉPESÍTETT TÁMADÁS ÉS FELDERÍTÉS")));
}

TEST(SmartStringTest, SubString) {
    SmartString string("Mechanized Assault & Exploration");

    EXPECT_EQ(string[0], 'M');
    EXPECT_EQ(string[31], 'n');
    EXPECT_TRUE(SmartString(string).Substr(10, 10).IsEqual(" Assault &"));
    EXPECT_STREQ(string.Substr(100, 0).GetCStr(), "");
    EXPECT_STREQ(string.Substr(0, 0).GetCStr(), "");
    EXPECT_STREQ(string.Substr(100, 100).GetCStr(), "");
    EXPECT_STREQ(string.Substr(0, 100).GetCStr(), "Mechanized Assault & Exploration");
    EXPECT_STREQ(string.Substr(31, 1).GetCStr(), "n");
    EXPECT_STREQ(string.Substr(32, 1).GetCStr(), "");
    EXPECT_STREQ(string.Substr(33, 1).GetCStr(), "");
    EXPECT_STREQ(string.Substr(1, 31).GetCStr(), "echanized Assault & Exploration");
    EXPECT_STREQ(string.Substr(1, 32).GetCStr(), "echanized Assault & Exploration");
    EXPECT_STREQ(string.Substr(0, 33).GetCStr(), "Mechanized Assault & Exploration");
}

TEST(SmartStringTest, Assign) {
    SmartString string1("Mechanized");
    SmartString string2("Assault");
    SmartString string3("Exploration");
    SmartString string4("M.A.X.");

    string2 = string1;
    string1 = string3;
    string3 = string2;
    string4 = SmartString("Carlton L.");

    EXPECT_STREQ(string1.GetCStr(), "Exploration");
    EXPECT_STREQ(string2.GetCStr(), "Mechanized");
    EXPECT_STREQ(string3.GetCStr(), "Mechanized");
    EXPECT_STREQ(string4.GetCStr(), "Carlton L.");
}

TEST(SmartStringTest, Assign2) {
    SmartString string1;
    SmartString string2;
    SmartString string3;
    SmartString string4;

    string2 = "M.";
    string1 = "A.";
    string3 = "";
    string4 = "Carlton L.";

    EXPECT_TRUE(string1.IsEqual("A."));
    EXPECT_TRUE(string2.IsEqual("M."));
    EXPECT_TRUE(string3.IsEqual(""));
    EXPECT_TRUE(string4.IsEqual("Carlton L."));
}

TEST(SmartStringTest, StrStr) {
    SmartString string1("Mechanized");
    SmartString string2("Assault");
    SmartString string3("Exploration");
    SmartString string4("M.A.X.");
    SmartString string5("lorat");
    SmartString string6("zed");
    SmartString string7("Ass");
    SmartString string8;

    EXPECT_STREQ(string3.StrStr(string5), "loration");
    EXPECT_STREQ(string1.StrStr(string6), "zed");
    EXPECT_STREQ(string2.StrStr(string7), "Assault");
    EXPECT_STREQ(string1.StrStr(string8), "Mechanized");
    EXPECT_EQ(string1.StrStr(string7), nullptr);

    EXPECT_STREQ(string3.StrStr("rat"), "ration");
    EXPECT_STREQ(string1.StrStr("zed"), "zed");
    EXPECT_STREQ(string2.StrStr("Ass"), "Assault");
    EXPECT_STREQ(string4.StrStr(""), "M.A.X.");
    EXPECT_EQ(string1.StrStr("M.A.X."), nullptr);
}

TEST(SmartStringTest, AsciiMisc) {
    SmartString string1("Mechanized Assault & Exploration");
    SmartString string2;
    SmartString string3;
    SmartString string4;

    string2 = string1.Toupper();
    string3 = string1.Tolower();

    EXPECT_STREQ(string2.GetCStr(), "MECHANIZED ASSAULT & EXPLORATION");
    EXPECT_EQ(string2.Strcmp("MECHANIZED ASSAULT & EXPLORATION"), 0);
    EXPECT_EQ(string2.Strcmp("mechanized assault & exploration", false), 0);

    EXPECT_STREQ(string3.GetCStr(), "mechanized assault & exploration");
    EXPECT_EQ(string2.Strcmp("MECHANIZED ASSAULT & exploration", false), 0);

    string1.Tolower().Toupper().Clear().Tolower().Toupper();

    EXPECT_STREQ(string1.GetCStr(), "");
    EXPECT_EQ(string1.GetLength(), 0);

    string4.Clear();

    EXPECT_STREQ(string4.GetCStr(), "");
    EXPECT_EQ(string4.GetLength(), 0);
}

TEST(SmartStringTest, Sprintf) {
    SmartString string1;
    SmartString string2;
    SmartString string3;
    SmartString string4;
    SmartString string5;
    SmartString string6;
    SmartString string7;
    SmartString string8;

    string1.Sprintf(0, "%s %s & %s", "Mechanized", "Assault", "Exploration");
    string2.Sprintf(4, "%s %s & %s", "Mechanized", "Assault", "Exploration");
    string3.Sprintf(5, "%s %s & %s", "Mechanized", "Assault", "Exploration");
    string4.Sprintf(6, "%s %s & %s", "Mechanized", "Assault", "Exploration");
    string5.Sprintf(31, "%s %s & %s", "Mechanized", "Assault", "Exploration");
    string6.Sprintf(32, "%s %s & %s", "Mechanized", "Assault", "Exploration");
    string7.Sprintf(33, "%s %s & %s", "Mechanized", "Assault", "Exploration");
    string8.Sprintf(10, "");

    EXPECT_TRUE(string1.IsEqual(""));
    EXPECT_TRUE(string2.IsEqual("Mec"));
    EXPECT_TRUE(string3.IsEqual("Mech"));
    EXPECT_TRUE(string4.IsEqual("Mecha"));
    EXPECT_TRUE(string5.IsEqual("Mechanized Assault & Explorati"));
    EXPECT_TRUE(string6.IsEqual("Mechanized Assault & Exploratio"));
    EXPECT_TRUE(string7.IsEqual("Mechanized Assault & Exploration"));
    EXPECT_TRUE(string8.IsEqual(""));
}

TEST(SmartStringTest, Resize) {
    SmartString string1(static_cast<size_t>(0));
    SmartString string2(5);

    string1 += 'A';
    string2 += "M.A.X.";
    string2 += SmartString(": Mechanized Assault & Exploration");

    EXPECT_STREQ(string1.GetCStr(), "A");
    EXPECT_STREQ(string2.GetCStr(), "M.A.X.: Mechanized Assault & Exploration");
}
