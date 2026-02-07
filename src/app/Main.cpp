#include "Application.h"

int main()
{
    planets::app::Application app;

    if (!app.Initialize())
        return -1;

    app.Run();
    app.Shutdown();

    return 0;
}
