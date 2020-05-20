@echo off
rmdir /s /q build
mkdir build
copy .\components\framework\libsloked.dll build
copy .\components\framework\tests\sloked_framework_tests.exe builds
copy .\components\headless\application-lib\libsloked_headless_app.dll build
copy .\components\bootstrap\sloked.exe build