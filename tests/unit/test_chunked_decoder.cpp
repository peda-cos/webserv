#include "minitest.hpp"

#include <ChunkedDecoder.hpp>

#include <string>
#include <stdexcept>

TEST(ChunkedDecoder, SingleChunk)
{
	ChunkedDecoder decoder;
	decoder.feed("5\r\nhello\r\n0\r\n\r\n");

	ASSERT_TRUE(decoder.isComplete());
	ASSERT_EQ(std::string("hello"), decoder.getBody());
}

TEST(ChunkedDecoder, MultipleChunks)
{
	ChunkedDecoder decoder;
	decoder.feed("5\r\nhello\r\n6\r\n world\r\n0\r\n\r\n");

	ASSERT_TRUE(decoder.isComplete());
	ASSERT_EQ(std::string("hello world"), decoder.getBody());
}

TEST(ChunkedDecoder, HexSizeParsed)
{
	ChunkedDecoder decoder;
	decoder.feed("a\r\n0123456789\r\n0\r\n\r\n");

	ASSERT_TRUE(decoder.isComplete());
	ASSERT_EQ(std::string("0123456789"), decoder.getBody());
}

TEST(ChunkedDecoder, ZeroChunkSignalsEof)
{
	ChunkedDecoder decoder;
	decoder.feed("0\r\n\r\n");

	ASSERT_TRUE(decoder.isComplete());
	ASSERT_EQ(std::string(""), decoder.getBody());
}

TEST(ChunkedDecoder, NonHexSizeThrows)
{
	ChunkedDecoder decoder;
	ASSERT_THROWS(decoder.feed("xyz\r\n"), ChunkedDecodeException);
}

TEST(ChunkedDecoder, TruncatedChunkAwaitsMore)
{
	ChunkedDecoder decoder;
	decoder.feed("a\r\nhello");

	ASSERT_FALSE(decoder.isComplete());
	ASSERT_FALSE(decoder.hasError());
}

TEST(ChunkedDecoder, PreservesRemainderAfterDone)
{
	ChunkedDecoder decoder;
	decoder.feed("5\r\nhello\r\n0\r\n\r\nNEXT");

	ASSERT_TRUE(decoder.isComplete());
	ASSERT_EQ(std::string("NEXT"), decoder.getRemainder());
}

TEST(ChunkedDecoder, SupportsTrailerHeaders)
{
	ChunkedDecoder decoder;
	decoder.feed("5\r\nhello\r\n0\r\nX-Test: yes\r\n\r\n");

	ASSERT_TRUE(decoder.isComplete());
	ASSERT_EQ(std::string("hello"), decoder.getBody());
}

TEST(ChunkedDecoder, BodyTooLargeThrowsSpecificException)
{
	ChunkedDecoder decoder;
	decoder.setMaxBodySize(4);
	ASSERT_THROWS(decoder.feed("5\r\nhello\r\n0\r\n\r\n"), ChunkedBodyTooLargeException);
}

MINITEST_MAIN()