/*
 * Copyright (C) 2025 Tencent.
 * BQLOG is licensed under the Apache License, Version 2.0.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */
#include "test_log.h"
#include <thread>
#include <atomic>
#include <cstdio>
#include <random>
#include <locale.h>
#include <math.h>
#include "bq_common/bq_common.h"

namespace bq {
    namespace test {
        class test_force_flush_callback {
        public:
            static test_result* test_result_ptr;
            static uint64_t sync_log_id;
            static uint64_t async_log_id;
            static bq::platform::atomic<uint64_t> idx;
            static bq::platform::atomic<uint64_t> current_idx_sync;
            static bq::platform::atomic<uint64_t> current_idx_async;
            static bool multi_thread_test_end;
            static void console_callback(uint64_t log_id, int32_t category_idx, bq::log_level log_level, const char* content, int32_t length)
            {
                (void)category_idx;
                (void)log_level;
                (void)length;
                if (log_id != sync_log_id && log_id != async_log_id) {
                    return;
                }
                bq::platform::atomic<uint64_t>& ref_idx = (log_id == sync_log_id) ? current_idx_sync : current_idx_async;
                ref_idx.fetch_add_seq_cst(1);
                char idx_tmp[32];
                snprintf(idx_tmp, sizeof(idx_tmp), "%" PRIu64, ref_idx.load_seq_cst());
                bq::string standard_end_str = (log_id == sync_log_id) ? (bq::string("force flush test sync log ") + idx_tmp) : (bq::string("force flush test async log ") + idx_tmp);
                bq::string log_content(content, static_cast<size_t>(length));
                test_result_ptr->add_result(log_content.end_with(standard_end_str), (log_id == sync_log_id) ? "force flush test sync" : "force flush test async");
            }
        };
        test_result* test_force_flush_callback::test_result_ptr = nullptr;
        uint64_t test_force_flush_callback::sync_log_id = 0;
        uint64_t test_force_flush_callback::async_log_id = 0;
        bq::platform::atomic<uint64_t> test_force_flush_callback::idx = 0;
        bq::platform::atomic<uint64_t> test_force_flush_callback::current_idx_sync = 0;
        bq::platform::atomic<uint64_t> test_force_flush_callback::current_idx_async = 0;
        bool test_force_flush_callback::multi_thread_test_end = false;

        void test_log::test_2(test_result& result, const test_category_log& log_inst)
        {
            result.add_result(log_inst.get_name() == "test_log", "log name test");

            {
                bq::string empty_str;
                bq::string full_str = "123";
                log_inst.fatal(log_inst.cat.ModuleA.SystemA.ClassA, "Empty Str Test {}, {}", empty_str, full_str);
                result.add_result(log_str.end_with("[F]\t[ModuleA.SystemA.ClassA]\tEmpty Str Test , 123"), "log update 1");
            }

            {
                log_inst.fatal(log_inst.cat.ModuleA.SystemA, "connect {}:{}");
                result.add_result(log_str.end_with("[F]\t[ModuleA.SystemA.ClassA]\tEmpty Str Test , 123"), "log update 2");
            }

            {
                bq::string empty_str;
                bq::string full_str = "123";
                log_inst.warning(log_inst.cat.ModuleA.SystemA.ClassA, "Empty Str Test {}, {}", empty_str.c_str(), full_str.c_str());
                result.add_result(log_str.end_with("[F]\t[ModuleA.SystemA.ClassA]\tEmpty Str Test , 123"), "log update 3");
            }

            {
                bq::string ip = "9.134.131.77";
                uint16_t port = 18900;
                log_inst.fatal(log_inst.cat.ModuleA.SystemA.ClassA, "connect {{}:{}}", ip, port);
                result.add_result(log_str.end_with("[F]\t[ModuleA.SystemA.ClassA]\tconnect {9.134.131.77:18900}"), "brace test 1");
                log_inst.fatal(log_inst.cat.ModuleA.SystemA.ClassA, "connect {{}:{}}");
                result.add_result(log_str.end_with("[F]\t[ModuleA.SystemA.ClassA]\tconnect {{}:{}}"), "brace test 2");
            }

            {
                int32_t* pointer = NULL;
                log_inst.error(log_inst.cat.ModuleB, "NULL Pointer Str Test {}, {}", pointer, pointer);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\tNULL Pointer Str Test null, null"), "log update 4");
            }
            {
                int64_t i { 12 };
                log_inst.error(log_inst.cat.ModuleB, "|{:+10d}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|       +12|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:10b}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|      1100|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:#10b}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|    0b1100|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:#10B}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|    0B1100|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:10X}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|         C|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:#10X}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|       0XC|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:<10d}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|12        |"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:>010d}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|        12|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:<010d}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|12        |"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:#010x}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|0x0000000c|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:<#010x}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|0xc       |"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:>#010x}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|       0xc|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:06d}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|000012|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:^06d}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|  12  |"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:+06d}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|+00012|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:<+06d}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|+12   |"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:+06d}|", -i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|-00012|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:<+06d}|", -i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|-12   |"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:06X}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|00000C|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:#06X}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|0X000C|"), "layout format");
                // Centre alignment puts the odd fill character after the value, like std::format.
                log_inst.error(log_inst.cat.ModuleB, "|{:^7d}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|  12   |"), "layout format centre alignment");
                log_inst.error(log_inst.cat.ModuleB, "|{:*^7}|", -1);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|**-1***|"), "layout format centre alignment with fill");
                log_inst.error(log_inst.cat.ModuleB, "|{:^8}|", "hello");
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t| hello  |"), "layout format centre alignment of a string");
                // The alternate form of octal starts with a 0, unless the value is 0.
                log_inst.error(log_inst.cat.ModuleB, "|{:#o}|{:#o}|{:#o}|{:#o}|", 8, -42, 0, static_cast<uint64_t>(UINT64_MAX));
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|010|-052|0|01777777777777777777777|"), "layout format octal alternate form");
                log_inst.error(log_inst.cat.ModuleB, "|{:#6o}|{:#06o}|", 8, 8);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|   010|000010|"), "layout format octal alternate form with width");
                // Zero padding goes after the sign and the 0x/0b prefix; any other fill goes in front of both.
                log_inst.error(log_inst.cat.ModuleB, "|{:#010x}|{:+#010x}|{:#010b}|{:#010X}|", -42, 42, -5, static_cast<int64_t>(-255));
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|-0x000002a|+0x000002a|-0b0000101|-0X00000FF|"), "layout format zero padding after sign and prefix");
                log_inst.error(log_inst.cat.ModuleB, "|{:*>8}|{:*>+8}|{:*>#8x}|", -42, 42, -42);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|*****-42|*****+42|***-0x2a|"), "layout format fill in front of sign and prefix");
                log_inst.error(log_inst.cat.ModuleB, "|{:015.3e}|", 1500.0);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|0000001.500e+03|"), "layout format zero padding with an exponent sign");
                // A space sign puts a space in front of non-negative values; a space followed by an alignment is a fill.
                log_inst.error(log_inst.cat.ModuleB, "|{: d}|{: d}|{: d}|{: d}|", 12, -12, 0, static_cast<uint32_t>(7));
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t| 12|-12| 0| 7|"), "layout format space sign");
                log_inst.error(log_inst.cat.ModuleB, "|{: 6d}|{: 06d}|{: x}|{: #x}|", 12, 12, 255, 255);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|    12| 00012| ff| 0xff|"), "layout format space sign with width and prefix");
                log_inst.error(log_inst.cat.ModuleB, "|{: >6}|{: <6}|", 12, 12);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|    12|12    |"), "layout format space fill");
                // + on an unsigned 0 is still written.
                log_inst.error(log_inst.cat.ModuleB, "|{:+d}|{:+d}|", static_cast<uint32_t>(0), static_cast<uint64_t>(0));
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|+0|+0|"), "layout format plus sign of unsigned zero");
                // The 0 flag pads with zeros only without an alignment; a 0 before an alignment is a fill character.
                log_inst.error(log_inst.cat.ModuleB, "|{:>010d}|{:^010d}|{:0>10}|{:0>8}|", 12, 12, 12, -42);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|        12|    12    |0000000012|00000-42|"), "layout format 0 flag with an alignment");
                log_inst.error(log_inst.cat.ModuleB, "|{:+>8}|{:->8}|", 42, 42);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|++++++42|------42|"), "layout format sign character as fill");
                // The 0 flag does not pad infinity and NaN with zeros.
                log_inst.error(log_inst.cat.ModuleB, "|{:010.2f}|{:010.2f}|{:010.2f}|{:+08}|", static_cast<double>(NAN), static_cast<double>(INFINITY), -static_cast<double>(INFINITY), static_cast<double>(INFINITY));
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|       nan|       inf|      -inf|    +inf|"), "layout format 0 flag with infinity and nan");
                // The width is a minimum: it never drops digits of the precision.
                log_inst.error(log_inst.cat.ModuleB, "|{:6.3f}|{:3.1f}|{:10.2f}|", 123.456, 1234.5, 1e20);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|123.456|1234.5|100000000000000000000.00|"), "layout format width does not cut the precision");
                // f and F default to 6 digits after the point; F writes INF and NAN in capitals.
                log_inst.error(log_inst.cat.ModuleB, "|{:f}|{:F}|{:f}|", 1.5, 1.5, 1.5f);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|1.500000|1.500000|1.500000|"), "layout format f default precision");
                log_inst.error(log_inst.cat.ModuleB, "|{:F}|{:F}|{:.2F}|{:f}|", static_cast<double>(INFINITY), static_cast<double>(NAN), -static_cast<double>(INFINITY), static_cast<double>(INFINITY));
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|INF|NAN|-INF|inf|"), "layout format F infinity and nan");
                // g and G are the general format, and so is a precision without a type.
                log_inst.error(log_inst.cat.ModuleB, "|{:g}|{:g}|{:g}|{:G}|{:g}|", 1234567.891, 0.0001, 100.0, 1e-05, -2.5);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|1.23457e+06|0.0001|100|1E-05|-2.5|"), "layout format g");
                log_inst.error(log_inst.cat.ModuleB, "|{:.3g}|{:.3}|{:.3}|{:.0g}|{:10.3g}|{:.3}|", 2.675, 2.675, 1234567.891, 2.5, 2.675, 1.5f);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|2.67|2.67|1.23e+06|2|      2.67|1.5|"), "layout format g precision");
                log_inst.error(log_inst.cat.ModuleB, "|{:G}|{:g}|", static_cast<double>(INFINITY), static_cast<double>(NAN));
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|INF|nan|"), "layout format g infinity and nan");
                // a and A are the hexadecimal format: no 0x prefix, shortest digits unless a precision is given.
                log_inst.error(log_inst.cat.ModuleB, "|{:a}|{:a}|{:a}|{:a}|{:A}|{:a}|", 1.5, 0.0, -0.5, 0.1, 0.1, 1e300);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|1.8p+0|0p+0|-1p-1|1.999999999999ap-4|1.999999999999AP-4|1.7e43c8800759cp+996|"), "layout format a");
                log_inst.error(log_inst.cat.ModuleB, "|{:.2a}|{:.0a}|{:.0a}|{:.1a}|{:.3a}|{:.20a}|", 0.1, 1.5, 1.75, 1.96875, 1.0, 1.5);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|1.9ap-4|2p+0|2p+0|2.0p+0|1.000p+0|1.80000000000000000000p+0|"), "layout format a precision");
                log_inst.error(log_inst.cat.ModuleB, "|{:a}|{:a}|{:a}|{:a}|", 4.9e-324, 2.2250738585072014e-308, 2.2250738585072009e-308, 0.1f);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|1p-1074|1p-1022|1.ffffffffffffep-1023|1.99999ap-4|"), "layout format a subnormal and float");
                log_inst.error(log_inst.cat.ModuleB, "|{:.4a}|{:.2a}|{:.15a}|", 4.9e-324, 2.2250738585072009e-308, 0.1);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|0.0000p-1022|1.00p-1022|1.999999999999a00p-4|"), "layout format a subnormal with precision");
                log_inst.error(log_inst.cat.ModuleB, "|{:a}|{:A}|", static_cast<double>(INFINITY), static_cast<double>(NAN));
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|inf|NAN|"), "layout format a infinity and nan");
                // The # flag always writes a radix point, and keeps the trailing zeros of g.
                log_inst.error(log_inst.cat.ModuleB, "|{:#.0f}|{:#.0e}|{:#g}|{:#.3g}|{:#a}|{:#.0f}|{:#.0f}|", 1.5, 1.5, 1.5, 100.0, 1.0, 1e20, static_cast<double>(INFINITY));
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|2.|2.e+00|1.50000|100.|1.p+0|100000000000000000000.|inf|"), "layout format alternate form of floating point");
                // The space sign works for floating-point values too.
                log_inst.error(log_inst.cat.ModuleB, "|{: .2f}|{: .1f}|{: e}|{: g}|{: a}|{: }|{: 08.2f}|", 1.5, -1.5, 1.5, 1.5, 1.5, static_cast<double>(NAN), 1.5);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t| 1.50|-1.5| 1.500000e+00| 1.5| 1.8p+0| nan| 0001.50|"), "layout format space sign of floating point");
                // bool is written as true and false, and as 1 and 0 with an integer type.
                log_inst.error(log_inst.cat.ModuleB, "|{}|{}|{:s}|{:d}|{:x}|{:#b}|{:o}|{:+d}|", true, false, true, true, true, true, false, true);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|true|false|true|1|1|0b1|0|+1|"), "layout format bool");
                // A character with an integer type is written as its (unsigned) code.
                log_inst.error(log_inst.cat.ModuleB, "|{:d}|{:x}|{:#x}|{:b}|{:c}|{:d}|{:+d}|{:05d}|", 'A', 'A', 'A', 'A', 'A', static_cast<char>(-1), 'A', 'A');
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|65|41|0x41|1000001|A|255|+65|00065|"), "layout format char as integer");
                log_inst.error(log_inst.cat.ModuleB, "|{:x}|{:X}|", static_cast<char16_t>(0xE9), static_cast<char32_t>(0x1F600));
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|e9|1F600|"), "layout format char16 and char32 as integer");
                // The precision of a string is its maximum width in columns; East Asian wide characters take two.
                log_inst.error(log_inst.cat.ModuleB, "|{:.2}|{:<8.3}|{:>8.3}|{:.0}|{:.10}|", "hello", "hello", "hello", "hello", "hello");
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|he|hel     |     hel||hello|"), "layout format string precision");
                log_inst.error(log_inst.cat.ModuleB, "|{:.2}|{:.1}|{:.3}|{:.2}|", "h\xc3\xa9llo", "\xe4\xb8\xad\xe6\x96\x87", "\xe4\xb8\xad\xe6\x96\x87", u"hello");
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|h\xc3\xa9||\xe4\xb8\xad|he|"), "layout format string precision in columns");
                // Without an alignment, text aligns left and numbers right.
                log_inst.error(log_inst.cat.ModuleB, "|{:8}|{:8}|{:8}|{:8}|{:>8}|", "hi", u"hi", true, 'A', "hi");
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|hi      |hi      |true    |A       |      hi|"), "layout format default alignment of text");
                log_inst.error(log_inst.cat.ModuleB, "|{:8}|{:8.1f}|{:8d}|{:8d}|", 12, 1.5, true, 'A');
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|      12|     1.5|       1|      65|"), "layout format default alignment of numbers");
                // The width of text is counted in columns, like its precision.
                log_inst.error(log_inst.cat.ModuleB, "|{:6}|{:>6}|{:^7}|{:*<5.3}|", "\xe4\xb8\xad\xe6\x96\x87", "h\xc3\xa9llo", "\xe4\xb8\xad", "\xe4\xb8\xad\xe6\x96\x87");
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|\xe4\xb8\xad\xe6\x96\x87  | h\xc3\xa9llo|  \xe4\xb8\xad   |\xe4\xb8\xad***|"), "layout format width of text in columns");
                double dd { 3.14159265758 / 2.3 };
                log_inst.error(log_inst.cat.ModuleB, "|{:12.3e}|", dd);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|   1.366e+00|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:12.3e}|", 103.1234);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|   1.031e+02|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:12d}|", dd);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|1.365909851121739|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:12.3}|", dd);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|        1.37|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{}|", -0.5);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|-0.500000000000000|"), "layout format negative fraction");
                log_inst.error(log_inst.cat.ModuleB, "|{:+.1f}|", -0.25f);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|-0.2|"), "layout format negative fraction with sign");
                log_inst.error(log_inst.cat.ModuleB, "|{:.2f}|", 0.96);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|0.96|"), "layout format rounding");
                log_inst.error(log_inst.cat.ModuleB, "|{:.0f}|", 1.75);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|2|"), "layout format rounding to integer");
                log_inst.error(log_inst.cat.ModuleB, "|{:.1f}|", 1e20);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|100000000000000000000.0|"), "layout format beyond int64");
                log_inst.error(log_inst.cat.ModuleB, "|{}|{}|{}|", static_cast<double>(NAN), static_cast<double>(INFINITY), -static_cast<double>(INFINITY));
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|nan|inf|-inf|"), "layout format nan and infinity");
                log_inst.error(log_inst.cat.ModuleB, "|{:12e}|", 10000000000000);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|1.000000e+13|"), "layout format integer e-style");
                log_inst.error(log_inst.cat.ModuleB, "|{:12E}|", (uint64_t)10000000000000);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|1.000000E+13|"), "layout format integer e-style");
                log_inst.error(log_inst.cat.ModuleB, "v=[{:e}] end", 100000);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\tv=[1.000000e+05] end"), "layout format integer e-style keeps preceding text");
                log_inst.error(log_inst.cat.ModuleB, "|{:e}|{:+.2e}|{:.1e}|{:.0e}|", -1500, (uint16_t)1500, 99999, 25);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|-1.500000e+03|+1.50e+03|1.0e+05|2e+01|"), "layout format integer e-style sign and rounding");
                {
                    // A spec longer than 10 characters is not supported and must not keep its width:
                    // the output buffer is not grown for it, so padding near the end of the buffer
                    // used to write past it. Try every line length around the initial 1024 byte buffer.
                    bool long_spec_ok = true;
                    bool width_ok = true;
                    bq::string padding;
                    bq::string spaces;
                    for (uint32_t space = 0; space < 98; ++space) {
                        spaces.push_back(' ');
                    }
                    for (uint32_t pad = 0; pad < 1100; ++pad) {
                        log_inst.error(log_inst.cat.ModuleB, (padding + "{:<99--------}").c_str(), 1);
                        long_spec_ok &= log_str.end_with("[E]\t[ModuleB]\t" + padding + "1");
                        log_inst.error(log_inst.cat.ModuleB, (padding + "{:<99}").c_str(), 1);
                        width_ok &= log_str.end_with("[E]\t[ModuleB]\t" + padding + "1" + spaces);
                        padding.push_back('a');
                    }
                    result.add_result(long_spec_ok, "layout format spec longer than 10 characters");
                    result.add_result(width_ok, "layout format width near the end of the buffer");
                }
                {
                    // The sign and the "0x"/"0b" prefix of an integer need room on top of its digits.
                    // Try every line length around the 1024 and 2048 byte buffer sizes, so the last
                    // digit lands on the end of the buffer.
                    bool hex_ok = true;
                    bool hex_min_ok = true;
                    bool bin_ok = true;
                    bool bin_max_ok = true;
                    bq::string padding;
                    bq::string ones_63;
                    for (uint32_t bit = 0; bit < 63; ++bit) {
                        ones_63.push_back('1');
                    }
                    for (uint32_t pad = 0; pad < 2100; ++pad) {
                        log_inst.error(log_inst.cat.ModuleB, (padding + "{:#x}").c_str(), static_cast<uint64_t>(0xFFFFFFFFFFFFFFFFULL));
                        hex_ok &= log_str.end_with("[E]\t[ModuleB]\t" + padding + "0xffffffffffffffff");
                        log_inst.error(log_inst.cat.ModuleB, (padding + "{:#x}").c_str(), static_cast<int64_t>(INT64_MIN));
                        hex_min_ok &= log_str.end_with("[E]\t[ModuleB]\t" + padding + "-0x8000000000000000");
                        log_inst.error(log_inst.cat.ModuleB, (padding + "{:+#b}").c_str(), static_cast<int64_t>(INT64_MAX));
                        bin_ok &= log_str.end_with("[E]\t[ModuleB]\t" + padding + "+0b" + ones_63);
                        log_inst.error(log_inst.cat.ModuleB, (padding + "{:+#b}").c_str(), static_cast<uint64_t>(0xFFFFFFFFFFFFFFFFULL));
                        bin_max_ok &= log_str.end_with("[E]\t[ModuleB]\t" + padding + "+0b1" + ones_63);
                        padding.push_back('a');
                    }
                    result.add_result(hex_ok, "layout format {:#x} of UINT64_MAX near the end of the buffer");
                    result.add_result(hex_min_ok, "layout format {:#x} of INT64_MIN near the end of the buffer");
                    result.add_result(bin_ok, "layout format {:+#b} of INT64_MAX near the end of the buffer");
                    result.add_result(bin_max_ok, "layout format {:+#b} of UINT64_MAX near the end of the buffer");
                }
                {
                    const char* current_locale = setlocale(LC_NUMERIC, nullptr);
                    bq::string saved_locale = current_locale ? current_locale : "C";
                    const char* comma_locales[] = { "de-DE", "de_DE.UTF-8", "de_DE.utf8", "fr_FR.UTF-8", "fr_FR.utf8" };
                    bool locale_applied = false;
                    for (const char* name : comma_locales) {
                        if (setlocale(LC_NUMERIC, name)) {
                            locale_applied = true;
                            break;
                        }
                    }
                    if (locale_applied) {
                        log_inst.error(log_inst.cat.ModuleB, "|{:.2f}|{:.1f}|", 1.25, 1e20);
                        result.add_result(log_str.end_with("[E]\t[ModuleB]\t|1.25|100000000000000000000.0|"), "layout format fixed notation ignores LC_NUMERIC");
                        log_inst.error(log_inst.cat.ModuleB, "|{:e}|{:12.3E}|{:.25f}|", 1.5, 103.1234, 0.1);
                        result.add_result(log_str.end_with("[E]\t[ModuleB]\t|1.500000e+00|   1.031E+02|0.1000000000000000055511151|"), "layout format snprintf path ignores LC_NUMERIC");
                        setlocale(LC_NUMERIC, saved_locale.c_str());
                    }
                }
                {
                    std::mt19937_64 rng(20261009);
                    char fmt[32];
                    char expected[512];
                    for (int32_t precision = 0; precision <= 19; ++precision) {
                        snprintf(fmt, sizeof(fmt), "|{:.%" PRId32 "f}|", precision);
                        for (int32_t n = 0; n < 300; ++n) {
                            double value = 0;
                            uint64_t bits = rng();
                            switch (n % 4) {
                            case 0:
                                value = std::uniform_real_distribution<double>(-1000.0, 1000.0)(rng);
                                break;
                            case 1:
                                // any magnitude below 2^64, subnormals included
                                bits = (bits & 0x800FFFFFFFFFFFFFULL) | (((bits >> 52) % (1023 + 64)) << 52);
                                memcpy(&value, &bits, sizeof(value));
                                break;
                            case 2:
                                // exact binary ties
                                value = static_cast<double>(static_cast<int64_t>(bits % 2000) - 1000) + static_cast<double>((bits >> 32) % 16) / 16.0;
                                break;
                            default:
                                value = static_cast<double>(static_cast<float>(std::uniform_real_distribution<double>(-100000.0, 100000.0)(rng)));
                                break;
                            }
                            snprintf(expected, sizeof(expected), "|%.*f|", static_cast<int>(precision), value);
                            log_inst.error(log_inst.cat.ModuleB, fmt, value);
                            result.add_result(log_str.end_with(expected), "layout format %s of %.17g matches printf %s", fmt, value, expected);
                        }
                    }
                }

                i = 15841548461;
                log_inst.error(log_inst.cat.ModuleB, "|{:+10d}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|+15841548461|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:10b}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|1110110000001110101101100010101101|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:#10b}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|0b1110110000001110101101100010101101|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:10X}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t| 3B03AD8AD|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:#10X}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|0X3B03AD8AD|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:<10d}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|15841548461|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:>010d}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|15841548461|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:<010d}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|15841548461|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:#010x}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|0x3b03ad8ad|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:<#010x}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|0x3b03ad8ad|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:>#010x}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|0x3b03ad8ad|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:06d}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|15841548461|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:^06d}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|15841548461|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:+06d}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|+15841548461|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:<+06d}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|+15841548461|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:+06d}|", -i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|-15841548461|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:<+06d}|", -i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|-15841548461|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:06X}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|3B03AD8AD|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:#06X}|", i);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|0X3B03AD8AD|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:12.3e}|", dd);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|   1.366e+00|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:12.3e}|", 103.1234);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|   1.031e+02|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:12d}|", 100);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|         100|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:12.3}|", dd);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|        1.37|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:12.3e}|", 10000000000000.0);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|   1.000e+13|"), "layout format");
                log_inst.error(log_inst.cat.ModuleB, "|{:12.3E}|", 10000000000000.0);
                result.add_result(log_str.end_with("[E]\t[ModuleB]\t|   1.000E+13|"), "layout format");
            }

            {
                // Concurrency Stress Test
                auto sync_log = bq::log::create_log("sync_log_stress", R"(
		            appenders_config.appender_1.type=console
                    log.thread_mode=sync
	            )");
                auto async_log = bq::log::create_log("async_log_stress", R"(
		            appenders_config.appender_1.type=console
                    log.thread_mode=async
	            )");

                std::atomic<int32_t> live_thread { 0 };
                std::atomic<int32_t> left_thread { 100 };
                bq::string appender;
                for (int32_t i = 0; i < 32; ++i)
                    appender += "a";

                // Sync Test
                while (left_thread > 0 || live_thread > 0) {
                    if (left_thread > 0 && live_thread < 5) {
                        left_thread--;
                        live_thread++;
                        std::thread([&]() {
                            bq::string log_content = "";
                            for (int32_t i = 0; i < 128; ++i) {
                                log_content += appender;
                                sync_log.info(log_content.c_str());
                            }
                            live_thread--;
                        }).detach();
                    } else {
                        bq::platform::thread::sleep(1);
                    }
                }

                // Async Test
                left_thread = 128;
                while (left_thread > 0 || live_thread > 0) {
                    if (left_thread > 0 && live_thread < 5) {
                        left_thread--;
                        live_thread++;
                        std::thread([&]() {
                            bq::string log_content = "";
                            for (int32_t i = 0; i < 2048; ++i) {
                                log_content += appender;
                                async_log.info(log_content.c_str());
                            }
                            live_thread--;
                        }).detach();
                    } else {
                        bq::platform::thread::sleep(1);
                    }
                }
                async_log.force_flush();
                bq::log::register_console_callback(&test_log::console_callback);
            }

            {
                const uint64_t seconds = 10;
                test_output_dynamic_param(bq::log_level::info, "testing force flush. wait for %" PRIu64 " seconds please...\n if error exist, an assert will be triggered\n", seconds);
                bq::log::register_console_callback(&test_force_flush_callback::console_callback);
                test_force_flush_callback::test_result_ptr = &result;
                uint64_t start_time = bq::platform::high_performance_epoch_ms();
                auto sync_log = bq::log::create_log("sync_log", R"(
		            appenders_config.appender_1.type=console
                    log.thread_mode=sync
	            )");
                auto async_log = bq::log::create_log("async_log", R"(
		            appenders_config.appender_1.type=console
                    log.thread_mode=async
	            )");
                test_force_flush_callback::sync_log_id = sync_log.get_id();
                test_force_flush_callback::async_log_id = async_log.get_id();
                std::thread tr1([]() {
                    auto log_obj = bq::log::get_log_by_name("async_log");
                    while (!test_force_flush_callback::multi_thread_test_end) {
                        log_obj.force_flush();
                        bq::platform::thread::sleep(3);
                    }
                });
                tr1.detach();
                std::thread tr2([]() {
                    while (!test_force_flush_callback::multi_thread_test_end) {
                        bq::log::force_flush_all_logs();
                        bq::platform::thread::sleep(3);
                    }
                });
                tr2.detach();

                while (true) {
                    sync_log.info("force flush test sync log {}", test_force_flush_callback::idx.add_fetch_seq_cst(1));
                    async_log.info("force flush test async log {}", test_force_flush_callback::idx.load_seq_cst());
                    if (bq::platform::high_performance_epoch_ms() - start_time >= seconds * 1000) {
                        test_force_flush_callback::multi_thread_test_end = true;
                        break;
                    }
                }
                bq::log::force_flush_all_logs();
                bool force_flush_sync_passed = (test_force_flush_callback::idx.load_seq_cst() == test_force_flush_callback::current_idx_sync.load_seq_cst());
                bool force_flush_async_passed = (test_force_flush_callback::idx.load_seq_cst() == test_force_flush_callback::current_idx_async.load_seq_cst());
                result.add_result(force_flush_sync_passed, "force flush test sync total count, %" PRIu64 ", %" PRIu64, test_force_flush_callback::idx.load_seq_cst(), test_force_flush_callback::current_idx_sync.load_seq_cst());
                result.add_result(force_flush_async_passed, "force flush test async total count, %" PRIu64 ", %" PRIu64, test_force_flush_callback::idx.load_seq_cst(), test_force_flush_callback::current_idx_async.load_seq_cst());
                if (!force_flush_sync_passed || !force_flush_async_passed) {
                    bq::log::force_flush_all_logs();
                    test_output_param(bq::log_level::error, "force flush re-check, idx:%" PRIu64 ", sync:%" PRIu64 ", async:%" PRIu64, test_force_flush_callback::idx.load_seq_cst(), test_force_flush_callback::current_idx_sync.load_seq_cst(), test_force_flush_callback::current_idx_async.load_seq_cst());
                }

                bq::log::register_console_callback(&test_log::console_callback);
                test_output_dynamic(bq::log_level::info, "force flush testing is finished!\n");
            }

            {
                // snapshot test
                auto snapshot_log = test_category_log::create_log("snapshot_log", R"(
						appenders_config.ConsoleAppender.type=console
						appenders_config.ConsoleAppender.time_zone=localtime
						appenders_config.ConsoleAppender.levels=[error,fatal]
					
						log.thread_mode=sync
						log.categories_mask=[ModuleA.SystemA.ClassA,ModuleB]
                        snapshot.buffer_size=100000
                        snapshot.levels=[info,error]
                        snapshot.categories_mask=[ModuleA.SystemA.ClassA,ModuleB]
			        )");

                auto snapshot = snapshot_log.take_snapshot("gmt");
                snapshot_log.verbose("AAAA");
                result.add_result(snapshot_log.take_snapshot("gmt") == snapshot, "snapshot test 1");
                snapshot_log.info(snapshot_log.cat.ModuleA.SystemA, "AAAA");
                result.add_result(snapshot_log.take_snapshot("gmt") == snapshot, "snapshot test 2");
                snapshot_log.error(snapshot_log.cat.ModuleA.SystemA.ClassA, "AAAA");
                auto new_snapshot1 = snapshot_log.take_snapshot("gmt");
                result.add_result(new_snapshot1 != snapshot && new_snapshot1.end_with("AAAA\n"), "snapshot test 3");
                snapshot_log.error(snapshot_log.cat.ModuleA.SystemA.ClassA, "BBBB");
                auto snapshot_log2 = test_category_log::create_log("snapshot_log", R"(
						appenders_config.ConsoleAppender.type=console
						appenders_config.ConsoleAppender.time_zone=localtime
						appenders_config.ConsoleAppender.levels=[error,fatal]
					
						log.thread_mode=sync
						log.categories_mask=[ModuleA.SystemA.ClassA,ModuleB]
                        snapshot.buffer_size=1000000       //more memory
                        snapshot.levels=[info,error]
                        snapshot.categories_mask=[ModuleA.SystemA.ClassA,ModuleB]
			        )");
                snapshot_log2.error(snapshot_log2.cat.ModuleA.SystemA.ClassA, "CCCC");
                auto new_snapshot2 = snapshot_log2.take_snapshot("gmt");
                result.add_result(new_snapshot2.begin_with(new_snapshot1), "snapshot test 4");
                result.add_result(new_snapshot2.find("BBBB") != bq::string::npos, "snapshot test 5");
                result.add_result(new_snapshot2.find("CCCC") > new_snapshot2.find("BBBB"), "snapshot test 6");
            }
        }
    }
}
