#include "sloked/core/CLI.h"

#include "catch2/catch.hpp"

using namespace sloked;

// TODO: Mandatory parameters, parameter mappings

TEST_CASE("CLI correctly parses positional parameters") {
    const char *Args[] = {"1",        "-abcd", "--switch1", "--value1=value",
                          "--value2", "-o",    "2",         "--",
                          "--hello",  "world"};
    SlokedCLI cli;
    cli.Parse(sizeof(Args) / sizeof(const char *), Args, true);
    REQUIRE(cli.ArgCount() == 4);
    REQUIRE(cli.Argument(0) == "1");
    REQUIRE(cli.Argument(1) == "2");
    REQUIRE(cli.Argument(2) == "--hello");
    REQUIRE(cli.Argument(3) == "world");
    std::vector<std::string> args(cli.begin(), cli.end());
    REQUIRE(args == std::vector<std::string>{"1", "2", "--hello", "world"});
}

TEST_CASE("CLI correctly parses short parameters") {
    const char *Args[] = {"abc",
                          "-o",
                          "file1",
                          "--temp",
                          "def",
                          "-n",
                          "200",
                          "ghi",
                          "-x",
                          "-f",
                          "3.14"
                          "--test",
                          "--",
                          "-a"};
    SlokedCLI cli;
    cli.Define("-o", cli.Option<std::string>());
    cli.Define("-n", cli.Option<int>());
    cli.Define("-x", cli.Option<bool>());
    cli.Define("-f", cli.Option<double>());
    cli.Define("-a", cli.Option<bool>());
    cli.Parse(sizeof(Args) / sizeof(const char *), Args, true);
    REQUIRE(cli.Has('o'));
    REQUIRE(cli.Has('n'));
    REQUIRE(cli.Has('x'));
    REQUIRE_FALSE(cli.Has('a'));
    REQUIRE(cli['o'].Type() == SlokedCLIValue::Type::String);
    REQUIRE(cli['n'].Type() == SlokedCLIValue::Type::Integer);
    REQUIRE(cli['x'].Type() == SlokedCLIValue::Type::Boolean);
    REQUIRE(cli['f'].Type() == SlokedCLIValue::Type::Float);
    REQUIRE_THROWS(cli['a']);
    REQUIRE(cli['o'].As<std::string>() == "file1");
    REQUIRE_THROWS(cli['o'].As<int>());
    REQUIRE_THROWS(cli['o'].As<double>());
    REQUIRE_THROWS(cli['o'].As<bool>());
    REQUIRE(cli['n'].As<int>() == 200);
    REQUIRE(cli['x'].As<bool>());
    REQUIRE(cli['f'].As<double>() == Approx(3.14));
}

TEST_CASE("CLI correctly parses long parameters") {
    const char *Args[] = {"1",      "--string=Hello", "world",   "--int", "5",
                          "--bool", "5678",           "--float", "2.71",  "--",
                          "--bool2"};
    SlokedCLI cli;
    cli.Define("--string", cli.Option<std::string>());
    cli.Define("--int", cli.Option<int>());
    cli.Define("--float", cli.Option<double>());
    cli.Define("--bool", cli.Option<bool>());
    cli.Define("--bool2", cli.Option<bool>());
    cli.Parse(sizeof(Args) / sizeof(const char *), Args, true);
    REQUIRE(cli.Has("string"));
    REQUIRE(cli.Has("int"));
    REQUIRE(cli.Has("float"));
    REQUIRE(cli.Has("bool"));
    REQUIRE_FALSE(cli.Has("bool2"));
    REQUIRE(cli["string"].Type() == SlokedCLIValue::Type::String);
    REQUIRE(cli["int"].Type() == SlokedCLIValue::Type::Integer);
    REQUIRE(cli["float"].Type() == SlokedCLIValue::Type::Float);
    REQUIRE(cli["bool"].Type() == SlokedCLIValue::Type::Boolean);
    REQUIRE_THROWS(cli["bool2"]);
    REQUIRE(cli["string"].As<std::string>() == "Hello");
    REQUIRE_THROWS(cli["string"].As<int>());
    REQUIRE_THROWS(cli["string"].As<double>());
    REQUIRE_THROWS(cli["string"].As<bool>());
    REQUIRE(cli["int"].As<int>() == 5);
    REQUIRE(cli["bool"].As<bool>());
    REQUIRE(cli["float"].As<double>() == Approx(2.71));
}

TEST_CASE("CLI correctly parses mixed parameters") {
    const char *Args[] = {"abc",   "-o",          "test", "--file", "-x",
                          "hello", "--test=test", "a",    "b",      "-e"};
    SlokedCLI cli;
    cli.Define("-o,--output", cli.Option<std::string>());
    cli.Define("-f,--file", cli.Option<bool>());
    cli.Define("-x,--extend", cli.Option<bool>());
    cli.Define("-t,--test", cli.Option<std::string>());
    cli.Define("-e,--enable", cli.Option<bool>());
    cli.Parse(sizeof(Args) / sizeof(const char *), Args, true);
    REQUIRE(cli.Has("output"));
    REQUIRE(cli.Has('f'));
    REQUIRE(cli.Has("extend"));
    REQUIRE(cli.Has('t'));
    REQUIRE(cli.Has("enable"));
    REQUIRE(cli["output"].As<std::string>() == cli['o'].As<std::string>());
    REQUIRE(cli["output"].As<std::string>() == "test");
    REQUIRE(cli["file"].As<bool>() == cli['f'].As<bool>());
    REQUIRE(cli["file"].As<bool>());
    REQUIRE(cli["extend"].As<bool>() == cli['x'].As<bool>());
    REQUIRE(cli["extend"].As<bool>());
    REQUIRE(cli["test"].As<std::string>() == cli['t'].As<std::string>());
    REQUIRE(cli["test"].As<std::string>() == "test");
    REQUIRE(cli["enable"].As<bool>() == cli['e'].As<bool>());
    REQUIRE(cli["enable"].As<bool>());
}

TEST_CASE("CLI fails on unknown parameters") {
    const char *Args[] = {"abc", "def", "-a", "--hello", "xyz", "--", "--test"};
    SlokedCLI cli;
    cli.Define("-a", cli.Option<bool>());
    REQUIRE_THROWS(cli.Parse(sizeof(Args) / sizeof(const char *), Args));
    SlokedCLI cli2;
    cli2.Define("--hello", cli.Option<bool>());
    REQUIRE_THROWS(cli.Parse(sizeof(Args) / sizeof(const char *), Args));
    SlokedCLI cli3;
    cli3.Define("--hello", cli.Option<bool>());
    cli3.Define("-a", cli.Option<bool>());
    REQUIRE_NOTHROW(cli3.Parse(sizeof(Args) / sizeof(const char *), Args));
}