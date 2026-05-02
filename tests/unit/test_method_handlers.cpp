/* ************************************************************************** */
/*  test_method_handlers.cpp - Unit tests for HTTP method handlers            */
/*                                                                            */
/*  Uses minitest framework. Tests the real MethodHandler implementation.     */
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

#include <LocationConfig.hpp>
#include <ServerConfig.hpp>
#include <MethodHandler.hpp>
#include <Enums.hpp>

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

static ServerConfig makeServerConfig(const std::string& root)
{
	ServerConfig srv;
	srv.host = "127.0.0.1";
	srv.port = "8080";
	srv.root = root;
	srv.client_max_body_size = 10485760;
	srv.autoindex = OFF;
	return srv;
}

static LocationConfig makeLocationConfig(const std::string& root,
										 const std::string& index,
										 AutoIndex autoindex,
										 const std::string& uploadStore)
{
	LocationConfig loc;
	loc.path = "/";
	loc.root = root;
	loc.index = index;
	loc.autoindex = autoindex;
	loc.upload_store = uploadStore;
	loc.return_code = 0;
	return loc;
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

	LocationConfig loc = makeLocationConfig(TEST_DIR, "", OFF, "");
	ServerConfig srv = makeServerConfig(TEST_DIR);

	MethodHandler::Result res = MethodHandler::handleGet("/hello.html", loc, srv);

	ASSERT_EQ(200, res.statusCode);

	removeTestDir();
}

/* 2. GET on a non-existent file should return 404 */
TEST(MethodHandler, GetMissingFileReturns404)
{
	createTestDir();

	LocationConfig loc = makeLocationConfig(TEST_DIR, "", OFF, "");
	ServerConfig srv = makeServerConfig(TEST_DIR);

	MethodHandler::Result res = MethodHandler::handleGet("/no_such_file.html", loc, srv);

	ASSERT_EQ(404, res.statusCode);

	removeTestDir();
}

/* 3. GET on a directory with an index configured should serve the index file */
TEST(MethodHandler, GetDirectoryWithIndex)
{
	createTestDir();
	writeFile(std::string(TEST_DIR) + "/index.html",
			  "<html><body>Index Page</body></html>");

	LocationConfig loc = makeLocationConfig(TEST_DIR, "index.html", OFF, "");
	ServerConfig srv = makeServerConfig(TEST_DIR);

	MethodHandler::Result res = MethodHandler::handleGet("/", loc, srv);

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

	LocationConfig loc = makeLocationConfig(TEST_DIR, "", ON, "");
	ServerConfig srv = makeServerConfig(TEST_DIR);

	MethodHandler::Result res = MethodHandler::handleGet("/", loc, srv);

	ASSERT_EQ(200, res.statusCode);
	ASSERT_STR_CONTAINS(res.body, "<html");

	removeTestDir();
}

/* 5. GET on a directory with autoindex off and no index file -> 403 */
TEST(MethodHandler, GetDirectoryAutoindexOff)
{
	createTestDir();

	LocationConfig loc = makeLocationConfig(TEST_DIR, "", OFF, "");
	ServerConfig srv = makeServerConfig(TEST_DIR);

	MethodHandler::Result res = MethodHandler::handleGet("/", loc, srv);

	ASSERT_EQ(403, res.statusCode);

	removeTestDir();
}

/* 6. POST upload should return 201 */
TEST(MethodHandler, PostUploadReturns201)
{
	createTestDir();
	std::string uploadDir = std::string(TEST_DIR) + "/uploads";
	mkdir(uploadDir.c_str(), 0755);

	LocationConfig loc = makeLocationConfig(TEST_DIR, "", OFF, uploadDir);
	ServerConfig srv = makeServerConfig(TEST_DIR);

	MethodHandler::Result res = MethodHandler::handlePost("/upload", "file contents here", loc, srv);

	ASSERT_EQ(201, res.statusCode);

	removeTestDir();
}

/* 7. DELETE on an existing file should return 204 */
TEST(MethodHandler, DeleteExistingReturns204)
{
	createTestDir();
	std::string filePath = std::string(TEST_DIR) + "/to_delete.txt";
	writeFile(filePath, "delete me");

	LocationConfig loc = makeLocationConfig(TEST_DIR, "", OFF, "");
	ServerConfig srv = makeServerConfig(TEST_DIR);

	MethodHandler::Result res = MethodHandler::handleDelete("/to_delete.txt", loc, srv);

	ASSERT_EQ(204, res.statusCode);

	removeTestDir();
}

/* 8. DELETE on a non-existent file should return 404 */
TEST(MethodHandler, DeleteMissingReturns404)
{
	createTestDir();

	LocationConfig loc = makeLocationConfig(TEST_DIR, "", OFF, "");
	ServerConfig srv = makeServerConfig(TEST_DIR);

	MethodHandler::Result res = MethodHandler::handleDelete("/ghost.txt", loc, srv);

	ASSERT_EQ(404, res.statusCode);

	removeTestDir();
}

/* 9. Method not allowed: isMethodAllowed returns false, handler returns 405 */
TEST(MethodHandler, MethodNotAllowedReturns405)
{
	createTestDir();

	LocationConfig loc = makeLocationConfig(TEST_DIR, "", OFF, "");
	loc.limit_except.push_back(GET);
	ServerConfig srv = makeServerConfig(TEST_DIR);

	bool allowed = MethodHandler::isMethodAllowed("DELETE", loc);
	ASSERT_FALSE(allowed);

	MethodHandler::Result res = MethodHandler::handleDelete("/some_file.txt", loc, srv);
	ASSERT_EQ(405, res.statusCode);

	removeTestDir();
}

/* ========================================================================== */
/*  Runner                                                                    */
/* ========================================================================== */

MINITEST_MAIN()
