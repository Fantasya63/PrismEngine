#include <iostream>
#include "Application.h"


int main(int argc, char* argv[])
{
    AppCreateInfo appInfo {
        .WindowInfo { 1920u, 1080u },
        .AppName = "Hello World",
        .Argc = argc,
        .Argv = argv
    };

    Application app(appInfo);
    app.Run();
}