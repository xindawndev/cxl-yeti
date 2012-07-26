#include "Common.h"

USINGNAMESPACE2;

#if defined(WIN32) && defined(_DEBUG)
#include <crtdbg.h>
#endif

extern unsigned char t1[];
extern unsigned int  t1_len;
extern unsigned char t1_gz[];
extern unsigned int  t1_gz_len;
extern unsigned int  t1_gz_header_len;

extern unsigned char t2[];
extern unsigned int  t2_len;
extern unsigned char t2_gz[];
extern unsigned int  t2_gz_len;
extern unsigned int  t2_gz_header_len;

typedef struct {
    unsigned char* uncompressed;
    unsigned int   uncompressed_len;
    unsigned char* compressed;
    unsigned int   compressed_len;
    unsigned int   compressed_header_len;
} TestVector;

TestVector TestVectors[] = {
    {t1, t1_len, t1_gz, t1_gz_len, t1_gz_header_len},
    {t2, t2_len, t2_gz, t2_gz_len, t2_gz_header_len},
};

static void error_hook() 
{
    fprintf(stderr, "STOPPING\n");
}

int zip_test(std::vector<std::string> args)
{
    size_t argc = args.size();
    for (size_t i = 0; i < argc; ++i)
    {
        if (i == 0) continue;
        std::cout << "Param[" << i << "] : " << args[i] << std::endl;
    }

    for (unsigned int t = 0; t < sizeof(TestVectors) / sizeof(TestVectors[0]); t++) {
        TestVector * v = &TestVectors[t];
        DataBuffer in1(v->compressed, v->compressed_len);
        DataBuffer out1;
        YETI_Result result = Zip::decompress_inflate(in1, out1);
        CHECK(result == YETI_SUCCESS);
        CHECK(out1.get_data_size() == v->uncompressed_len);
        CHECK(MemoryEqual(out1.get_data(), v->uncompressed, v->uncompressed_len));

        DataBuffer in2(v->uncompressed, v->uncompressed_len);
        DataBuffer out2;
        DataBuffer out2_check;
        result = Zip::compress_deflate(in2, out2, YETI_ZIP_COMPRESSION_LEVEL_MAX, Zip::GZIP);
        CHECK(result == YETI_SUCCESS);
        result = Zip::decompress_inflate(out2, out2_check);
        CHECK(result == YETI_SUCCESS);
        CHECK(out2_check.get_data_size() == in2.get_data_size());
        CHECK(MemoryEqual(v->uncompressed, out2_check.get_data(), in2.get_data_size()));

        // try with random data
        DataBuffer in3(300000);
        unsigned char * in3_p = in3.use_data();
        for (int i=0; i< 300000; i++) {
            *in3_p++ = System::get_random_integer();
        }
        in3.set_data_size(300000);
        DataBuffer out3;
        result = Zip::compress_deflate(in3, out3);
        CHECK(result == YETI_SUCCESS);
        DataBuffer out3_check;
        result = Zip::decompress_inflate(out3, out3_check);
        CHECK(result == YETI_SUCCESS);
        CHECK(in3 == out3_check);

        // try with redundant data
        in3_p = in3.use_data();
        for (int i=0; i< 200000; i+=4) {
            *in3_p++ = System::get_random_integer();
            *in3_p++ = 0;
            *in3_p++ = 0;
            *in3_p++ = 0;
        }
        result = Zip::compress_deflate(in3, out3);
        CHECK(result == YETI_SUCCESS);
        result = Zip::decompress_inflate(out3, out3_check);
        CHECK(result == YETI_SUCCESS);
        CHECK(in3 == out3_check);

        // streams
        for (unsigned int x = 0; x < 10; x++) {
            MemoryStream* ms_gz = new MemoryStream(v->compressed, v->compressed_len);
            InputStreamReference ms_gz_ref(ms_gz);
            ZipInflatingInputStream ziis(ms_gz_ref);
            DataBuffer buffer;
            YETI_Position position = 0;
            bool expect_eos = false;
            for (;;) {
                YETI_Size chunk = System::get_random_integer() % 40000;
                buffer.set_data_size(chunk);
                YETI_Size bytes_read = 0;
                result = ziis.read(buffer.use_data(), chunk, &bytes_read);
                if (expect_eos) {
                    CHECK(result == YETI_ERROR_EOS);
                    break;
                }
                if (result == YETI_ERROR_EOS) {
                    CHECK(position == v->uncompressed_len);
                } else {
                    CHECK(result == YETI_SUCCESS);
                }
                CHECK(bytes_read <= chunk);
                if (bytes_read != chunk) expect_eos = true;
                CHECK(MemoryEqual(v->uncompressed+position, 
                    buffer.get_data(),
                    bytes_read));
                position += bytes_read;
            }
            CHECK(position == v->uncompressed_len);
        }

        for (unsigned int x = 0; x < 10; x++) {
            MemoryStream* ms = new MemoryStream(v->uncompressed, v->uncompressed_len);
            InputStreamReference ms_ref(ms);
            ZipDeflatingInputStream zdis(ms_ref, YETI_ZIP_COMPRESSION_LEVEL_MAX, Zip::GZIP);
            DataBuffer buffer;
            YETI_Position position = 0;
            bool expect_eos = false;
            for (;;) {
                YETI_Size chunk = System::get_random_integer() % 40000;
                buffer.reserve(buffer.get_data_size()+chunk);
                YETI_Size bytes_read = 0;
                result = zdis.read(buffer.use_data() + buffer.get_data_size(), chunk, &bytes_read);
                if (expect_eos) {
                    CHECK(result == YETI_ERROR_EOS);
                    break;
                }
                CHECK(result == YETI_SUCCESS);
                CHECK(bytes_read <= chunk);
                if (bytes_read != chunk) expect_eos = true;
                position += bytes_read;
                buffer.set_data_size(buffer.get_data_size() + bytes_read);
            }
            DataBuffer out;
            DataBuffer check(v->uncompressed, v->uncompressed_len);
            CHECK(Zip::decompress_inflate(buffer, out) == YETI_SUCCESS);
            CHECK(out == check);
        }
    }

    return 0;
}

static TestRegister test("zip_test", zip_test);