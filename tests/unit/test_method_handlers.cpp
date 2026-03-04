/* ************************************************************************** */
/*  test_method_handlers.cpp - Unit tests for HTTP method handlers            */
/*                                                                            */
/*  Uses minitest framework. All tests currently fail because the stub        */
/*  handlers return default values (statusCode=0).                            */
/* ************************************************************************** */

#include "minitest.hpp"

#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>

/* ========================================================================== */
/*  Stub types                                                                */
/* ========================================================================== */

struct HandlerResult {
	int                                statusCode;
	std::string                        body;
	std::map<std::string, std::string> headers;
};

struct LocationConfig {
	std::string              root;
	std::string              index;
	bool                     autoindex;
	std::vector<std::string> limitExcept;
	std::string              uploadStore;
};

/* ========================================================================== */
/*  Stub handler class                                                        */
/* ========================================================================== */

class MethodHandler {
public:
	static HandlerResult handleGet(const std::string& path,
								   const LocationConfig& loc)
	{
		(void)path;
		(void)loc;
		HandlerResult r;
		r.statusCode = 0;
		return r;
	}

	static HandlerResult handlePost(const std::string& path,
									const std::string& body,
									const LocationConfig& loc)
	{
		(void)path;
		(void)body;
		(void)loc;
		HandlerResult r;
		r.statusCode = 0;
		return r;
	}

	static HandlerResult handleDelete(const std::string& path,
									  const LocationConfig& loc)
	{
		(void)path;
		(void)loc;
		HandlerResult r;
		r.statusCode = 0;
		return r;
	}

	static bool isMethodAllowed(const std::string& method,
								const LocationConfig& loc)
	{
		(void)method;
		(void)loc;
		return false;
	}
};

/* ========================================================================== */
/*  Helper: temp directory management                                         */
/* ========================================================================== */

static const char* TEST_DIR = "/tmp/webserv_test_handlers";

static void createTestDir()
{
	mkdir(TEST_DIR, 0755);
}

static void removeTestDir()
{
	std::string cmd = "rm -rf ";
	cmd += TEST_DIR;
	std::system(cmd.c_str());
}

static void writeFile(const std::string& path, const std::string& content)
{
	std::ofstream ofs(path.c_str());
	ofs << content;
	ofs.close();
}

/* ========================================================================== */
/*  Tests                                                                     */
/* ========================================================================== */

/* 1. GET on an existing file should return 200 */
TEST(MethodHandler, GetExistingFileReturns200)
{
	createTestDir();
	std::string filePath = std::string(TEST_DIR) + "/hello.html";
	writeFile(filePath, "<html><body>Hello</body></html>");

	LocationConfig loc;
	loc.root      = TEST_DIR;
	loc.index     = "";
	loc.autoindex = false;
	loc.uploadStore = "";

	HandlerResult res = MethodHandler::handleGet("/hello.html", loc);

	ASSERT_EQ(200, res.statusCode);

	removeTestDir();
}

/* 2. GET on a non-existent file should return 404 */
TEST(MethodHandler, GetMissingFileReturns404)
{
	createTestDir();

	LocationConfig loc;
	loc.root      = TEST_DIR;
	loc.index     = "";
	loc.autoindex = false;
	loc.uploadStore = "";

	HandlerResult res = MethodHandler::handleGet("/no_such_file.html", loc);

	ASSERT_EQ(404, res.statusCode);

	removeTestDir();
}

/* 3. GET on a directory with an index configured should serve the index file */
TEST(MethodHandler, GetDirectoryWithIndex)
{
	createTestDir();
	writeFile(std::string(TEST_DIR) + "/index.html",
			  "<html><body>Index Page</body></html>");

	LocationConfig loc;
	loc.root      = TEST_DIR;
	loc.index     = "index.html";
	loc.autoindex = false;
	loc.uploadStore = "";

	HandlerResult res = MethodHandler::handleGet("/", loc);

	ASSERT_EQ(200, res.statusCode);
	ASSERT_STR_CONTAINS(res.body, "Index Page");

	removeTestDir();
}

/* 4. GET on a directory with autoindex on and no index file -> 200 with HTML listing */
TEST(MethodHandler, GetDirectoryAutoindexOn)
{
	createTestDir();
	writeFile(std::string(TEST_DIR) + "/file_a.txt", "aaa");
	writeFile(std::string(TEST_DIR) + "/file_b.txt", "bbb");

	LocationConfig loc;
	loc.root      = TEST_DIR;
	loc.index     = "";
	loc.autoindex = true;
	loc.uploadStore = "";

	HandlerResult res = MethodHandler::handleGet("/", loc);

	ASSERT_EQ(200, res.statusCode);
	ASSERT_STR_CONTAINS(res.body, "<html");

	removeTestDir();
}

/* 5. GET on a directory with autoindex off and no index file -> 403 */
TEST(MethodHandler, GetDirectoryAutoindexOff)
{
	createTestDir();

	LocationConfig loc;
	loc.root      = TEST_DIR;
	loc.index     = "";
	loc.autoindex = false;
	loc.uploadStore = "";

	HandlerResult res = MethodHandler::handleGet("/", loc);

	ASSERT_EQ(403, res.statusCode);

	removeTestDir();
}

/* 6. POST upload should return 201 */
TEST(MethodHandler, PostUploadReturns201)
{
	createTestDir();
	std::string uploadDir = std::string(TEST_DIR) + "/uploads";
	mkdir(uploadDir.c_str(), 0755);

	LocationConfig loc;
	loc.root        = TEST_DIR;
	loc.index       = "";
	loc.autoindex   = false;
	loc.uploadStore = uploadDir;

	HandlerResult res = MethodHandler::handlePost("/upload", "file contents here", loc);

	ASSERT_EQ(201, res.statusCode);

	removeTestDir();
}

/* 7. DELETE on an existing file should return 204 */
TEST(MethodHandler, DeleteExistingReturns204)
{
	createTestDir();
	std::string filePath = std::string(TEST_DIR) + "/to_delete.txt";
	writeFile(filePath, "delete me");

	LocationConfig loc;
	loc.root      = TEST_DIR;
	loc.index     = "";
	loc.autoindex = false;
	loc.uploadStore = "";

	HandlerResult res = MethodHandler::handleDelete("/to_delete.txt", loc);

	ASSERT_EQ(204, res.statusCode);

	removeTestDir();
}

/* 8. DELETE on a non-existent file should return 404 */
TEST(MethodHandler, DeleteMissingReturns404)
{
	createTestDir();

	LocationConfig loc;
	loc.root      = TEST_DIR;
	loc.index     = "";
	loc.autoindex = false;
	loc.uploadStore = "";

	HandlerResult res = MethodHandler::handleDelete("/ghost.txt", loc);

	ASSERT_EQ(404, res.statusCode);

	removeTestDir();
}

/* 9. Method not allowed: isMethodAllowed returns false, handler returns 405 */
TEST(MethodHandler, MethodNotAllowedReturns405)
{
	LocationConfig loc;
	loc.root      = TEST_DIR;
	loc.index     = "";
	loc.autoindex = false;
	loc.uploadStore = "";
	loc.limitExcept.push_back("GET");

	bool allowed = MethodHandler::isMethodAllowed("DELETE", loc);
	ASSERT_FALSE(allowed);

	/* When the method is not allowed the server should respond with 405.
	   Since the stub always returns statusCode=0 this assertion will fail,
	   demonstrating that the real implementation is needed. */
	HandlerResult res = MethodHandler::handleDelete("/some_file.txt", loc);
	ASSERT_EQ(405, res.statusCode);
}

/* ========================================================================== */
/*  Runner                                                                    */
/* ========================================================================== */

MINITEST_MAIN()
