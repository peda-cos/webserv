/* ************************************************************************** */
/*  test_chunked_decoder.cpp — TDD tests for the chunked transfer decoder     */
/*                                                                            */
/*  Compile:                                                                  */
/*    c++ -std=c++98 -Wall -Wextra -Werror -I tests/framework                */
/*        -I includes tests/unit/test_chunked_decoder.cpp -o test_chunked_decoder */
/* ************************************************************************** */

#include "minitest.hpp"

#include <ChunkedDecoder.hpp>

#include <string>
#include <stdexcept>

/* ------------------------------------------------------------------------ */
/* 1. A single chunk followed by the terminator must produce the correct    */
/*    decoded body.                                                         */
/* ------------------------------------------------------------------------ */
TEST(ChunkedDecoder, SingleChunk)
{
	ChunkedDecoder decoder;
	decoder.feed("5\r\nhello\r\n0\r\n\r\n");

	ASSERT_TRUE(decoder.isComplete());
	ASSERT_EQ(std::string("hello"), decoder.getBody());
}

/* ------------------------------------------------------------------------ */
/* 2. Multiple chunks concatenated must produce the combined body.          */
/* ------------------------------------------------------------------------ */
TEST(ChunkedDecoder, MultipleChunks)
{
	ChunkedDecoder decoder;
	decoder.feed("5\r\nhello\r\n6\r\n world\r\n0\r\n\r\n");

	ASSERT_TRUE(decoder.isComplete());
	ASSERT_EQ(std::string("hello world"), decoder.getBody());
}

/* ------------------------------------------------------------------------ */
/* 3. Hex chunk size "a" (10 decimal) must consume exactly 10 bytes.        */
/* ------------------------------------------------------------------------ */
TEST(ChunkedDecoder, HexSizeParsed)
{
	ChunkedDecoder decoder;
	decoder.feed("a\r\n0123456789\r\n0\r\n\r\n");

	ASSERT_TRUE(decoder.isComplete());
	ASSERT_EQ(std::string("0123456789"), decoder.getBody());
}

/* ------------------------------------------------------------------------ */
/* 4. The zero-length terminator chunk must signal end-of-body.             */
/* ------------------------------------------------------------------------ */
TEST(ChunkedDecoder, ZeroChunkSignalsEof)
{
	ChunkedDecoder decoder;
	decoder.feed("0\r\n\r\n");

	ASSERT_TRUE(decoder.isComplete());
	ASSERT_EQ(std::string(""), decoder.getBody());
}

/* ------------------------------------------------------------------------ */
/* 5. A non-hex chunk size must throw ChunkedDecodeException.               */
/* ------------------------------------------------------------------------ */
TEST(ChunkedDecoder, NonHexSizeThrows)
{
	ChunkedDecoder decoder;
	ASSERT_THROWS(decoder.feed("xyz\r\n"), ChunkedDecodeException);
}

/* ------------------------------------------------------------------------ */
/* 6. A truncated chunk (fewer data bytes than declared) must leave the     */
/*    decoder in an incomplete state without signalling an error.           */
/* ------------------------------------------------------------------------ */
TEST(ChunkedDecoder, TruncatedChunkAwaitsMore)
{
	ChunkedDecoder decoder;
	/* Size declares 10 bytes ("a" hex), but only 5 arrive. */
	decoder.feed("a\r\nhello");

	ASSERT_FALSE(decoder.isComplete());
	ASSERT_FALSE(decoder.hasError());
}

MINITEST_MAIN()
