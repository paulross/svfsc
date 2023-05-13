"""Examples of seek read operations.

MIT License

Copyright (c) 2020-2023 Paul Ross

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
"""

# A fictitious example of a Sparse File
# This has 32 bytes of data every 64 bytes over a size of 20480.
EXAMPLE_FILE_POSITIONS_LENGTHS_SYNTHETIC = tuple((fpos, 32) for fpos in range(0, 2048 * 10, 64))


def gen_rle_seek_read_actions(seek_read_rle):
    for fpos, reads in seek_read_rle:
        for read_len, count in reads:
            for i in range(count):
                yield fpos, read_len
                fpos += read_len


# This is seek read operations to get all the TIFF metadata from the open-slide test image CMU-1.tiff
# File Position : Read amount, count
EXAMPLE_FILE_POSITIONS_LENGTHS_TIFF_CMU_1_RLE = (
    (0, [(4, 2)],),
    (134029850, [(2, 1), (12, 3)],),
    (134030076, [(2, 3)],),
    (134029888, [(12, 5)],),
    (134030060, [(8, 1)],),
    (134029948, [(12, 1)],),
    (134030068, [(8, 1)],),
    (134029960, [(12, 5)],),
    (134122962, [(4, 23220)],),
    (134030020, [(12, 1)],),
    (134030082, [(4, 23220)],),
    (134030032, [(12, 1)],),
    (134215890, [(574, 1)],),
    (134030044, [(12, 1)],),
    (134215842, [(8, 6)],),
    (134030056, [(4, 1)],),
    (185242180, [(2, 1), (12, 4)],),
    (185242418, [(2, 3)],),
    (185242230, [(12, 5)],),
    (185242402, [(8, 1)],),
    (185242290, [(12, 1)],),
    (185242410, [(8, 1)],),
    (185242302, [(12, 5)],),
    (185265824, [(4, 5850)],),
    (185242362, [(12, 1)],),
    (185242424, [(4, 5850)],),
    (185242374, [(12, 1)],),
    (185289272, [(574, 1)],),
    (185242386, [(12, 1)],),
    (185289224, [(8, 6)],),
    (185242398, [(4, 1)],),
    (199390964, [(2, 1), (12, 4)],),
    (199391202, [(2, 3)],),
    (199391014, [(12, 5)],),
    (199391186, [(8, 1)],),
    (199391074, [(12, 1)],),
    (199391194, [(8, 1)],),
    (199391086, [(12, 5)],),
    (199397148, [(4, 1485)],),
    (199391146, [(12, 1)],),
    (199391208, [(4, 1485)],),
    (199391158, [(12, 1)],),
    (199403136, [(574, 1)],),
    (199391170, [(12, 1)],),
    (199403088, [(8, 6)],),
    (199391182, [(4, 1)],),
    (202857928, [(2, 1), (12, 4)],),
    (202858166, [(2, 3)],),
    (202857978, [(12, 5)],),
    (202858150, [(8, 1)],),
    (202858038, [(12, 1)],),
    (202858158, [(8, 1)],),
    (202858050, [(12, 5)],),
    (202859736, [(4, 391)],),
    (202858110, [(12, 1)],),
    (202858172, [(4, 391)],),
    (202858122, [(12, 1)],),
    (202861348, [(574, 1)],),
    (202858134, [(12, 1)],),
    (202861300, [(8, 6)],),
    (202858146, [(4, 1)],),
    (203744628, [(2, 1), (12, 4)],),
    (203744866, [(2, 3)],),
    (203744678, [(12, 5)],),
    (203744850, [(8, 1)],),
    (203744738, [(12, 1)],),
    (203744858, [(8, 1)],),
    (203744750, [(12, 5)],),
    (203745304, [(4, 108)],),
    (203744810, [(12, 1)],),
    (203744872, [(4, 108)],),
    (203744822, [(12, 1)],),
    (203745784, [(574, 1)],),
    (203744834, [(12, 1)],),
    (203745736, [(8, 6)],),
    (203744846, [(4, 1)],),
    (203985588, [(2, 1), (12, 4)],),
    (203985826, [(2, 3)],),
    (203985638, [(12, 5)],),
    (203985810, [(8, 1)],),
    (203985698, [(12, 1)],),
    (203985818, [(8, 1)],),
    (203985710, [(12, 5)],),
    (203985952, [(4, 30)],),
    (203985770, [(12, 1)],),
    (203985832, [(4, 30)],),
    (203985782, [(12, 1)],),
    (203986120, [(574, 1)],),
    (203985794, [(12, 1)],),
    (203986072, [(8, 6)],),
    (203985806, [(4, 1)],),
    (204061812, [(2, 1), (12, 4)],),
    (204062050, [(2, 3)],),
    (204061862, [(12, 5)],),
    (204062034, [(8, 1)],),
    (204061922, [(12, 1)],),
    (204062042, [(8, 1)],),
    (204061934, [(12, 5)],),
    (204062092, [(4, 9)],),
    (204061994, [(12, 1)],),
    (204062056, [(4, 9)],),
    (204062006, [(12, 1)],),
    (204062176, [(574, 1)],),
    (204062018, [(12, 1)],),
    (204062128, [(8, 6)],),
    (204062030, [(4, 1)],),
    (204109962, [(2, 1), (12, 4)],),
    (204110200, [(2, 3)],),
    (204110012, [(12, 5)],),
    (204110184, [(8, 1)],),
    (204110072, [(12, 1)],),
    (204110192, [(8, 1)],),
    (204110084, [(12, 5)],),
    (204110222, [(4, 4)],),
    (204110144, [(12, 1)],),
    (204110206, [(4, 4)],),
    (204110156, [(12, 1)],),
    (204110286, [(574, 1)],),
    (204110168, [(12, 1)],),
    (204110238, [(8, 6)],),
    (204110180, [(4, 1)],),
    (204116980, [(2, 1), (12, 4)],),
    (204117218, [(2, 3)],),
    (204117030, [(12, 5)],),
    (204117202, [(8, 1)],),
    (204117090, [(12, 1)],),
    (204117210, [(8, 1)],),
    (204117102, [(12, 7)],),
    (204117272, [(574, 1)],),
    (204117186, [(12, 1)],),
    (204117224, [(8, 6)],),
    (204117198, [(4, 1)],),
)

EXAMPLE_FILE_POSITIONS_LENGTHS_TIFF_CMU_1 = tuple(
    gen_rle_seek_read_actions(EXAMPLE_FILE_POSITIONS_LENGTHS_TIFF_CMU_1_RLE))

# TUPAC-TR-001.svs
# Seek read blocks [78] in (value, count) form:
# File Position : Read amount, count
EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_001_svs_RLE = (
    (0, [(4, 2)],),
    (2004273992, [(2, 1), (12, 4)],),
    (2004274190, [(2, 3)],),
    (2004274042, [(12, 3)],),
    (2004274196, [(115, 1)],),
    (2004274078, [(12, 5)],),
    (2004274312, [(4, 492203)],),
    (2004274138, [(12, 1)],),
    (2006243124, [(4, 492203)],),
    (2004274150, [(12, 1)],),
    (2008211936, [(289, 1)],),
    (2004274162, [(12, 2), (4, 1)],),
    (2008262396, [(2, 1), (12, 4)],),
    (2008262582, [(2, 3)],),
    (2008262446, [(12, 3)],),
    (2008262588, [(104, 1)],),
    (2008262482, [(12, 1)],),
    (2008262692, [(4, 48)],),
    (2008262494, [(12, 3)],),
    (2008262884, [(4, 48)],),
    (2008262530, [(12, 2)],),
    (2008263076, [(289, 1)],),
    (2008262554, [(12, 2), (4, 1)],),
    (2220001884, [(2, 1), (12, 4)],),
    (2220002082, [(2, 3)],),
    (2220001934, [(12, 3)],),
    (2220002088, [(84, 1)],),
    (2220001970, [(12, 5)],),
    (2220002172, [(4, 30976)],),
    (2220002030, [(12, 1)],),
    (2220126076, [(4, 30976)],),
    (2220002042, [(12, 1)],),
    (2220249980, [(289, 1)],),
    (2220002054, [(12, 2), (4, 1)],),
    (2245498626, [(2, 1), (12, 4)],),
    (2245498824, [(2, 3)],),
    (2245498676, [(12, 3)],),
    (2245498830, [(83, 1)],),
    (2245498712, [(12, 5)],),
    (2245498914, [(4, 1952)],),
    (2245498772, [(12, 1)],),
    (2245506722, [(4, 1952)],),
    (2245498784, [(12, 1)],),
    (2245514530, [(289, 1)],),
    (2245498796, [(12, 2), (4, 1)],),
    (2248363744, [(2, 1), (12, 4)],),
    (2248363942, [(2, 3)],),
    (2248363794, [(12, 3)],),
    (2248363948, [(82, 1)],),
    (2248363830, [(12, 5)],),
    (2248364030, [(4, 128)],),
    (2248363890, [(12, 1)],),
    (2248364542, [(4, 128)],),
    (2248363902, [(12, 1)],),
    (2248365054, [(289, 1)],),
    (2248363914, [(12, 2), (4, 1)],),
    (2249464648, [(2, 1), (12, 4)],),
    (2249464846, [(2, 3)],),
    (2249464698, [(12, 3)],),
    (2249464852, [(82, 1)],),
    (2249464734, [(12, 5)],),
    (2249464934, [(4, 32)],),
    (2249464794, [(12, 1)],),
    (2249465062, [(4, 32)],),
    (2249464806, [(12, 1)],),
    (2249465190, [(289, 1)],),
    (2249464818, [(12, 2), (4, 1)],),
    (2249782464, [(2, 1), (12, 4)],),
    (2249781802, [(2, 3)],),
    (2249782514, [(12, 3)],),
    (2249782136, [(4, 82)],),
    (2249782550, [(12, 3)],),
    (2249781808, [(4, 82)],),
    (2249782586, [(12, 1)],),
    (2249781786, [(8, 1)],),
    (2249782598, [(12, 1)],),
    (2249781794, [(8, 1)],),
    (2249782610, [(12, 3), (4, 1)],),
    (2249751796, [(2, 1), (12, 4)],),
    (2249751982, [(2, 3)],),
    (2249751846, [(12, 3)],),
    (2249751988, [(45, 1)],),
    (2249751882, [(12, 1)],),
    (2249752034, [(4, 64)],),
    (2249751894, [(12, 3)],),
    (2249752290, [(4, 64)],),
    (2249751930, [(12, 2)],),
    (2249752546, [(289, 1)],),
    (2249751954, [(12, 2), (4, 1)],),
)

EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_001_svs = tuple(
    gen_rle_seek_read_actions(EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_001_svs_RLE))

# TUPAC-TR-002.svs
#     File size 689,156,184 file bytes read: 483,582 (0.070%)
# Seek read blocks [63] in (value, count) form:
# File Position : Read amount, count
EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_002_svs_RLE = (
    (0, [(4, 2)],),
    (626030488, [(2, 1), (12, 4)],),
    (626030674, [(2, 3)],),
    (626030538, [(12, 3)],),
    (626030680, [(517, 1)],),
    (626030574, [(12, 5)],),
    (626031198, [(4, 39498)],),
    (626030634, [(12, 1)],),
    (626189190, [(4, 39498)],),
    (626030646, [(12, 2)],),
    (626347182, [(141992, 1)],),
    (626030670, [(4, 1)],),
    (626866788, [(2, 1), (12, 4)],),
    (626866974, [(2, 3)],),
    (626866838, [(12, 3)],),
    (626866980, [(486, 1)],),
    (626866874, [(12, 1)],),
    (626867466, [(4, 48)],),
    (626866886, [(12, 3)],),
    (626867658, [(4, 48)],),
    (626866922, [(12, 2)],),
    (626867850, [(289, 1)],),
    (626866946, [(12, 2), (4, 1)],),
    (685100738, [(2, 1), (12, 4)],),
    (685100912, [(2, 3)],),
    (685100788, [(12, 3)],),
    (685100918, [(103, 1)],),
    (685100824, [(12, 5)],),
    (685101022, [(4, 2508)],),
    (685100884, [(12, 1)],),
    (685111054, [(4, 2508)],),
    (685100896, [(12, 1), (4, 1)],),
    (688729558, [(2, 1), (12, 4)],),
    (688729732, [(2, 3)],),
    (688729608, [(12, 3)],),
    (688729738, [(101, 1)],),
    (688729644, [(12, 5)],),
    (688729840, [(4, 165)],),
    (688729704, [(12, 1)],),
    (688730500, [(4, 165)],),
    (688729716, [(12, 1), (4, 1)],),
    (689155998, [(2, 1), (12, 4)],),
    (689155344, [(2, 3)],),
    (689156048, [(12, 3)],),
    (689155674, [(4, 81)],),
    (689156084, [(12, 3)],),
    (689155350, [(4, 81)],),
    (689156120, [(12, 1)],),
    (689155328, [(8, 1)],),
    (689156132, [(12, 1)],),
    (689155336, [(8, 1)],),
    (689156144, [(12, 3), (4, 1)],),
    (689123006, [(2, 1), (12, 4)],),
    (689123192, [(2, 3)],),
    (689123056, [(12, 3)],),
    (689123198, [(45, 1)],),
    (689123092, [(12, 1)],),
    (689123244, [(4, 26)],),
    (689123104, [(12, 3)],),
    (689123348, [(4, 26)],),
    (689123140, [(12, 2)],),
    (689123452, [(289, 1)],),
    (689123164, [(12, 2), (4, 1)],),
)

EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_002_svs = tuple(
    gen_rle_seek_read_actions(EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_002_svs_RLE))

# TUPAC-TR-003.svs
# File size 590,615,632 file bytes read: 242,436 (0.041%)
# Seek read blocks [78] in (value, count) form:
# File Position : Read amount, count
EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_003_svs_RLE = (
    (0, [(4, 2)],),
    (526990334, [(2, 1), (12, 4)],),
    (526990532, [(2, 3)],),
    (526990384, [(12, 3)],),
    (526990538, [(112, 1)],),
    (526990420, [(12, 5)],),
    (526990650, [(4, 27727)],),
    (526990480, [(12, 1)],),
    (527101558, [(4, 27727)],),
    (526990492, [(12, 1)],),
    (527212466, [(289, 1)],),
    (526990504, [(12, 2), (4, 1)],),
    (527298850, [(2, 1), (12, 4)],),
    (527299036, [(2, 3)],),
    (527298900, [(12, 3)],),
    (527299042, [(101, 1)],),
    (527298936, [(12, 1)],),
    (527299144, [(4, 48)],),
    (527298948, [(12, 3)],),
    (527299336, [(4, 48)],),
    (527298984, [(12, 2)],),
    (527299528, [(289, 1)],),
    (527299008, [(12, 2), (4, 1)],),
    (583368170, [(2, 1), (12, 4)],),
    (583368368, [(2, 3)],),
    (583368220, [(12, 3)],),
    (583368374, [(80, 1)],),
    (583368256, [(12, 5)],),
    (583368454, [(4, 1770)],),
    (583368316, [(12, 1)],),
    (583375534, [(4, 1770)],),
    (583368328, [(12, 1)],),
    (583382614, [(289, 1)],),
    (583368340, [(12, 2), (4, 1)],),
    (588455284, [(2, 1), (12, 4)],),
    (588455482, [(2, 3)],),
    (588455334, [(12, 3)],),
    (588455488, [(79, 1)],),
    (588455370, [(12, 5)],),
    (588455568, [(4, 120)],),
    (588455430, [(12, 1)],),
    (588456048, [(4, 120)],),
    (588455442, [(12, 1)],),
    (588456528, [(289, 1)],),
    (588455454, [(12, 2), (4, 1)],),
    (590124274, [(2, 1), (12, 4)],),
    (590124472, [(2, 3)],),
    (590124324, [(12, 3)],),
    (590124478, [(78, 1)],),
    (590124360, [(12, 5)],),
    (590124556, [(4, 32)],),
    (590124420, [(12, 1)],),
    (590124684, [(4, 32)],),
    (590124432, [(12, 1)],),
    (590124812, [(289, 1)],),
    (590124444, [(12, 2), (4, 1)],),
    (590615446, [(2, 1), (12, 4)],),
    (590614792, [(2, 3)],),
    (590615496, [(12, 3)],),
    (590615122, [(4, 81)],),
    (590615532, [(12, 3)],),
    (590614798, [(4, 81)],),
    (590615568, [(12, 1)],),
    (590614776, [(8, 1)],),
    (590615580, [(12, 1)],),
    (590614784, [(8, 1)],),
    (590615592, [(12, 3), (4, 1)],),
    (590417588, [(2, 1), (12, 4)],),
    (590417774, [(2, 3)],),
    (590417638, [(12, 3)],),
    (590417780, [(44, 1)],),
    (590417674, [(12, 1)],),
    (590417824, [(4, 71)],),
    (590417686, [(12, 3)],),
    (590418108, [(4, 71)],),
    (590417722, [(12, 2)],),
    (590418392, [(289, 1)],),
    (590417746, [(12, 2), (4, 1)],),
)

EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_003_svs = tuple(
    gen_rle_seek_read_actions(EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_003_svs_RLE))

# TUPAC-TR-004.svs
#     File size 780,367,739 file bytes read: 1,311,074 (0.168%)
# Seek read blocks [78] in (value, count) form:
# File Position : Read amount, count
EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_004_svs_RLE = (
    (0, [(4, 2)],),
    (668498064, [(2, 1), (12, 4)],),
    (668498274, [(2, 3)],),
    (668498114, [(12, 3)],),
    (668498280, [(666, 1)],),
    (668498150, [(12, 5)],),
    (668498946, [(4, 135978)],),
    (668498210, [(12, 1)],),
    (669042858, [(4, 135978)],),
    (668498222, [(12, 1)],),
    (669586770, [(289, 1)],),
    (668498234, [(12, 3)],),
    (669587060, [(141992, 1)],),
    (668498270, [(4, 1)],),
    (669921708, [(2, 1), (12, 4)],),
    (669921894, [(2, 3)],),
    (669921758, [(12, 3)],),
    (669921900, [(635, 1)],),
    (669921794, [(12, 1)],),
    (669922536, [(4, 48)],),
    (669921806, [(12, 3)],),
    (669922728, [(4, 48)],),
    (669921842, [(12, 2)],),
    (669922920, [(289, 1)],),
    (669921866, [(12, 2), (4, 1)],),
    (763703858, [(2, 1), (12, 4)],),
    (763704056, [(2, 3)],),
    (763703908, [(12, 3)],),
    (763704062, [(102, 1)],),
    (763703944, [(12, 5)],),
    (763704164, [(4, 8613)],),
    (763704004, [(12, 1)],),
    (763738616, [(4, 8613)],),
    (763704016, [(12, 1)],),
    (763773068, [(289, 1)],),
    (763704028, [(12, 2), (4, 1)],),
    (775175442, [(2, 1), (12, 4)],),
    (775175640, [(2, 3)],),
    (775175492, [(12, 3)],),
    (775175646, [(100, 1)],),
    (775175528, [(12, 5)],),
    (775175746, [(4, 550)],),
    (775175588, [(12, 1)],),
    (775177946, [(4, 550)],),
    (775175600, [(12, 1)],),
    (775180146, [(289, 1)],),
    (775175612, [(12, 2), (4, 1)],),
    (779565210, [(2, 1), (12, 4)],),
    (779565408, [(2, 3)],),
    (779565260, [(12, 3)],),
    (779565414, [(100, 1)],),
    (779565296, [(12, 5)],),
    (779565514, [(4, 143)],),
    (779565356, [(12, 1)],),
    (779566086, [(4, 143)],),
    (779565368, [(12, 1)],),
    (779566658, [(289, 1)],),
    (779565380, [(12, 2), (4, 1)],),
    (780149804, [(2, 1), (12, 4)],),
    (780149978, [(2, 3)],),
    (780149854, [(12, 3)],),
    (780149984, [(44, 1)],),
    (780149890, [(12, 1)],),
    (780150028, [(4, 159)],),
    (780149902, [(12, 3)],),
    (780150664, [(4, 159)],),
    (780149938, [(12, 3), (4, 1)],),
    (780366884, [(2, 1), (12, 4)],),
    (780367070, [(2, 3)],),
    (780366934, [(12, 3)],),
    (780367076, [(45, 1)],),
    (780366970, [(12, 1)],),
    (780367122, [(4, 41)],),
    (780366982, [(12, 3)],),
    (780367286, [(4, 41)],),
    (780367018, [(12, 2)],),
    (780367450, [(289, 1)],),
    (780367042, [(12, 2), (4, 1)],),
)

EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_004_svs = tuple(
    gen_rle_seek_read_actions(EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_004_svs_RLE))

# TUPAC-TR-005.svs
#     File size 1,000,853,688 file bytes read: 709,714 (0.071%)
# Seek read blocks [78] in (value, count) form:
# File Position : Read amount, count
EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_005_svs_RLE = (
    (0, [(4, 2)],),
    (901039776, [(2, 1), (12, 4)],),
    (901039974, [(2, 3)],),
    (901039826, [(12, 3)],),
    (901039980, [(113, 1)],),
    (901039862, [(12, 5)],),
    (901040094, [(4, 82485)],),
    (901039922, [(12, 1)],),
    (901370034, [(4, 82485)],),
    (901039934, [(12, 1)],),
    (901699974, [(289, 1)],),
    (901039946, [(12, 2), (4, 1)],),
    (901741620, [(2, 1), (12, 4)],),
    (901741806, [(2, 3)],),
    (901741670, [(12, 3)],),
    (901741812, [(102, 1)],),
    (901741706, [(12, 1)],),
    (901741914, [(4, 48)],),
    (901741718, [(12, 3)],),
    (901742106, [(4, 48)],),
    (901741754, [(12, 2)],),
    (901742298, [(289, 1)],),
    (901741778, [(12, 2), (4, 1)],),
    (992008454, [(2, 1), (12, 4)],),
    (992008652, [(2, 3)],),
    (992008504, [(12, 3)],),
    (992008658, [(82, 1)],),
    (992008540, [(12, 5)],),
    (992008740, [(4, 5194)],),
    (992008600, [(12, 1)],),
    (992029516, [(4, 5194)],),
    (992008612, [(12, 1)],),
    (992050292, [(289, 1)],),
    (992008624, [(12, 2), (4, 1)],),
    (999846716, [(2, 1), (12, 4)],),
    (999846914, [(2, 3)],),
    (999846766, [(12, 3)],),
    (999846920, [(80, 1)],),
    (999846802, [(12, 5)],),
    (999847000, [(4, 351)],),
    (999846862, [(12, 1)],),
    (999848404, [(4, 351)],),
    (999846874, [(12, 1)],),
    (999849808, [(289, 1)],),
    (999846886, [(12, 2), (4, 1)],),
    (1000516706, [(2, 1), (12, 4)],),
    (1000516904, [(2, 3)],),
    (1000516756, [(12, 3)],),
    (1000516910, [(79, 1)],),
    (1000516792, [(12, 5)],),
    (1000516990, [(4, 28)],),
    (1000516852, [(12, 1)],),
    (1000517102, [(4, 28)],),
    (1000516864, [(12, 1)],),
    (1000517214, [(289, 1)],),
    (1000516876, [(12, 2), (4, 1)],),
    (1000853502, [(2, 1), (12, 4)],),
    (1000852848, [(2, 3)],),
    (1000853552, [(12, 3)],),
    (1000853178, [(4, 81)],),
    (1000853588, [(12, 3)],),
    (1000852854, [(4, 81)],),
    (1000853624, [(12, 1)],),
    (1000852832, [(8, 1)],),
    (1000853636, [(12, 1)],),
    (1000852840, [(8, 1)],),
    (1000853648, [(12, 3), (4, 1)],),
    (1000819990, [(2, 1), (12, 4)],),
    (1000820176, [(2, 3)],),
    (1000820040, [(12, 3)],),
    (1000820182, [(44, 1)],),
    (1000820076, [(12, 1)],),
    (1000820226, [(4, 71)],),
    (1000820088, [(12, 3)],),
    (1000820510, [(4, 71)],),
    (1000820124, [(12, 2)],),
    (1000820794, [(289, 1)],),
    (1000820148, [(12, 2), (4, 1)],),
)

EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_005_svs = tuple(
    gen_rle_seek_read_actions(EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_005_svs_RLE))

# TUPAC-TR-006.svs
# Seek read blocks [78] in (value, count) form:
# File Position : Read amount, count
EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_006_svs_RLE = (
    (0, [(4, 2)],),
    (825019564, [(2, 1), (12, 4)],),
    (825019774, [(2, 3)],),
    (825019614, [(12, 3)],),
    (825019780, [(666, 1)],),
    (825019650, [(12, 5)],),
    (825020446, [(4, 118987)],),
    (825019710, [(12, 1)],),
    (825496394, [(4, 118987)],),
    (825019722, [(12, 1)],),
    (825972342, [(289, 1)],),
    (825019734, [(12, 3)],),
    (825972632, [(141992, 1)],),
    (825019770, [(4, 1)],),
    (826410724, [(2, 1), (12, 4)],),
    (826410910, [(2, 3)],),
    (826410774, [(12, 3)],),
    (826410916, [(635, 1)],),
    (826410810, [(12, 1)],),
    (826411552, [(4, 48)],),
    (826410822, [(12, 3)],),
    (826411744, [(4, 48)],),
    (826410858, [(12, 2)],),
    (826411936, [(289, 1)],),
    (826410882, [(12, 2), (4, 1)],),
    (969043048, [(2, 1), (12, 4)],),
    (969043246, [(2, 3)],),
    (969043098, [(12, 3)],),
    (969043252, [(102, 1)],),
    (969043134, [(12, 5)],),
    (969043354, [(4, 7520)],),
    (969043194, [(12, 1)],),
    (969073434, [(4, 7520)],),
    (969043206, [(12, 1)],),
    (969103514, [(289, 1)],),
    (969043218, [(12, 2), (4, 1)],),
    (984706136, [(2, 1), (12, 4)],),
    (984706334, [(2, 3)],),
    (984706186, [(12, 3)],),
    (984706340, [(100, 1)],),
    (984706222, [(12, 5)],),
    (984706440, [(4, 480)],),
    (984706282, [(12, 1)],),
    (984708360, [(4, 480)],),
    (984706294, [(12, 1)],),
    (984710280, [(289, 1)],),
    (984706306, [(12, 2), (4, 1)],),
    (990406720, [(2, 1), (12, 4)],),
    (990406918, [(2, 3)],),
    (990406770, [(12, 3)],),
    (990406924, [(100, 1)],),
    (990406806, [(12, 5)],),
    (990407024, [(4, 120)],),
    (990406866, [(12, 1)],),
    (990407504, [(4, 120)],),
    (990406878, [(12, 1)],),
    (990407984, [(289, 1)],),
    (990406890, [(12, 2), (4, 1)],),
    (990966474, [(2, 1), (12, 4)],),
    (990966648, [(2, 3)],),
    (990966524, [(12, 3)],),
    (990966654, [(44, 1)],),
    (990966560, [(12, 1)],),
    (990966698, [(4, 159)],),
    (990966572, [(12, 3)],),
    (990967334, [(4, 159)],),
    (990966608, [(12, 3), (4, 1)],),
    (991197158, [(2, 1), (12, 4)],),
    (991197344, [(2, 3)],),
    (991197208, [(12, 3)],),
    (991197350, [(45, 1)],),
    (991197244, [(12, 1)],),
    (991197396, [(4, 41)],),
    (991197256, [(12, 3)],),
    (991197560, [(4, 41)],),
    (991197292, [(12, 2)],),
    (991197724, [(289, 1)],),
    (991197316, [(12, 2), (4, 1)],),
)

EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_006_svs = tuple(
    gen_rle_seek_read_actions(EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_006_svs_RLE))

EXAMPLE_FILE_POSITIONS_LENGTHS = {
    # 'EXAMPLE_FILE_POSITIONS_LENGTHS_SYNTHETIC': EXAMPLE_FILE_POSITIONS_LENGTHS_SYNTHETIC,
    'EXAMPLE_FILE_POSITIONS_LENGTHS_TIFF_CMU_1': EXAMPLE_FILE_POSITIONS_LENGTHS_TIFF_CMU_1,
    # 'EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_001_svs': EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_001_svs,
    # 'EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_002_svs': EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_002_svs,
    # 'EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_003_svs': EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_003_svs,
    # 'EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_004_svs': EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_004_svs,
    # 'EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_005_svs': EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_005_svs,
    # 'EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_006_svs': EXAMPLE_FILE_POSITIONS_LENGTHS_TUPAC_TR_006_svs,
}
