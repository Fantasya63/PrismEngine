#include <iostream>
#include "Application.h"


int main(int argc, char* argv[])
{
    AppCreateInfo appInfo {
        .appName = "Hello World",
        .argc = argc,
        .argv = argv
    };

    Application app(appInfo);
    app.Run();
}