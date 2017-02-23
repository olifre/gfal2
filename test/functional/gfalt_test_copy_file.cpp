/*
 * Copyright (c) CERN 2013-2017
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include <gfal_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/exceptions/gerror_to_cpp.h>
#include <transfer/gfal_transfer.h>

#include <common/gfal_lib_test.h>
#include <common/gfal_gtest_asserts.h>


class CopyTest: public testing::Test {
public:
    static const char* source_root;
    static const char* destination_root;

    char source[2048];
    char destination[2048];
    gfal2_context_t handle;

    CopyTest() {
        GError *error = NULL;
        handle =  gfal2_context_new(&error);
        Gfal::gerror_to_cpp(&error);
    }

    virtual ~CopyTest() {
        gfal2_context_free(handle);
    }

    virtual void SetUp() {
        generate_random_uri(source_root, "copyfile_source", source, 2048);
        generate_random_uri(destination_root, "copyfile", destination, 2048);

        RecordProperty("Source", source);
        RecordProperty("Destination", source);

        GError* error = NULL;
        int ret = generate_file_if_not_exists(handle, source, "file:///etc/hosts", &error);
        EXPECT_PRED_FORMAT2(AssertGfalSuccess, ret, error);
    }

    virtual void TearDown() {
        gfal_unlink(source);
        gfal_unlink(destination);
    }
};

const char* CopyTest::source_root;
const char* CopyTest::destination_root;

TEST_F(CopyTest, SimpleFileCopy)
{
    GError* error = NULL;
    int ret = gfalt_copy_file(handle, NULL, source, destination, &error);
    EXPECT_PRED_FORMAT2(AssertGfalSuccess, ret, error);
}


TEST_F(CopyTest, SimpleFileCopyENOENT)
{
    GError* error = NULL;
    int ret = gfal2_unlink(handle, source, &error);
    EXPECT_PRED_FORMAT2(AssertGfalSuccess, ret, error);

    ret = gfalt_copy_file(handle, NULL, source, destination, &error);
    EXPECT_PRED_FORMAT3(AssertGfalErrno, ret, error, ENOENT);
}


int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);

    if (argc < 2) {
        printf("Missing source and destination base urls\n");
        printf("\t%s [options] srm://host/base/path/ srm://destination/base/path/\n", argv[0]);
        return 1;
    }

    CopyTest::source_root = argv[1];
    CopyTest::destination_root = argv[2];

    // gfal2_log_set_level(G_LOG_LEVEL_DEBUG);

    return RUN_ALL_TESTS();
}
