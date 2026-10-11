#include "RunModes.h"
#include "Application.h"

namespace planets::app
{

bool InteractiveMode::Run(Application& app)
{
    if (!app.Initialize())
        return false;
    app.Run();
    app.Shutdown();
    return true;
}

} // namespace planets::app
