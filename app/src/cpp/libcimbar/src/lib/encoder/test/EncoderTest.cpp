/* This code is subject to the terms of the Mozilla Public License, v.2.0. http://mozilla.org/MPL/2.0/. */
#include "unittest.h"
#include "TestHelpers.h"

#include "encoder/EncoderPlus.h"
#include "fountain/FountainInit.h"
#include "image_hash/average_hash.h"
#include "serialize/format.h"
#include "util/byte_istream.h"
#include "util/ConfigScope.h"
#include "util/File.h"
#include "util/MakeTempDirectory.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>


TEST_CASE( "EncoderTest/testVanilla", "[unit]" )
{
	MakeTempDirectory tempdir;
	ConfigScope cs;
	cs.active_conf().ecc_bytes = 40;

	std::string inputFile = TestCimbar::getProjectDir() + "/LICENSE";
	std::string outPrefix = tempdir.path() / "encoder.vanilla";

	EncoderPlus enc(4, 2);
	assertEquals( 3, enc.encode(inputFile, outPrefix) );

	std::vector<uint64_t> hashes = {0xefc5cf004796e4c2, 0x4fa8ef4670878ea0, 0xee73af46efee0c09};
	for (unsigned i = 0; i < hashes.size(); ++i)
	{
		DYNAMIC_SECTION( "are we correct? : " << i )
		{
			std::string path = fmt::format("{}_{}.png", outPrefix, i);
			cv::Mat img = cv::imread(path);
			assertEquals( hashes[i], image_hash::average_hash(img) );
		}
	}
}

TEST_CASE( "EncoderTest/testFountain.4c", "[unit]" )
{
	MakeTempDirectory tempdir;
	ConfigScope cs(4);
	cs.active_conf().ecc_bytes = 40;

	std::string inputFile = TestCimbar::getProjectDir() + "/LICENSE";
	std::string outPrefix = tempdir.path() / "encoder.fountain";

	EncoderPlus enc(4, 2);
	assertEquals( 3, enc.encode_fountain(inputFile, outPrefix, 0) );

	std::vector<uint64_t> hashes = {0x2c6c0f22c666a5a5, 0x9510aca4f7c2c32f, 0x9bc8ff8b07c44b28};
	for (unsigned i = 0; i < hashes.size(); ++i)
	{
		DYNAMIC_SECTION( "are we correct? : " << i )
		{
			std::string path = fmt::format("{}_{}.png", outPrefix, i);
			cv::Mat img = cv::imread(path);
			assertEquals( hashes[i], image_hash::average_hash(img) );
		}
	}
}

TEST_CASE( "EncoderTest/testFountain.B", "[unit]" )
{
	MakeTempDirectory tempdir;
	ConfigScope cs;
	cs.active_conf().ecc_bytes = 40;

	std::string inputFile = TestCimbar::getProjectDir() + "/LICENSE";
	std::string outPrefix = tempdir.path() / "encoder.fountain";

	EncoderPlus enc(4, 2);
	assertEquals( 3, enc.encode_fountain(inputFile, outPrefix, 0) );

	std::vector<uint64_t> hashes = {0x46bd6948e8cc0423, 0x472aed24eb64a00d, 0xd7e36e40a0cc04ec};
	for (unsigned i = 0; i < hashes.size(); ++i)
	{
		DYNAMIC_SECTION( "are we correct? : " << i )
		{
			std::string path = fmt::format("{}_{}.png", outPrefix, i);
			cv::Mat img = cv::imread(path);
			assertEquals( hashes[i], image_hash::average_hash(img) );
		}
	}
}

TEST_CASE( "EncoderTest/testFountain.Compress", "[unit]" )
{
	MakeTempDirectory tempdir;

	std::string inputFile = TestCimbar::getProjectDir() + "/LICENSE";
	std::string outPrefix = tempdir.path() / "encoder.fountain";

	EncoderPlus enc(4, 2);
	assertEquals( 1, enc.encode_fountain(inputFile, outPrefix) );

	uint64_t hash = 0x3239c745674f8a06;
	std::string path = fmt::format("{}_0.png", outPrefix);
	cv::Mat img = cv::imread(path);
	assertEquals( hash, image_hash::average_hash(img) );
}

TEST_CASE( "EncoderTest/testPiecemealFountainEncoder", "[unit]" )
{
	// use the fountain_encoder_stream directly on a char*,int pair
	MakeTempDirectory tempdir;
	ConfigScope cs;
	cs.active_conf().ecc_bytes = 40;

	EncoderPlus enc(4, 2);

	std::string inputFile = TestCimbar::getProjectDir() + "/LICENSE";
	std::string contents = File(inputFile).read_all();
	assertEquals( File(inputFile).read_all(), contents );

	cimbar::byte_istream bis(contents.data(), contents.size());
	// equivalent to:
	// cimbar::bytebuf bb(contents.data(), contents.size());
	// std::istream is(&bb);

	fountain_encoder_stream::ptr fes = enc.create_fountain_encoder(bis, "LICENSE.txt");
	assertTrue( fes );

	std::optional<cv::Mat> frame = enc.encode_next(*fes);
	assertTrue( frame );

	uint64_t hash = 0xa487632082a1576f;
	assertEquals( hash, image_hash::average_hash(*frame) );
}

TEST_CASE( "EncoderTest/testFountain.Size", "[unit]" )
{
	MakeTempDirectory tempdir;

	std::string inputFile = TestCimbar::getProjectDir() + "/LICENSE";
	std::string outPrefix = tempdir.path() / "encoder.fountain";

	EncoderPlus enc(4, 2);
	assertEquals( 1, enc.encode_fountain(inputFile, outPrefix, 16, 1.6) );

	uint64_t hash = 0x3239c745674f8a06;
	std::string path = fmt::format("{}_0.png", outPrefix);
	cv::Mat img = cv::imread(path);
	assertEquals( 1024, img.rows );
	assertEquals( 1024, img.cols );
	assertEquals( hash, image_hash::average_hash(img) );
}
