#include "Application.h"
#include "CaptureRequest.h"
#include "modes/RunMode.h"

int main(int argc, char** argv)
{
    const auto request = planets::app::CaptureRequest::Parse(argc, argv);

    planets::app::Application app;
    return planets::app::MakeRunMode(request)->Run(app) ? 0 : -1;
}
