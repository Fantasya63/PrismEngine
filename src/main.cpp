#include <iostream>
#include "Application.h"


int main(int argc, char* argv[])
{
    AppCreateInfo appInfo {
        .WindowInfo { 1280u, 656u },
        .AppName = "Hello World",
        .Argc = argc,
        .Argv = argv
    };

    Application app(appInfo);
    app.Run();
}